/**
  ******************************************************************************
  * @file    sf_i2c.h
  * @author  Xiao Yang 260384793@qq.com
  * @version V1.0.0
  * @date    2021-10-06
  * @brief   This file realizes the software simulation IIC driver library.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SF_I2C_H
#define __SF_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/* Include -------------------------------------------------------------------*/
#include <stdint.h>

/* Public define -------------------------------------------------------------*/
#define I2C_WRITE(slave)            (slave & 0xFE) /*! I2C write operation    */
#define I2C_READ(slave)             (slave | 0x01) /*! I2C read operation     */

#define I2C_OBJ_FIND                1u             /*! Find i2c driver object */

/**
 * IIC error infomation
 */ 
typedef enum
{
    SF_I2C_SUCCESS = 0, /*! Not error   */
    SF_I2C_TIMEOUT,     /*! ack timeout */
} sf_i2c_err;

/**
 * IIC gpio optinos api
 */ 
typedef struct
{
    void (*sda_pin_out_low)(void);          /*! Set i2c sda pin low level     */
    void (*sda_pin_out_high)(void);         /*! Set i2c sda pin high level    */
    void (*scl_pin_out_low)(void);          /*! Set i2c scl pin low level     */
    void (*scl_pin_out_high)(void);         /*! Set i2c scl pin high level    */
    uint8_t (*sda_pin_read_level)(void);    /*! Read i2c sda pin level        */
    void (*sda_pin_dir_input)(void);        /*! Switch i2c sda pin dir input  */
    void (*sda_pin_dir_output)(void);       /*! Switch i2c sda pin dir output */
} i2c_port;

/**
 * I2C driver object
 */
typedef struct i2c_obj
{
    i2c_port port;          /*! i2c port interface    */
    uint32_t speed;         /*! control i2c bus speed */
#if (I2C_OBJ_FIND > 0u)     /*! if enable boject find */
    const char *name;       /*! i2c driver name       */
    struct i2c_obj *next;   /*! For the linked list   */
#endif
} i2c_dev;

/* Exported functions --------------------------------------------------------*/
void        i2c_init(i2c_dev *dev);
void        i2c_start(const i2c_dev *dev);
void        i2c_stop(const i2c_dev *dev);
#if (I2C_OBJ_FIND > 0u)
i2c_dev*    i2c_obj_find(const char* dev_name);
#endif
sf_i2c_err  i2c_write_byte(const i2c_dev *dev, uint8_t byte);
uint8_t     i2c_read_byte(const i2c_dev *dev, uint8_t ack);
sf_i2c_err  i2c_write_multi_byte(const i2c_dev *dev, uint8_t slave_addr, 
                                 uint8_t reg_addr, void *pbuf, uint16_t length);
void        i2c_read_multi_byte(const i2c_dev *dev, uint8_t slave_addr, 
                                uint8_t reg_addr, void *pbuf, uint16_t length);

/* 无寄存器地址器件原语 (FD-002 §3.2, 供 GXHT40 使用):
 *   i2c_write_cmd  : START -> 地址+W -> 命令 -> STOP
 *   i2c_read_bytes : START -> 地址+R -> N 字节(前 N-1 字节主机 ACK, 末字节主机 NACK) -> STOP
 * 两者都把从机 ACK 结果作为返回值 (SF_I2C_SUCCESS / SF_I2C_TIMEOUT)。
 * 既有函数语义与调用方式不变。 */
sf_i2c_err  i2c_write_cmd(const i2c_dev *dev, uint8_t slave_addr, uint8_t cmd);
sf_i2c_err  i2c_read_bytes(const i2c_dev *dev, uint8_t slave_addr, void *pbuf, uint16_t length);

/* 地址探测原语 (FWR-116; FD-002 rev 5.0 §6.6.1): 只发地址写字节, 用于上电总线身份诊断。
 *   i2c_probe_addr : START -> 地址字节(8bit 写形式, 7bit<<1) -> ACK 判定 -> STOP
 * 返回 SF_I2C_SUCCESS = 该地址收到 ACK; SF_I2C_TIMEOUT = 无 ACK (总线已释放)。
 * 不写命令、不读数据; 无器件应答时也会发 STOP 释放总线; 既有函数语义不变。 */
sf_i2c_err  i2c_probe_addr(const i2c_dev *dev, uint8_t slave_addr);
sf_i2c_err  i2c_write_multi_byte_16bit(const i2c_dev *dev, uint8_t slave_addr, 
                                       uint16_t reg_addr, void *pbuf, uint16_t length);
void        i2c_read_multi_byte_16bit(const i2c_dev *dev, uint8_t slave_addr, 
                                      uint16_t reg_addr, void *pbuf, uint16_t length);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SF_I2C_H */
