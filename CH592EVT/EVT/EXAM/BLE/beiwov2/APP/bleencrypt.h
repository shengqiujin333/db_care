#ifndef __BLEENCRYPT_H__
#define __BLEENCRYPT_H__

#include <stdint.h>
#include <string.h>
#include <stdbool.h>



#define FRAME_BLOCK_SIZE   16
#define GWID_LEN           6
#define DEV_PAYLOAD_LEN    8
#define MAX_DEV_COUNT      255


size_t build_padded_frame_blocks(uint8_t gwid6[GWID_LEN],
                                 uint8_t dev_count,
                                 uint8_t *payloads, /* len = dev_count*8 */
                                 uint8_t *out_buf,
                                 size_t out_cap);

#endif
