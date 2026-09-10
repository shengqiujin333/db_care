/**
 * @file    rf_twi_driver.c
 * @brief   Phase 2.4 — rf_twi_driver: Bit-banged TWI driver for UM2005C
 *
 * MCU:  CW32L010Y8M6 (TSSOP-20)
 * Bus:  Bit-banged TWI on PB02(CLK)/PB03(DATA) @ ~1MHz
 * Peer: UM2005C RF transmitter (SOT23-6L)
 * Instance: GPIO_IN_INTERRUPT (connectivity.json bus kind for nets RF_CLK/RF_DATA)
 *
 * Sources:
 *   UM2005C_数据手册_V1.2_V1.2.pdf#page=12 (TWI protocol: 16-cycle R/W, special commands)
 *   UM2005C_数据手册_V1.2_V1.2.pdf#page=7  (Timing: tCH/tCL >= 500ns, FSCL <= 1MHz)
 *   UM2005C_数据手册_V1.2_V1.2.pdf#page=14 (State diagram: Sleep -> TWI_ON -> config -> TX -> Sleep)
 *   UM2005C_数据手册_V1.2_V1.2.pdf#page=6  (Timing: tSLP-TX=1.2ms, tXTAL=400us, tTUNE=300us)
 *   UM2005C_数据手册_V1.2_V1.2.pdf#page=13 (LBD: read via TWI after TWI_ON)
 *   connectivity.json#buses[GPIO_IN_INTERRUPT] (RF_CLK=PB02, RF_DATA=PB03)
 *   cw32l010_gpio.h (GPIO macros for PB02, PB03)
 *   cw32l010_sysctrl.h (__SYSCTRL_GPIOB_CLK_ENABLE)
 *   output/design/common_types.h (error_code_t, rf_packet_t)
 *
 * Consumers: rf_mgr (application), packet_builder (service)
 */

#include "rf_twi_driver.h"

/* ====================================================================
 * Module-Level State
 * ==================================================================== */

/** Singleton driver state */
static rf_twi_driver_t s_driver = {
    .state = RF_TWI_STATE_UNINIT,
    .timeout_us = 10000U,
    .initialized = false
};

/* ====================================================================
 * Helpers -- NOP-based Delay
 * ==================================================================== */

/**
 * @brief Microsecond delay using NOP loops at 48MHz HCLK
 *
 * At 48MHz, 1us ~ 48 CPU cycles. Each NOP takes 1 cycle (Cortex-M0+).
 * We use a volatile loop to prevent compiler optimization.
 *
 * Note: This is a rough delay; for precise timing use a hardware timer.
 * The TWI protocol only requires CLK <= 1MHz, so +/-20% jitter is acceptable.
 */
void rf_twi_delay_us(uint32_t us)
{
    for (uint32_t u = 0; u < us; u++) {
        /* ~48 NOPs ~ 1us @ 48MHz */
        for (volatile uint32_t i = 0; i < 12; i++) {
            __NOP(); __NOP(); __NOP(); __NOP();
        }
    }
}

/**
 * @brief Millisecond delay using rf_twi_delay_us
 *
 * Source: system_cw32l010.h#L68 (FirmwareDelay declaration used as reference)
 */
void rf_twi_delay_ms(uint32_t ms)
{
    for (uint32_t m = 0; m < ms; m++) {
        rf_twi_delay_us(1000);
    }
}

/* ====================================================================
 * GPIO Configuration
 * ==================================================================== */

int32_t rf_twi_init(void)
{
    if (s_driver.initialized) {
        return ERR_OK;  /* Already initialized, idempotent */
    }

    /* ---------------------- Enable GPIOB clock ---------------------- */
    /* Source: cw32l010_sysctrl.h#L351 */
    __SYSCTRL_GPIOB_CLK_ENABLE();

    /* ---------------------- Configure PB02 (CLK) -------------------- */
    /* Set as GPIO function (AF0) */
    RF_TWI_CLK_AF_GPIO();
    /* Digital mode (not analog) */
    RF_TWI_CLK_DIGITAL();
    /* Output direction */
    RF_TWI_CLK_DIR_OUT();
    /* Push-pull output (not open-drain) */
    PB02_PUSHPULL_ENABLE();
    /* No internal pull-up */
    PB02_PUR_DISABLE();
    /* Drive low initially */
    RF_TWI_CLK_SET_LOW();

    /* ---------------------- Configure PB03 (DATA) ------------------- */
    /* Set as GPIO function (AF0) */
    RF_TWI_DATA_AF_GPIO();
    /* Digital mode (not analog) */
    RF_TWI_DATA_DIGITAL();
    /* Output direction (default for write mode) */
    RF_TWI_DATA_DIR_OUT();
    /* Push-pull output */
    PB03_PUSHPULL_ENABLE();
    /* No internal pull-up */
    PB03_PUR_DISABLE();
    /* Drive low initially */
    RF_TWI_DATA_SET_LOW();

    /* Update driver state */
    s_driver.state = RF_TWI_STATE_IDLE;
    s_driver.initialized = true;

    LOG_INFO("rf_twi: initialized (PB02=CLK, PB03=DATA)");
    return ERR_OK;
}

int32_t rf_twi_deinit(void)
{
    if (!s_driver.initialized) {
        return ERR_OK;
    }

    /* Set both pins to input mode (high-Z) for low-power sleep */
    RF_TWI_CLK_DIR_IN();
    RF_TWI_DATA_DIR_IN();

    /* Disable pull-ups to minimize leakage */
    PB02_PUR_DISABLE();
    PB03_PUR_DISABLE();

    s_driver.state = RF_TWI_STATE_UNINIT;
    s_driver.initialized = false;

    LOG_INFO("rf_twi: deinitialized");
    return ERR_OK;
}

/* ====================================================================
 * TWI Transaction Primitives
 * ==================================================================== */

/**
 * @brief Transmit 8 bits (address or data byte) via TWI
 *
 * Drives DATA with each bit MSB-first, generating one CLK cycle per bit.
 *
 * @param byte  Byte to transmit (8 bits)
 */
static void rf_twi_send_byte(uint8_t byte)
{
    for (int32_t i = 7; i >= 0; i--) {
        rf_twi_cycle((byte >> i) & 1);
    }
}

/**
 * @brief Receive 8 bits from UM2005C via TWI
 *
 * Releases DATA (set to input), generates CLK cycles, samples DATA on
 * falling edge, assembles byte MSB-first.
 *
 * @return uint8_t  Received byte
 */
static uint8_t rf_twi_recv_byte(void)
{
    uint8_t byte = 0;

    /* Release DATA line: switch to input mode */
    RF_TWI_DATA_DIR_IN();

    for (int32_t i = 7; i >= 0; i--) {
        if (rf_twi_sample_cycle()) {
            byte |= (1 << i);
        }
    }

    /* Re-assert DATA line: switch back to output */
    RF_TWI_DATA_DIR_OUT();
    RF_TWI_DATA_SET_LOW();

    return byte;
}

/* ====================================================================
 * TWI Special Commands
 * ==================================================================== */

int32_t rf_twi_on(void)
{
    if (!s_driver.initialized) {
        return ERR_NOT_INIT;
    }

    /* Ensure CLK and DATA are both low before starting */
    RF_TWI_CLK_SET_LOW();
    RF_TWI_DATA_SET_LOW();

    /* Send 32 CLK cycles with DATA=0 */
    for (uint32_t i = 0; i < RF_TWI_ON_CYCLES; i++) {
        rf_twi_cycle(0);
    }

    s_driver.state = RF_TWI_STATE_ACTIVE;

    LOG_DEBUG("rf_twi: TWI_ON sent (%u cycles)", RF_TWI_ON_CYCLES);
    return ERR_OK;
}

/**
 * @brief Send a special command to UM2005C
 *
 * Special commands use address 0x3F (W/R=1, A[5:0]=0x3F) + data byte.
 *
 * @param cmd_byte  Command data byte (0x01=TWI_RST, 0x02=TWI_OFF, 0x04=SOFT_RST)
 * @return ERR_OK on success
 */
static int32_t rf_twi_send_command(uint8_t cmd_byte)
{
    if (!s_driver.initialized) {
        return ERR_NOT_INIT;
    }

    /* Address byte: W/R=1, A5..A0 = 0x3F, bit8=0 */
    /* Bit layout: [W/R=1][A5=1][A4=1][A3=1][A2=1][A1=1][A0=1][X=0] = 0xFE */
    rf_twi_send_byte(0xFE);

    /* Data byte: command value */
    rf_twi_send_byte(cmd_byte);

    /* After command, both CLK and DATA should be low (idle) */
    RF_TWI_CLK_SET_LOW();
    RF_TWI_DATA_SET_LOW();

    return ERR_OK;
}

int32_t rf_twi_off(void)
{
    int32_t ret = rf_twi_send_command(0x02);
    if (ret == ERR_OK) {
        s_driver.state = RF_TWI_STATE_IDLE;
        LOG_DEBUG("rf_twi: TWI_OFF sent");
    }
    return ret;
}

int32_t rf_twi_soft_reset(void)
{
    int32_t ret = rf_twi_send_command(0x04);
    if (ret == ERR_OK) {
        s_driver.state = RF_TWI_STATE_IDLE;
        LOG_DEBUG("rf_twi: SOFT_RST sent");
    }
    return ret;
}

int32_t rf_twi_reset(void)
{
    int32_t ret = rf_twi_send_command(0x01);
    if (ret == ERR_OK) {
        s_driver.state = RF_TWI_STATE_ACTIVE;
        LOG_DEBUG("rf_twi: TWI_RST sent");
    }
    return ret;
}

/* ====================================================================
 * TWI Data Transactions
 * ==================================================================== */

int32_t rf_twi_write(uint8_t address, uint8_t data)
{
    uint8_t addr_byte;

    if (!s_driver.initialized) {
        return ERR_NOT_INIT;
    }

    /* Build address byte: W/R=1, A[5:0]=address[5:0], bit8=0 */
    /* Bit layout: [W/R=1][A5][A4][A3][A2][A1][A0][X=0] */
    addr_byte = 0x80 | ((address & 0x3F) << 1);

    /* Transmit address byte (cycles 1-8) */
    rf_twi_send_byte(addr_byte);

    /* Transmit data byte (cycles 9-16) */
    rf_twi_send_byte(data);

    /* Idle: CLK low, DATA low */
    RF_TWI_CLK_SET_LOW();
    RF_TWI_DATA_SET_LOW();

    return ERR_OK;
}

int32_t rf_twi_read(uint8_t address, uint8_t *out_byte)
{
    uint8_t addr_byte;

    if (!s_driver.initialized) {
        return ERR_NOT_INIT;
    }
    if (out_byte == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* Build address byte: W/R=0, A[5:0]=address[5:0], bit8=0 */
    /* Bit layout: [W/R=0][A5][A4][A3][A2][A1][A0][X=0] */
    addr_byte = 0x00 | ((address & 0x3F) << 1);

    /* Transmit address byte (cycles 1-8) -- W/R=0 means read */
    rf_twi_send_byte(addr_byte);

    /* Receive data byte (cycles 9-16) -- UM2005C drives DATA */
    *out_byte = rf_twi_recv_byte();

    /* Idle: CLK low, DATA output low */
    RF_TWI_CLK_SET_LOW();
    RF_TWI_DATA_SET_LOW();

    return ERR_OK;
}

int32_t rf_twi_write_buf(uint8_t address, const uint8_t *data, uint16_t length)
{
    int32_t ret;

    if (!s_driver.initialized) {
        return ERR_NOT_INIT;
    }
    if (data == NULL) {
        return ERR_INVALID_PARAM;
    }

    for (uint16_t i = 0; i < length; i++) {
        ret = rf_twi_write(address, data[i]);
        if (ret != ERR_OK) {
            LOG_ERROR("rf_twi: write_buf failed at byte %u, err=%ld", i, ret);
            return ret;
        }
    }

    return ERR_OK;
}

/* ====================================================================
 * High-Level RF Transmission
 * ==================================================================== */

/**
 * State machine for RF transmission:
 *
 * IDLE -> WAKE_RF (tSLP-TX ~1.2ms) -> TWI_ON (32 zeros)
 *    -> DATA_WRITE (14 bytes) -> TWI_OFF -> SLEEP (hold CLK low 15ms)
 *
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=14
 */
int32_t rf_twi_transmit(const uint8_t *packet)
{
    int32_t ret;

    if (!s_driver.initialized) {
        return ERR_NOT_INIT;
    }
    if (packet == NULL) {
        return ERR_INVALID_PARAM;
    }

    LOG_INFO("rf_twi: starting RF transmission (%u bytes)", RF_TWI_PACKET_SIZE);

    /* ----------------------------------------------------------------
     * Step 1: Ensure CLK is low to avoid spurious wake-up
     * ---------------------------------------------------------------- */
    RF_TWI_CLK_SET_LOW();
    RF_TWI_DATA_SET_LOW();

    /* ----------------------------------------------------------------
     * Step 2: Wake sequence -- CLK rising edge wakes chip
     * Then wait for XTAL startup + VCO calibration
     *
     * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=14 (state diagram)
     * tSLP-TX = 1.2ms typ (includes XTAL 400us + VCO 300us + margin)
     * ---------------------------------------------------------------- */
    /* Generate a rising edge on CLK to wake the UM2005C */
    RF_TWI_CLK_SET_HIGH();
    rf_twi_delay_us(10);            /* Hold high briefly */
    RF_TWI_CLK_SET_LOW();

    /* Wait for full wake-up sequence */
    rf_twi_delay_us(RF_TWI_WAKE_DELAY_US);   /* 1.2ms */

    /* ----------------------------------------------------------------
     * Step 3: Send TWI_ON -- 32 CLK cycles with DATA=0
     * Enters programming mode and resets TWI circuit
     *
     * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=12
     * ---------------------------------------------------------------- */
    ret = rf_twi_on();
    if (ret != ERR_OK) {
        LOG_ERROR("rf_twi: TWI_ON failed");
        s_driver.state = RF_TWI_STATE_ERROR;
        return ret;
    }

    /* Small delay after TWI_ON for internal stabilization */
    rf_twi_delay_us(200);

    /* ----------------------------------------------------------------
     * Step 4: Write packet bytes via TWI
     * Each byte is written as a separate TWI 16-cycle transaction
     * to address 0x00 (data register)
     *
     * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=12 (write transaction)
     * ---------------------------------------------------------------- */
    ret = rf_twi_write_buf(RF_TWI_ADDR_DEFAULT, packet, RF_TWI_PACKET_SIZE);
    if (ret != ERR_OK) {
        LOG_ERROR("rf_twi: packet write failed");
        s_driver.state = RF_TWI_STATE_ERROR;
        return ret;
    }

    /* ----------------------------------------------------------------
     * Step 5: Send TWI_OFF -- exits programming mode, starts TX
     *
     * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=12
     * After TWI_OFF, the chip enters TX mode and begins modulating
     * ---------------------------------------------------------------- */
    ret = rf_twi_off();
    if (ret != ERR_OK) {
        LOG_ERROR("rf_twi: TWI_OFF failed");
        s_driver.state = RF_TWI_STATE_ERROR;
        return ret;
    }

    /* ----------------------------------------------------------------
     * Step 6: Hold CLK low for >=15ms to ensure chip enters sleep
     *
     * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=14 (state diagram)
     * ---------------------------------------------------------------- */
    RF_TWI_CLK_SET_LOW();
    rf_twi_delay_ms(RF_TWI_SLEEP_HOLD_MS);   /* 15ms */

    s_driver.state = RF_TWI_STATE_IDLE;

    LOG_INFO("rf_twi: transmission complete, chip in sleep");
    return ERR_OK;
}

/* ====================================================================
 * LBD (Low Battery Detection)
 * ==================================================================== */

int32_t rf_twi_read_lbd(uint8_t *out_lbd)
{
    int32_t ret;

    if (!s_driver.initialized) {
        return ERR_NOT_INIT;
    }
    if (out_lbd == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* Ensure we're in active (programming) mode */
    if (s_driver.state != RF_TWI_STATE_ACTIVE) {
        /* If not active, send TWI_ON first */
        ret = rf_twi_on();
        if (ret != ERR_OK) {
            return ret;
        }
    }

    /* Read LBD register (address 0x3E) */
    /* Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=13 (LBD section) */
    ret = rf_twi_read(0x3E, out_lbd);
    if (ret != ERR_OK) {
        LOG_ERROR("rf_twi: LBD read failed");
        return ret;
    }

    LOG_INFO("rf_twi: LBD value = 0x%02X (Vbat ~ %.1fV)",
             *out_lbd, 2.0 + (*out_lbd) * 0.1);
    return ERR_OK;
}
