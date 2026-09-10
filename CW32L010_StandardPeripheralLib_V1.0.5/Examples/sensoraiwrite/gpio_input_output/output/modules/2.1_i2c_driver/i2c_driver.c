/**
 * @file    i2c_driver.c
 * @brief   Bit-bang I2C Master Driver Implementation
 * @author  Auto-generated (EmbedDevOps)
 * @date    2025-07-17
 *
 * Implements standard I2C master protocol (Standard-mode, 100 kHz) via
 * GPIO bit-banging on PA03(SDA)/PA04(SCL).
 *
 * == Pin Driving Strategy ==
 * CW32L010 GPIO has a dedicated OPENDRAIN register (offset 0x04).
 * We configure the pins as output push-pull, then set the OPENDRAIN bit
 * to enable open-drain behavior:
 *   - Write 1 to BSRR[pin] (BS bit) -> Hi-Z, pulled HIGH by external R
 *   - Write 1 to BSRR[pin+16] (BR bit) -> drives pin LOW
 *
 * This is cleaner than toggling DIR mode and avoids output glitches.
 * [cw32l010.h#L1188-L1201]
 *
 * == APIs Used ==
 * - GPIO_InitTypeDef, GPIO_PIN_*, GPIO_MODE_OUTPUT_PP
 *   [inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h]
 * - CW_GPIOA, CW_SYSCTRL, __SYSCTRL_GPIOA_CLK_ENABLE()
 *   [inputs/mcu_sdk/Libraries/inc/cw32l010.h]
 *   [inputs/mcu_sdk/Libraries/inc/cw32l010_sysctrl.h]
 * - SysTickDelay() for millisecond-level delays
 *   [inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h]
 *
 * == AHT10 Protocol Summary (for reference) ==
 *   Address: 0x38 (7-bit) -> 0x70 as write byte, 0x71 as read byte
 *   Trigger: 0xAC 0x33 0x00 -> wait >=75 ms -> read 6 bytes
 *   Response: [hum_msb][hum_lsb][hum_xlsb][temp_msb][temp_lsb][temp_xlsb]
 *   (See protocol_notes.md for full details)
 *
 * == AF Verification ==
 * Per cw32l010_gpio.h#L352-L368:
 *   PA03 AF0-7 = GPIO, UART2_TXD, LPTIM_CH1, SPI1_MISO, BTIM1_ETR,
 *                IR_OUT, GTIM_CH4, ATIM_CH3  (NO I2C_SDA)
 *   PA04 AF0-7 = GPIO, UART2_RXD, LPTIM_CH2, SPI1_MOSI, MCO_OUT,
 *                VC2_OUT, GTIM1_CH3, ATIM_CH1N  (NO I2C_SCL)
 * Therefore bit-bang is required on these pins.
 */

/* ========================================================================== */
/* Includes                                                                   */
/* ========================================================================== */
#include "i2c_driver.h"

/* SDK includes */
#include "cw32l010.h"            /* CW_GPIOA, CW_SYSCTRL, GPIO_PIN_x */
#include "cw32l010_gpio.h"       /* GPIO_InitTypeDef, GPIO_MODE_* */
#include "cw32l010_sysctrl.h"    /* __SYSCTRL_GPIOA_CLK_ENABLE */
#include "cw32l010_systick.h"    /* SysTickDelay */

/* ========================================================================== */
/* Local Defines                                                              */
/* ========================================================================== */

/** Set SCL HIGH (output Hi-Z, pulled up by external R) */
#define SCL_HIGH()       (I2C_SCL_PORT->BSRR = I2C_SCL_PIN)
/** Set SCL LOW (output drive low) */
#define SCL_LOW()        (I2C_SCL_PORT->BSRR = (uint32_t)(I2C_SCL_PIN) << 16)

/** Set SDA HIGH (output Hi-Z, pulled up by external R) */
#define SDA_HIGH()       (I2C_SDA_PORT->BSRR = I2C_SDA_PIN)
/** Set SDA LOW (output drive low) */
#define SDA_LOW()        (I2C_SDA_PORT->BSRR = (uint32_t)(I2C_SDA_PIN) << 16)

/* ========================================================================== */
/* Local State                                                                */
/* ========================================================================== */

/** True after i2c_drv_init() succeeds */
static uint8_t s_initialized = 0;

/* ========================================================================== */
/* Local Helper: Microsecond Delay                                            */
/* ========================================================================== */

/**
 * @brief  Blocking delay in microseconds (approximate)
 * @param  us  Microseconds to wait
 *
 * Uses SysTickDelay() which counts at 1 ms resolution minimum.
 * For sub-millisecond delays we use a simple loop calibrated for 48 MHz.
 *
 * At 48 MHz HCLK, one iteration of the loop (NOP + branch) ~ 3 cycles ~ 62.5 ns.
 * To delay N us: loops = (N * 48) / 3 ~ N * 16
 */
static void delay_us(uint32_t us)
{
    /* Use SysTickDelay for delays >= 1 ms */
    if (us >= 1000) {
        SysTickDelay(us / 1000);
        return;
    }

    /* Tight loop for sub-ms delays */
    uint32_t loops = us * 16U;
    while (loops--) {
        __NOP();
    }
}

/* ========================================================================== */
/* Local Helper: Pin Configuration                                            */
/* ========================================================================== */

/**
 * @brief  Configure a GPIO pin as open-drain output
 *
 * Strategy: Set pin as output push-pull, then enable OPENDRAIN bit.
 *   - BSRR bit set (pin)      -> output Hi-Z (pull-up takes HIGH)
 *   - BSRR bit set (pin+16)   -> output drive LOW
 *
 * @param  port  GPIO port (CW_GPIOA or CW_GPIOB)
 * @param  pin   GPIO pin mask (GPIO_PIN_x)
 */
static void pin_init_od(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef init;

    /* Enable GPIO clock */
    if (port == CW_GPIOA) {
        __SYSCTRL_GPIOA_CLK_ENABLE();
    }

    init.Pins = pin;
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.IT   = GPIO_IT_NONE;

    GPIO_Init(port, &init);

    /* Enable open-drain mode */
    port->OPENDRAIN |= pin;

    /* Start with pin HIGH (released -- Hi-Z, pulled up) */
    port->BSRR = pin;
}

/**
 * @brief  Read the current level of a pin
 * @param  port  GPIO port
 * @param  pin   GPIO pin mask
 * @retval 0 if LOW, non-zero if HIGH
 */
static uint8_t pin_read(GPIO_TypeDef *port, uint16_t pin)
{
    return (port->IDR & pin) ? 1 : 0;
}

/* ========================================================================== */
/* Local Helper: SCL / SDA Low-Level Operations                               */
/* ========================================================================== */

static inline void scl_low(void)
{
    SCL_LOW();
}

static inline void scl_high(void)
{
    SCL_HIGH();
}

static inline void sda_low(void)
{
    SDA_LOW();
}

static inline void sda_high(void)
{
    SDA_HIGH();
}

static inline uint8_t sda_read(void)
{
    return pin_read(I2C_SDA_PORT, I2C_SDA_PIN);
}

static inline uint8_t scl_read(void)
{
    return pin_read(I2C_SCL_PORT, I2C_SCL_PIN);
}

/* ========================================================================== */
/* Local Helper: I2C Bus Primitives                                           */
/* ========================================================================== */

/**
 * @brief  Generate I2C START condition
 *         SDA transitions LOW while SCL is HIGH
 */
static void i2c_start(void)
{
    /* Ensure both lines are released (HIGH) */
    sda_high();
    delay_us(I2C_SDA_SETUP_US);
    scl_high();
    delay_us(I2C_HALF_PERIOD_US);

    /* SDA falling while SCL HIGH */
    sda_low();
    delay_us(I2C_SDA_HOLD_US);

    /* SCL LOW for data transmission */
    scl_low();
    delay_us(I2C_HALF_PERIOD_US);
}

/**
 * @brief  Generate I2C STOP condition
 *         SDA transitions HIGH while SCL is HIGH
 */
static void i2c_stop(void)
{
    /* SDA LOW, SCL LOW */
    sda_low();
    delay_us(I2C_SDA_SETUP_US);

    /* SCL HIGH */
    scl_high();
    delay_us(I2C_HALF_PERIOD_US);

    /* SDA rising while SCL HIGH */
    sda_high();
    delay_us(I2C_SDA_HOLD_US);
}

/**
 * @brief  Wait for SCL to be released by slave (clock stretching)
 * @retval I2C_DRV_OK if SCL went high within timeout
 * @retval I2C_DRV_ERR_TIMEOUT if slave held SCL low too long
 */
static i2c_drv_error_t i2c_wait_scl(void)
{
    uint32_t timeout = I2C_BUS_TIMEOUT_MS * 1000;  /* convert to us */

    /* SCL was driven LOW by master; wait for slave to release */
    scl_high();
    while (scl_read() == 0) {
        delay_us(1);
        if (--timeout == 0) {
            return I2C_DRV_ERR_TIMEOUT;
        }
    }
    return I2C_DRV_OK;
}

/**
 * @brief  Transmit one byte on the I2C bus
 * @param  data  Byte to send
 * @retval I2C_DRV_OK on success
 * @retval I2C_DRV_ERR_NACK if slave NACKs
 * @retval I2C_DRV_ERR_TIMEOUT on clock stretch timeout
 */
static i2c_drv_error_t i2c_send_byte(uint8_t data)
{
    i2c_drv_error_t err;

    /* Send 8 bits, MSB first */
    for (int32_t i = 7; i >= 0; i--) {
        /* Set SDA */
        if (data & (1U << i)) {
            sda_high();
        } else {
            sda_low();
        }
        delay_us(I2C_SDA_SETUP_US);

        /* Clock high -> slave samples */
        err = i2c_wait_scl();
        if (err != I2C_DRV_OK) {
            return err;
        }
        delay_us(I2C_SDA_HOLD_US);

        /* Clock low */
        scl_low();
        delay_us(I2C_HALF_PERIOD_US);
    }

    /* Release SDA for ACK/NACK */
    sda_high();
    delay_us(I2C_SDA_SETUP_US);

    /* Clock 9th pulse */
    err = i2c_wait_scl();
    if (err != I2C_DRV_OK) {
        return err;
    }

    /* Read ACK bit */
    uint8_t ack = sda_read();
    delay_us(I2C_SDA_HOLD_US);

    /* Clock low */
    scl_low();
    delay_us(I2C_HALF_PERIOD_US);

    return (ack == 0) ? I2C_DRV_OK : I2C_DRV_ERR_NACK;
}

/**
 * @brief  Receive one byte from the I2C bus
 * @param  ack  1 = send ACK (more bytes expected), 0 = send NACK (last byte)
 * @return Received byte
 */
static i2c_drv_error_t i2c_recv_byte(uint8_t ack, uint8_t *out_data)
{
    uint8_t data = 0;

    if (!out_data) return I2C_DRV_ERR_PARAM;

    /* Release SDA for slave to drive */
    sda_high();
    delay_us(I2C_SDA_SETUP_US);

    /* Receive 8 bits, MSB first */
    for (int32_t i = 7; i >= 0; i--) {
        i2c_drv_error_t err;
        /* Clock high */
        err = i2c_wait_scl();
        if (err != I2C_DRV_OK) {
            return err;  /* Timeout during receive */
        }
        delay_us(I2C_SDA_HOLD_US);

        /* Read SDA */
        if (sda_read()) {
            data |= (1U << i);
        }

        /* Clock low */
        scl_low();
        delay_us(I2C_HALF_PERIOD_US);
    }

    /* Send ACK or NACK */
    if (ack) {
        sda_low();   /* ACK: drive SDA low */
    } else {
        sda_high();  /* NACK: release SDA */
    }
    delay_us(I2C_SDA_SETUP_US);

    /* Clock 9th pulse: ignore timeout on ack phase */
    (void)i2c_wait_scl();
    delay_us(I2C_SDA_HOLD_US);

    /* Clock low */
    scl_low();
    delay_us(I2C_HALF_PERIOD_US);

    *out_data = data;
    return I2C_DRV_OK;
}

/* ========================================================================== */
/* Public API Implementation                                                  */
/* ========================================================================== */

i2c_drv_error_t i2c_drv_init(void)
{
    /* Ensure GPIOA clock is enabled */
    __SYSCTRL_GPIOA_CLK_ENABLE();

    /* Initialize both pins as open-drain (both start HIGH = released) */
    pin_init_od(I2C_SCL_PORT, I2C_SCL_PIN);
    pin_init_od(I2C_SDA_PORT, I2C_SDA_PIN);

    /* Ensure bus is in idle state (SCL HIGH, SDA HIGH) */
    scl_high();
    sda_high();
    delay_us(I2C_HALF_PERIOD_US * 2);

    s_initialized = 1;
    return I2C_DRV_OK;
}

void i2c_drv_deinit(void)
{
    s_initialized = 0;

    /* Disable open-drain, set to input (Hi-Z) */
    I2C_SCL_PORT->OPENDRAIN &= ~I2C_SCL_PIN;
    I2C_SCL_PORT->DIR &= ~I2C_SCL_PIN;

    I2C_SDA_PORT->OPENDRAIN &= ~I2C_SDA_PIN;
    I2C_SDA_PORT->DIR &= ~I2C_SDA_PIN;
}

i2c_drv_error_t i2c_drv_master_write(uint8_t slave_addr,
                                     const uint8_t *data,
                                     size_t len)
{
    i2c_drv_error_t err;
    uint8_t addr_byte;

    if (!s_initialized) return I2C_DRV_ERR_BUSY;
    if (!data || len == 0) return I2C_DRV_ERR_PARAM;

    /* START */
    i2c_start();

    /* Send slave address + W (bit 0 = 0) */
    addr_byte = (uint8_t)((uint32_t)slave_addr << 1) | 0x00;
    err = i2c_send_byte(addr_byte);
    if (err != I2C_DRV_OK) {
        i2c_stop();
        return err;
    }

    /* Send data bytes */
    for (size_t i = 0; i < len; i++) {
        err = i2c_send_byte(data[i]);
        if (err != I2C_DRV_OK) {
            i2c_stop();
            return err;
        }
    }

    /* STOP */
    i2c_stop();
    return I2C_DRV_OK;
}

i2c_drv_error_t i2c_drv_master_read(uint8_t slave_addr,
                                    uint8_t *data,
                                    size_t len)
{
    i2c_drv_error_t err;
    uint8_t addr_byte;

    if (!s_initialized) return I2C_DRV_ERR_BUSY;
    if (!data || len == 0) return I2C_DRV_ERR_PARAM;

    /* START */
    i2c_start();

    /* Send slave address + R (bit 0 = 1) */
    addr_byte = (uint8_t)((uint32_t)slave_addr << 1) | 0x01;
    err = i2c_send_byte(addr_byte);
    if (err != I2C_DRV_OK) {
        i2c_stop();
        return err;
    }

    /* Receive data bytes */
    for (size_t i = 0; i < len; i++) {
        /* ACK for all bytes except the last */
        uint8_t send_ack = (i < (len - 1)) ? 1 : 0;
        err = i2c_recv_byte(send_ack, &data[i]);
        if (err != I2C_DRV_OK) {
            i2c_stop();
            return err;
        }
    }

    /* STOP */
    i2c_stop();
    return I2C_DRV_OK;
}

i2c_drv_error_t i2c_drv_master_write_read(uint8_t slave_addr,
                                          const uint8_t *wdata,
                                          size_t wlen,
                                          uint8_t *rdata,
                                          size_t rlen)
{
    i2c_drv_error_t err;
    uint8_t addr_byte;

    if (!s_initialized) return I2C_DRV_ERR_BUSY;
    if (!wdata || wlen == 0 || !rdata || rlen == 0) return I2C_DRV_ERR_PARAM;

    /* START */
    i2c_start();

    /* Send slave address + W */
    addr_byte = (uint8_t)((uint32_t)slave_addr << 1) | 0x00;
    err = i2c_send_byte(addr_byte);
    if (err != I2C_DRV_OK) {
        i2c_stop();
        return err;
    }

    /* Send write data */
    for (size_t i = 0; i < wlen; i++) {
        err = i2c_send_byte(wdata[i]);
        if (err != I2C_DRV_OK) {
            i2c_stop();
            return err;
        }
    }

    /* REPEATED START */
    i2c_start();

    /* Send slave address + R */
    addr_byte = (uint8_t)((uint32_t)slave_addr << 1) | 0x01;
    err = i2c_send_byte(addr_byte);
    if (err != I2C_DRV_OK) {
        i2c_stop();
        return err;
    }

    /* Read data */
    for (size_t i = 0; i < rlen; i++) {
        uint8_t send_ack = (i < (rlen - 1)) ? 1 : 0;
        err = i2c_recv_byte(send_ack, &rdata[i]);
        if (err != I2C_DRV_OK) {
            i2c_stop();
            return err;
        }
    }

    /* STOP */
    i2c_stop();
    return I2C_DRV_OK;
}

bool i2c_drv_probe(uint8_t slave_addr)
{
    i2c_drv_error_t err;
    uint8_t addr_byte;

    if (!s_initialized) return false;

    i2c_start();
    addr_byte = (uint8_t)((uint32_t)slave_addr << 1) | 0x00;
    err = i2c_send_byte(addr_byte);
    i2c_stop();

    return (err == I2C_DRV_OK);
}

void i2c_drv_bus_clear(void)
{
    if (!s_initialized) return;

    /* Generate up to 9 clock pulses while monitoring SDA */
    for (uint32_t i = 0; i < I2C_BUS_CLEAR_PULSES; i++) {
        /* SCL high */
        i2c_wait_scl();
        delay_us(I2C_HALF_PERIOD_US);

        /* Check if SDA is released (HIGH) */
        if (sda_read()) {
            /* Bus is free, send STOP */
            break;
        }

        /* SCL low */
        scl_low();
        delay_us(I2C_HALF_PERIOD_US);
    }

    /* Send STOP to reset bus state */
    i2c_stop();
}

const char *i2c_drv_strerror(i2c_drv_error_t err)
{
    switch (err) {
        case I2C_DRV_OK:          return "OK";
        case I2C_DRV_ERR_NACK:    return "NACK received";
        case I2C_DRV_ERR_TIMEOUT: return "Bus timeout (SCL held low)";
        case I2C_DRV_ERR_ARBLOST: return "Arbitration lost";
        case I2C_DRV_ERR_BUSY:    return "Driver busy or not initialized";
        case I2C_DRV_ERR_PARAM:   return "Invalid parameter";
        default:                  return "Unknown error";
    }
}
