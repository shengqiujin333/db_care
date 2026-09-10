#include "seg_display.h"
#include <stdint.h>

// 段定义掩码
#define SEG_A 0x01
#define SEG_B 0x02
#define SEG_C 0x04
#define SEG_D 0x08
#define SEG_E 0x10
#define SEG_F 0x20
#define SEG_G 0x40

// 逻辑引脚 (1-5) 到 MCU 物理引脚的映射
// Pin 1: PA3, Pin 2: PA4, Pin 3: PA5, Pin 4: PA6, Pin 5: PB2
static void* get_port(uint8_t pin_idx) {
    if (pin_idx == 5) return CW_GPIOB;
    return CW_GPIOA;
}

static uint32_t get_pin(uint8_t pin_idx) {
    switch(pin_idx) {
        case 1: return GPIO_PIN_3;
        case 2: return GPIO_PIN_4;
        case 3: return GPIO_PIN_5;
        case 4: return GPIO_PIN_6;
        case 5: return GPIO_PIN_2;
        default: return 0;
    }
}

// 全局变量
static uint8_t dig1_segments = 0;  // DIG1 (十位) 的段掩码
static uint8_t dig2_segments = 0;  // DIG2 (个位) 的段掩码
static uint8_t current_segment = 0; // 当前扫描索引 (0-13)

// 数字 0-9 字模表
static const uint8_t digit_table[10] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
};

// 基于新图的段映射表 (Anode -> Cathode)
// 结构：{ Digit位置, 段掩码, 阳极引脚(1-5), 阴极引脚(1-5) }
typedef struct {
    uint8_t digit;
    uint8_t seg_mask;
    uint8_t anode_idx;
    uint8_t cathode_idx;
} SegmentMap_t;

static const SegmentMap_t segment_map[14] = {
    // DIG1 (左位) - 公共端主要是 Pin 1(阳) 和 Pin 2(阳)
    {0, SEG_A, 1, 2}, // A: Pin 1(+) -> Pin 2(-)
    {0, SEG_B, 1, 3}, // B: Pin 1(+) -> Pin 3(-)
    {0, SEG_C, 1, 4}, // C: Pin 1(+) -> Pin 4(-)
    {0, SEG_D, 1, 5}, // D: Pin 1(+) -> Pin 5(-)
    {0, SEG_E, 2, 1}, // E: Pin 2(+) -> Pin 1(-)
    {0, SEG_F, 2, 3}, // F: Pin 2(+) -> Pin 3(-)
    {0, SEG_G, 2, 4}, // G: Pin 2(+) -> Pin 4(-)

    // DIG2 (右位) - 公共端主要是 Pin 3(阳), Pin 4(阳), 以及跨位的 Pin 2(阳)
    {1, SEG_A, 2, 5}, // A: Pin 2(+) -> Pin 5(-)
    {1, SEG_B, 3, 1}, // B: Pin 3(+) -> Pin 1(-)
    {1, SEG_C, 3, 2}, // C: Pin 3(+) -> Pin 2(-)
    {1, SEG_D, 3, 4}, // D: Pin 3(+) -> Pin 4(-)
    {1, SEG_E, 3, 5}, // E: Pin 3(+) -> Pin 5(-)
    {1, SEG_F, 4, 1}, // F: Pin 4(+) -> Pin 1(-)
    {1, SEG_G, 4, 2}  // G: Pin 4(+) -> Pin 2(-)
};

// 辅助函数：设置所有引脚为输入 (高阻态) - 消影关键
void set_all_input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    // 批量设置 GPIOA 的 Pin 3,4,5,6
    GPIO_InitStruct.Pins = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);
    
    // 设置 GPIOB 的 Pin 2
    GPIO_InitStruct.Pins = GPIO_PIN_2;
    GPIO_Init(CW_GPIOB, &GPIO_InitStruct);
}

// 初始化函数
void Display_Init(void) {
    set_all_input();
    dig1_segments = 0;
    dig2_segments = 0;
    current_segment = 0;
}

// 设置显示数字
void Display_SetNumber(uint16_t num) {
    if (num > 99) num = 99;
    
    if (num < 10) {
        // 1位数：DIG1(十位)熄灭，DIG2(个位)显示
        dig1_segments = 0;
        dig2_segments = digit_table[num];
    } else {
        // 2位数
        dig1_segments = digit_table[num / 10];
        dig2_segments = digit_table[num % 10];
    }
}

// 扫描函数 - 必须在主循环或定时器中断中调用 (建议 1ms/次)
void Display_Scan(void) {
    const SegmentMap_t* seg = &segment_map[current_segment];
    
    // 判断当前段是否需要点亮
    uint8_t target_mask = (seg->digit == 0) ? dig1_segments : dig2_segments;
    
    if (target_mask & seg->seg_mask) {
        // --- 点亮逻辑 ---
        
        // 1. 消影：先将所有引脚设为高阻 (输入)
        set_all_input();
        
        // 2. 配置阳极为输出高电平
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.IT = GPIO_IT_NONE;
        
        GPIO_InitStruct.Pins = get_pin(seg->anode_idx);
        GPIO_Init(get_port(seg->anode_idx), &GPIO_InitStruct);
        GPIO_WritePin(get_port(seg->anode_idx), get_pin(seg->anode_idx), GPIO_Pin_SET);
        
        // 3. 配置阴极为输出低电平
        GPIO_InitStruct.Pins = get_pin(seg->cathode_idx);
        GPIO_Init(get_port(seg->cathode_idx), &GPIO_InitStruct);
        GPIO_WritePin(get_port(seg->cathode_idx), get_pin(seg->cathode_idx), GPIO_Pin_RESET);
        
    } else {
        // --- 熄灭逻辑 ---
        // 保持所有引脚为高阻 (输入)
        set_all_input();
    }
    
    // 切换到下一个段
    current_segment++;
    if (current_segment >= 14) {
        current_segment = 0;
    }
}