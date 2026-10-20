/* Architecture probe only: real sensor encoder, gateway decoder and BLE packer.
 * BLE record assembly is an explicit adapter, not the MCU event loop.
 * No radio, AES backend, Android runtime or server is exercised. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "encrytogate.h"
#include "feistel_al.h"
#include "bleencrypt.h"

int main(void) {
    uint8_t uid[10] = {0x12,0x34,0x56,0x78}, gw[6] = {1,2,3,4,5,6};
    const int16_t temps[] = {-400,-1,0,350,351,1250};
    const uint16_t hums[] = {0,1,500,999,1000,100};
    unsigned failures = 0;
    for (unsigned i=0;i<sizeof temps/sizeof temps[0];++i) {
        uint8_t rf[10], id[4], record[8], plain[32];
        int16_t t=0; uint16_t h=0;
        encode_frame10(uid,temps[i],hums[i],rf);
        int ok=decode_frame10(rf,id,&t,&h);
        memcpy(record,id,4);
        record[4]=(uint8_t)(h>>8); record[5]=(uint8_t)h;
        record[6]=(uint8_t)((uint16_t)t>>8); record[7]=(uint8_t)t;
        size_t n=build_padded_frame_blocks(gw,1,record,plain,sizeof plain);
        ok=ok && t==temps[i] && h==hums[i] && !memcmp(id,uid,4) && n==32;
        failures+=!ok;
        printf("VECTOR %u input_t=%d input_h=%u decoded_t=%d decoded_h=%u ok=%d plain=",i,temps[i],hums[i],t,h,ok);
        for(size_t j=0;j<n;++j) printf("%02X",plain[j]);
        puts("");
        if(i==4) {
            uint8_t again[32];
            size_t m=build_padded_frame_blocks(gw,1,record,again,sizeof again);
            int same=m==n && !memcmp(plain,again,n);
            printf("FRESHNESS same_value_new_report_vs_cache_read_identical=%d\n",same);
            failures+=!same;
        }
    }
    /* Safe packer-only bounds experiment; never run MCU cache indexing. */
    uint8_t records[80]={0}, frame[500];
    const uint8_t counts[]={1,2,9,10};
    for(unsigned i=0;i<sizeof counts;++i) {
        size_t n=build_padded_frame_blocks(gw,counts[i],records,frame,sizeof frame);
        printf("CAPACITY count=%u packed_bytes=%zu gatt_attribute_bytes=160 fits=%d\n",counts[i],n,n<=160);
    }
    printf("SUMMARY vectors=6 failures=%u\n",failures);
    return failures?1:0;
}
