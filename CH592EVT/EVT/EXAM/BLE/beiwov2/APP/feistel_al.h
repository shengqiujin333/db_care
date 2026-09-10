#ifndef __FEISTEL_AL_H__
#define __FEISTEL_AL_H__

#include <stdint.h>
#include <stdbool.h>
bool decode_frame10(uint8_t in10[10], uint8_t uid_pick_out[4], int16_t *temp_out, uint16_t *hum_out);


#endif
