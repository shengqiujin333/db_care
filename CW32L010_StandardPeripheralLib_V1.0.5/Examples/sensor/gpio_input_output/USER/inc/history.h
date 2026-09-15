/*
 * history.h - 20 项温湿度历史环形缓冲 (FR-204, IC-001 4 节)
 *
 * 每项 4B: int16 temp_x10 + uint16 hum_x10, 共 80B; 仅成功测量写入。
 */
#ifndef __HISTORY_H
#define __HISTORY_H

#include <stdint.h>
#include <stdbool.h>

#define HISTORY_DEPTH   20

typedef struct {
    int16_t  temp_x10;   /* 温度 x10, 有符号 0.1 C */
    uint16_t hum_x10;    /* 湿度 x10, 无符号 0.1 %RH */
} history_entry_t;

void  history_init(void);
void  history_push(int16_t temp_x10, uint16_t hum_x10);
uint8_t history_count(void);
bool  history_get(uint8_t idx, history_entry_t *out);  /* idx 0=最旧 */

/*
 * 下降告警纯判定 (FR-202): 最近 20 项中任一样本与当前值之差满足阈值。
 *   (sample.temp - cur_temp) >= temp_drop_x10 或
 *   (sample.hum  - cur_hum)  >= hum_drop_x10
 */
bool history_any_drop(int16_t cur_temp_x10, uint16_t cur_hum_x10,
                      int16_t temp_drop_x10, int16_t hum_drop_x10);

#endif /* __HISTORY_H */
