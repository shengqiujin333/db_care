#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/*================== ??(??/????????) ==================*/
// ??????(16B)——????????
static const uint8_t K_SYS[16] = {
  0xB1,0x13,0x27,0x58,0x9A,0xCD,0xEF,0x01,0x22,0x44,0x66,0x88,0xAA,0xCC,0xEE,0x00
};

// ? UID10 ?? 4 ???? uid_pick
static const uint8_t UID_IDX[4] = {0,1,2,3};

// 10 ???????:???? P,???? P^-1;?/?????????(??)
static const uint8_t PERM[10]  = {3, 8, 1, 5, 0, 9, 4, 7, 2, 6};
static const uint8_t IPERM[10] = {4, 2, 8, 0, 6, 3, 9, 7, 1, 5};

// ??(?? 6/8/10),??????????
#define ROUNDS 8

/*================== ??:CRC16-CCITT ==================*/
static uint16_t crc16_ccitt(const uint8_t *p, size_t n){
  uint16_t crc = 0xFFFF;
  for(size_t i=0;i<n;i++){
    crc ^= (uint16_t)p[i] << 8;
    for(int b=0;b<8;b++)
      crc = (crc & 0x8000) ? (uint16_t)((crc<<1) ^ 0x1021) : (uint16_t)(crc<<1);
  }
  return crc;
}

/*================== ?? S-Box(4bit) ==================*/
// ??????? 4bit SBox;?????(Feistel ???? SBox)
static const uint8_t S4[16] = {
  0xC,0x5,0x6,0xB,0x9,0x0,0xA,0xD,0x3,0xE,0xF,0x8,0x4,0x7,0x1,0x2
};
static inline uint8_t sbox_byte(uint8_t x){
  return (uint8_t)((S4[x>>4]<<4) | S4[x & 0xF]);
}

/*================== ???? ==================*/
static void permute10(const uint8_t in[10], uint8_t out[10], const uint8_t p[10]){
  for(int i=0;i<10;i++) out[i] = in[p[i]];
}

/*================== ?????(??) ==================*/
/* ?? ROUNDS ? 5 ??????????,???? KDF:
   - ? K_SYS ?? 16 ?????
   - ??? 5 ?????????,?????????
*/
static void subkeys_gen(uint8_t sk[ROUNDS][5]){
  uint8_t pool[16];
  memcpy(pool, K_SYS, 16);
  for(int r=0;r<ROUNDS;r++){
    // ? 5 ?? + ?????
    for(int i=0;i<5;i++){
      sk[r][i] = (uint8_t)(pool[(r+i) & 15] ^ (uint8_t)(0xA5 ^ (r*13 + i*7)));
    }
    // ????:? pool ??????? 1 ??
    uint8_t t = pool[0];
    memmove(pool, pool+1, 15);
    pool[15] = t ^ (uint8_t)(0x5A ^ r);
  }
}

/*================== ??? F:?? 5 ?? R,?? 5 ?? ==================*/
/* ????:???,?? XOR??????4bit SBox??? */
static void F_func(const uint8_t R[5], const uint8_t rk[5], uint8_t out[5], uint8_t round_idx){
  uint8_t x[5];
  // 1) ??????
  for(int i=0;i<5;i++) x[i] = (uint8_t)(R[i] ^ rk[i]);
  // 2) ???:??? 4bit SBox
  for(int i=0;i<5;i++) x[i] = sbox_byte(x[i]);
  // 3) ????:??????????+??
  //    ?????? 40 ????,???? (round_idx+1) ?
  uint64_t acc = ((uint64_t)x[0]) | ((uint64_t)x[1]<<8) | ((uint64_t)x[2]<<16) | ((uint64_t)x[3]<<24) | ((uint64_t)x[4]<<32);
  uint8_t rot = (uint8_t)((round_idx + 1) % 40);
  uint64_t y = (acc << rot) | (acc >> (40 - rot));
  out[0] = (uint8_t)(y & 0xFF);
  out[1] = (uint8_t)((y>>8) & 0xFF);
  out[2] = (uint8_t)((y>>16)& 0xFF);
  out[3] = (uint8_t)((y>>24)& 0xFF);
  out[4] = (uint8_t)((y>>32)& 0xFF);
  // 4) ?????????
  out[0] ^= (uint8_t)(out[4] + 0x3D);
  out[2] ^= (uint8_t)(out[0] + out[1]);
}

/*================== 10 ?? Feistel ?/?? ==================*/
/* ?????? 10 ??(????) */
static void feistel10_encrypt(uint8_t block[10]){
  uint8_t L[5], R[5], tmp[5], rk[ROUNDS][5];
  subkeys_gen(rk);

  // ????
  uint8_t buf[10]; permute10(block, buf, PERM);
  memcpy(L, buf, 5);
  memcpy(R, buf+5, 5);

  for(int r=0;r<ROUNDS;r++){
    F_func(R, rk[r], tmp, (uint8_t)r);
    // L = L ^ F(R, rk)
    for(int i=0;i<5;i++) L[i] ^= tmp[i];
    // ?? L/R
    uint8_t t5[5]; memcpy(t5, L, 5); memcpy(L, R, 5); memcpy(R, t5, 5);
  }
  // ?????????,? (R||L) ??
  for(int i=0;i<5;i++){ buf[i] = R[i]; buf[5+i] = L[i]; }

  // ????
  uint8_t out[10]; permute10(buf, out, PERM);
  memcpy(block, out, 10);
}


/*================== ??/??:???? 10B ???? ==================*/
/* ??:uid_pick(4) | temp_le(2) | hum_le(2) | crc16_le(2)  ?  ?? ? 10B ?? */
void encode_frame10(uint8_t uid10[10], int16_t temp, uint16_t hum, uint8_t out10[10]){
  uint8_t p[10]; size_t k=0;
  // uid_pick
  for(int i=0;i<4;i++) p[k++] = uid10[UID_IDX[i]];
  // ??(??)
  p[k++] = (uint8_t)(temp & 0xFF);
  p[k++] = (uint8_t)((temp>>8) & 0xFF);
  p[k++] = (uint8_t)(hum & 0xFF);
  p[k++] = (uint8_t)((hum>>8) & 0xFF);
  // CRC16(?? 8 ??)
  uint16_t c = crc16_ccitt(p, 8);
  p[k++] = (uint8_t)(c & 0xFF);
  p[k++] = (uint8_t)((c>>8) & 0xFF);

  // 10B Feistel ??
  memcpy(out10, p, 10);
  feistel10_encrypt(out10);
}


/*================== ???:?? & ??? UID4 ? HEX ==================*/
static void print_hex(const char* title, const uint8_t* b, size_t n){
  printf("%s", title);
  for(size_t i=0;i<n;i++) printf("%02X ", b[i]); printf("\n");
}
static void uid4_hex(const uint8_t u4[4], char out9[9]){
  static const char hex[]="0123456789ABCDEF";
  for(int i=0;i<4;i++){ out9[2*i]=hex[(u4[i]>>4)&0xF]; out9[2*i+1]=hex[u4[i]&0xF]; }
  out9[8]='\0';
}


