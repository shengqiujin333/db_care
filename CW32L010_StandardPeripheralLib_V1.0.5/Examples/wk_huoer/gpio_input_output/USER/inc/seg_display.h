#ifndef SEG_DISPLAY_H
#define SEG_DISPLAY_H

//#include "main.h" // 包含你的 MCU 头文件，确保有 GPIO 定义
#include "cw32l010_gpio.h"
// 定义数码管引脚对应的 MCU 引脚
// Pin 1 -> PA3
// Pin 2 -> PA4
// Pin 3 -> PA5
// Pin 4 -> PA6
// Pin 5 -> PB2

// 函数声明
void Display_Init(void);
void Display_SetNumber(uint16_t num); // 输入 0-99
void Display_Scan(void);              // 需要在主循环或定时器中频繁调用
void set_all_input(void) ;
#endif