/*
 * params.c - 传感器阈值参数模型与非易失存储 (FR-108/201/205, FD-001 6.3)
 *
 * 纯逻辑(校验/换算/记录编解码)在 fw_core.c(宿主机 L0 可测);
 * 本文件仅保留 Flash 访问与 RAM 模型。
 */
#include "params.h"
#include "fw_core.h"
#include <string.h>
#include "cw32l010_flash.h"

static params_t s_params;

/* ------------------------------------------------------------------ */
/* Flash 访问 (目标硬件)                                                */
/* ------------------------------------------------------------------ */
static void flash_read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    const uint8_t *p = (const uint8_t *)addr;
    memcpy(buf, p, len);
}

static bool flash_write_record(const uint8_t rec[PARAMS_REC_LEN])
{
    if (FLASH_UnlockPage(PARAMS_NVM_PAGE) != 0x00u) return false;
    if (FLASH_ErasePage(PARAMS_NVM_PAGE) != 0x00u) return false;
    if (FLASH_WriteBytes(PARAMS_NVM_ADDR, (uint8_t *)rec, PARAMS_REC_LEN) != 0x00u) {
        FLASH_LockPage(PARAMS_NVM_PAGE);
        return false;
    }
    FLASH_LockPage(PARAMS_NVM_PAGE);
    return true;
}

static void params_apply(const float v[6], uint32_t txn)
{
    int16_t x10[PARAMS_NUM];
    params_floats_to_x10(v, x10);
    s_params.transaction_id = txn;
    s_params.temp_drop_x10  = x10[PARAMS_IDX_TEMP_DROP];
    s_params.hum_drop_x10   = x10[PARAMS_IDX_HUM_DROP];
    s_params.temp_low_x10   = x10[PARAMS_IDX_TEMP_LOW];
    s_params.hum_low_x10    = x10[PARAMS_IDX_HUM_LOW];
    s_params.temp_high_x10  = x10[PARAMS_IDX_TEMP_HIGH];
    s_params.hum_high_x10   = x10[PARAMS_IDX_HUM_HIGH];
    s_params.configured     = true;
}

static void params_load_defaults(void)
{
    s_params.transaction_id   = 0;
    s_params.temp_drop_x10    = PARAMS_DEF_TEMP_DROP_X10;
    s_params.hum_drop_x10     = PARAMS_DEF_HUM_DROP_X10;
    s_params.temp_low_x10     = PARAMS_DEF_TEMP_LOW_X10;
    s_params.hum_low_x10      = PARAMS_DEF_HUM_LOW_X10;
    s_params.temp_high_x10    = PARAMS_DEF_TEMP_HIGH_X10;
    s_params.hum_high_x10     = PARAMS_DEF_HUM_HIGH_X10;
    s_params.configured       = false;
}

/* ------------------------------------------------------------------ */
/* 对外接口                                                             */
/* ------------------------------------------------------------------ */
void params_init(void)
{
    uint8_t rec[PARAMS_REC_LEN];
    float v[PARAMS_NUM];
    uint32_t txn = 0;

    params_load_defaults();
    flash_read(PARAMS_NVM_ADDR, rec, PARAMS_REC_LEN);
    if (params_parse_record(rec, v, &txn) && params_validate_floats(v)) {
        params_apply(v, txn);
    }
}

bool params_commit(const float v[6], uint32_t txn)
{
    if (!params_validate_floats(v)) return false;   /* 调用方已校验; 双保险 */

    /* 幂等重放: 与上次成功 txn 相同 → 不写 Flash (FR-107) */
    if (s_params.configured && s_params.transaction_id == txn) {
        return true;
    }

    uint8_t rec[PARAMS_REC_LEN];
    params_build_record(v, txn, rec);

    if (!flash_write_record(rec)) return false;     /* Flash 写失败: 保持 RAM 旧配置 */

    params_apply(v, txn);
    return true;
}

uint32_t params_get_transaction_id(void) { return s_params.transaction_id; }
int16_t  params_get_temp_drop_x10(void)  { return s_params.temp_drop_x10; }
int16_t  params_get_hum_drop_x10(void)   { return s_params.hum_drop_x10; }
int16_t  params_get_temp_low_x10(void)   { return s_params.temp_low_x10; }
int16_t  params_get_hum_low_x10(void)    { return s_params.hum_low_x10; }
int16_t  params_get_temp_high_x10(void)  { return s_params.temp_high_x10; }
int16_t  params_get_hum_high_x10(void)   { return s_params.hum_high_x10; }
bool     params_is_configured(void)      { return s_params.configured; }
