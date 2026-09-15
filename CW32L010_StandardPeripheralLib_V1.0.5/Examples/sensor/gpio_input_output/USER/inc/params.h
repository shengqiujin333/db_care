/*
 * params.h - 传感器阈值参数模型与非易失存储 (FR-108/201/205, FD-001 6.3)
 *
 * RAM 表示: 阈值换算为 x10 整数参与比较(避免逐样本 float)
 * NVM 表示: 顶层 128B Flash 页保存 6 个 binary32 LE + 版本 + CRC
 */
#ifndef __PARAMS_H
#define __PARAMS_H

#include <stdint.h>
#include <stdbool.h>
#include "fw_core.h"

/* 非易失记录常量 */
#define PARAMS_MAGIC                0x4442u  /* "DB" little-endian 记录 magic */
#define PARAMS_VERSION              0x01u
#define PARAMS_NVM_PAGE             511u     /* 顶层页, 128B */
#define PARAMS_NVM_ADDR             (0x0000FF80uL)  /* 511*128 */
#define PARAMS_REC_LEN              33u      /* magic2+ver1+txn4+float24+crc2 */

/* 参数索引(与 IC-001 2 节固定排序一致) */
#define PARAMS_IDX_TEMP_DROP        0
#define PARAMS_IDX_HUM_DROP         1
#define PARAMS_IDX_TEMP_LOW         2
#define PARAMS_IDX_HUM_LOW          3
#define PARAMS_IDX_TEMP_HIGH        4
#define PARAMS_IDX_HUM_HIGH         5
#define PARAMS_NUM                  6

/* 编译期安全默认值 (FD-001 6.3 / TD-001 7) */
#define PARAMS_DEF_TEMP_DROP_X10    20      /* 2.0 C */
#define PARAMS_DEF_HUM_DROP_X10     100     /* 10.0 %RH */
#define PARAMS_DEF_TEMP_LOW_X10     280     /* 28.0 C */
#define PARAMS_DEF_HUM_LOW_X10      300     /* 30.0 %RH */
#define PARAMS_DEF_TEMP_HIGH_X10    360     /* 36.0 C */
#define PARAMS_DEF_HUM_HIGH_X10     850     /* 85.0 %RH */

typedef struct {
    uint32_t transaction_id;      /* 最近成功提交的事务号(0=从未) */
    int16_t  temp_drop_x10;       /* 温度下降阈值 x10 */
    int16_t  hum_drop_x10;        /* 湿度下降阈值 x10 */
    int16_t  temp_low_x10;        /* 温度低阈值 x10 */
    int16_t  hum_low_x10;         /* 湿度低阈值 x10 */
    int16_t  temp_high_x10;       /* 温度高阈值 x10 */
    int16_t  hum_high_x10;        /* 湿度高阈值 x10 */
    bool     configured;          /* false=装载默认值(未配置) */
} params_t;

/* 访问器 */
uint32_t params_get_transaction_id(void);
int16_t  params_get_temp_drop_x10(void);
int16_t  params_get_hum_drop_x10(void);
int16_t  params_get_temp_low_x10(void);
int16_t  params_get_hum_low_x10(void);
int16_t  params_get_temp_high_x10(void);
int16_t  params_get_hum_high_x10(void);
bool     params_is_configured(void);

/* 上电装载: 校验通过装载 NVM 记录, 否则装载编译期默认值 */
void params_init(void);

/*
 * 提交新配置 (FR-106/107/108):
 *   frame[35] 已通过 optcfg_parse_and_validate;
 *   txn 为帧内 transaction_id.
 *   完整校验在调用方完成; 本函数做 x10 换算 + 原子写 Flash + 更新 RAM.
 *   幂等重放(与上次 txn 相同): 不写 Flash, 返回 true.
 *   返回 false 仅当 Flash 写失败(保持 RAM 旧配置).
 */
bool params_commit(const float v[6], uint32_t txn);

#endif /* __PARAMS_H */
