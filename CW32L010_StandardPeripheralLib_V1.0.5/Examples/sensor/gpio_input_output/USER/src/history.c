/*
 * history.c - 20 项温湿度历史环形缓冲 (FR-204, IC-001 4 节)
 */
#include "history.h"
#include <string.h>

static history_entry_t s_buf[HISTORY_DEPTH];
static uint8_t s_head;      /* 下一写入位置 */
static uint8_t s_count;     /* 已存项数 0..HISTORY_DEPTH */

void history_init(void)
{
    memset(s_buf, 0, sizeof(s_buf));
    s_head  = 0;
    s_count = 0;
}

void history_push(int16_t temp_x10, uint16_t hum_x10)
{
    s_buf[s_head].temp_x10 = temp_x10;
    s_buf[s_head].hum_x10  = hum_x10;
    s_head = (uint8_t)((s_head + 1) % HISTORY_DEPTH);
    if (s_count < HISTORY_DEPTH) s_count++;
}

uint8_t history_count(void)
{
    return s_count;
}

/* idx 0 = 最旧 (当缓冲满时 = head 指向的位置) */
bool history_get(uint8_t idx, history_entry_t *out)
{
    if (idx >= s_count) return false;
    uint8_t oldest = (uint8_t)((s_head + HISTORY_DEPTH - s_count) % HISTORY_DEPTH);
    uint8_t pos = (uint8_t)((oldest + idx) % HISTORY_DEPTH);
    if (out) *out = s_buf[pos];
    return true;
}

bool history_any_drop(int16_t cur_temp_x10, uint16_t cur_hum_x10,
                      int16_t temp_drop_x10, int16_t hum_drop_x10)
{
    for (uint8_t i = 0; i < s_count; i++) {
        history_entry_t e;
        if (!history_get(i, &e)) break;
        if ((int32_t)e.temp_x10 - (int32_t)cur_temp_x10 >= (int32_t)temp_drop_x10) return true;
        if ((int32_t)e.hum_x10  - (int32_t)cur_hum_x10  >= (int32_t)hum_drop_x10) return true;
    }
    return false;
}
