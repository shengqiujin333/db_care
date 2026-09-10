#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/*================== 配置（设备/网关两端保持一致） ==================*/
// 固定系统密钥（16B）——请用真随机值替换
static const uint8_t K_SYS[16] = {
  0xB1,0x13,0x27,0x58,0x9A,0xCD,0xEF,0x01,0x22,0x44,0x66,0x88,0xAA,0xCC,0xEE,0x00
};

// 从 UID10 取哪 4 个字节作 uid_pick
static const uint8_t UID_IDX[4] = {1,3,7,9};

// 10 字节置换与其逆：加密前先 P，解密前先 P^-1；加/解结束后再各做一次（对称）
static const uint8_t PERM[10]  = {3, 8, 1, 5, 0, 9, 4, 7, 2, 6};
static const uint8_t IPERM[10] = {4, 2, 8, 0, 6, 3, 9, 7, 1, 5};

// 轮数（可调 6/8/10），更多轮更安全但更耗时
#define ROUNDS 8

/*================== 工具：CRC16-CCITT ==================*/
static uint16_t crc16_ccitt(const uint8_t *p, size_t n){
  uint16_t crc = 0xFFFF;
  for(size_t i=0;i<n;i++){
    crc ^= (uint16_t)p[i] << 8;
    for(int b=0;b<8;b++)
      crc = (crc & 0x8000) ? (uint16_t)((crc<<1) ^ 0x1021) : (uint16_t)(crc<<1);
  }
  return crc;
}

/*================== 轻量 S-Box（4bit） ==================*/
// 选了一个常见的 4bit SBox；只用于混淆（Feistel 下不需逆 SBox）
static const uint8_t S4[16] = {
  0xC,0x5,0x6,0xB,0x9,0x0,0xA,0xD,0x3,0xE,0xF,0x8,0x4,0x7,0x1,0x2
};
static inline uint8_t sbox_byte(uint8_t x){
  return (uint8_t)((S4[x>>4]<<4) | S4[x & 0xF]);
}

/*================== 字节置换 ==================*/
static void permute10(const uint8_t in[10], uint8_t out[10], const uint8_t p[10]){
  for(int i=0;i<10;i++) out[i] = in[p[i]];
}

/*================== 子密钥调度（极简） ==================*/
/* 生成 ROUNDS 组 5 字节子密钥。为了省电，不做重型 KDF：
   - 把 K_SYS 当作 16 字节循环池
   - 每轮取 5 字节并掺一点轮常数，然后做一次字节旋转
*/
static void subkeys_gen(uint8_t sk[ROUNDS][5]){
  uint8_t pool[16];
  memcpy(pool, K_SYS, 16);
  for(int r=0;r<ROUNDS;r++){
    // 取 5 字节 + 掺入轮常数
    for(int i=0;i<5;i++){
      sk[r][i] = (uint8_t)(pool[(r+i) & 15] ^ (uint8_t)(0xA5 ^ (r*13 + i*7)));
    }
    // 轻微搅拌：把 pool 做一次循环左移 1 字节
    uint8_t t = pool[0];
    memmove(pool, pool+1, 15);
    pool[15] = t ^ (uint8_t)(0x5A ^ r);
  }
}

/*================== 轮函数 F：输入 5 字节 R，输出 5 字节 ==================*/
/* 设计目标：极轻量，只有 XOR、循环移位、4bit SBox、加法 */
static void F_func(const uint8_t R[5], const uint8_t rk[5], uint8_t out[5], uint8_t round_idx){
  uint8_t x[5];
  // 1) 与子密钥异或
  for(int i=0;i<5;i++) x[i] = (uint8_t)(R[i] ^ rk[i]);
  // 2) 非线性：逐字节 4bit SBox
  for(int i=0;i<5;i++) x[i] = sbox_byte(x[i]);
  // 3) 轻量扩散：字节内与跨字节的旋转+混合
  //    把五字节看成 40 位寄存器，循环左移 (round_idx+1) 位
  uint64_t acc = ((uint64_t)x[0]) | ((uint64_t)x[1]<<8) | ((uint64_t)x[2]<<16) | ((uint64_t)x[3]<<24) | ((uint64_t)x[4]<<32);
  uint8_t rot = (uint8_t)((round_idx + 1) % 40);
  uint64_t y = (acc << rot) | (acc >> (40 - rot));
  out[0] = (uint8_t)(y & 0xFF);
  out[1] = (uint8_t)((y>>8) & 0xFF);
  out[2] = (uint8_t)((y>>16)& 0xFF);
  out[3] = (uint8_t)((y>>24)& 0xFF);
  out[4] = (uint8_t)((y>>32)& 0xFF);
  // 4) 再加一点跨字节混合
  out[0] ^= (uint8_t)(out[4] + 0x3D);
  out[2] ^= (uint8_t)(out[0] + out[1]);
}



static void feistel10_decrypt(uint8_t block[10]){
  uint8_t L[5], R[5], tmp[5], rk[ROUNDS][5];
  subkeys_gen(rk);

  // 先做后置置换的逆：即用 IPERM
  uint8_t buf[10]; permute10(block, buf, IPERM);
  // buf = R||L （因为加密最后交换过）
  memcpy(R, buf, 5);
  memcpy(L, buf+5, 5);

  // 逆向轮：子密钥倒序
  for(int r=ROUNDS-1; r>=0; r--){
    // 逆交换：加密最后一步是 swap(L,R)，解密时先 swap 回来
    uint8_t t5[5]; memcpy(t5, L, 5); memcpy(L, R, 5); memcpy(R, t5, 5);
    F_func(R, rk[r], tmp, (uint8_t)r);
    for(int i=0;i<5;i++) L[i] ^= tmp[i];
  }
  // 复原为 L||R，随后做前置置换的逆（IPERM）
  for(int i=0;i<5;i++){ buf[i] = L[i]; buf[5+i] = R[i]; }

  uint8_t out[10]; permute10(buf, out, IPERM);
  memcpy(block, out, 10);
}



/* 解码：成功返回 true，并输出 uid_pick/temp/hum */
bool decode_frame10(uint8_t in10[10], uint8_t uid_pick_out[4], int16_t *temp_out, uint16_t *hum_out){
  uint8_t p[10]; memcpy(p, in10, 10);
  feistel10_decrypt(p);

  // CRC 校验
  uint16_t c_calc = crc16_ccitt(p, 8);
  uint16_t c_recv = (uint16_t)p[8] | ((uint16_t)p[9]<<8);
  if (c_calc != c_recv) return false;

  if (uid_pick_out) for(int i=0;i<4;i++) uid_pick_out[i] = p[i];
  if (temp_out) *temp_out = (int16_t)(p[4] | ((int16_t)p[5]<<8));
  if (hum_out)  *hum_out  = (uint16_t)(p[6] | ((uint16_t)p[7]<<8));
  return true;
}

/*================== 小工具：打印 & 主题用 UID4 → HEX ==================*/
static void print_hex(const char* title, const uint8_t* b, size_t n){
  printf("%s", title);
  for(size_t i=0;i<n;i++) printf("%02X ", b[i]); printf("\n");
}
static void uid4_hex(const uint8_t u4[4], char out9[9]){
  static const char hex[]="0123456789ABCDEF";
  for(int i=0;i<4;i++){ out9[2*i]=hex[(u4[i]>>4)&0xF]; out9[2*i+1]=hex[u4[i]&0xF]; }
  out9[8]='\0';
}


