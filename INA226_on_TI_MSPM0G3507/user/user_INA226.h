#ifndef USER_INA226_H_
#define USER_INA226_H_

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

/* I2C实例定义 */
#define I2C_INST                I2C_0_INST

/* INA226 I2C地址 */
#define INA226_ADDRESS          0x41

/* INA226 寄存器地址 */
#define INA226_CONF_REG         0x00    // 配置寄存器
#define INA226_SHUNT_REG        0x01    // 分流电压寄存器
#define INA226_BUS_REG          0x02    // 总线电压寄存器
#define INA226_PWR_REG          0x03    // 功率寄存器
#define INA226_CURRENT_REG      0x04    // 电流寄存器
#define INA226_CAL_REG          0x05    // 校准寄存器
#define INA226_MASK_EN_REG      0x06    // 掩码/使能寄存器
#define INA226_ALERT_LIMIT_REG  0x07    // 警报限制寄存器
#define INA226_MAN_ID_REG       0xFE    // 制造商ID寄存器
#define INA226_ID_REG           0xFF    // 器件ID寄存器

/* 配置寄存器位定义 */
#define INA226_RST              0x8000  // 复位位
#define INA226_AFF              0x0010  // 警报功能标志
#define INA226_CVRF             0x0008  // 转换就绪标志
#define INA226_OVF              0x0004  // 溢出标志
#define INA226_ALERT_POL        0x0002  // 警报引脚极性
#define INA226_LATCH_EN         0x0001  // 锁存使能

/* 平均采样模式枚举 */
typedef enum {
    INA226_AVERAGE_1       = 0x0000,
    INA226_AVERAGE_4       = 0x0200,
    INA226_AVERAGE_16      = 0x0400,
    INA226_AVERAGE_64      = 0x0600,
    INA226_AVERAGE_128     = 0x0800,
    INA226_AVERAGE_256     = 0x0A00,
    INA226_AVERAGE_512     = 0x0C00,
    INA226_AVERAGE_1024    = 0x0E00
} INA226_AVERAGES;

/* 转换时间枚举 (微秒) */
typedef enum {
    INA226_CONV_TIME_140   = 0b00000000,
    INA226_CONV_TIME_204   = 0b00000001,
    INA226_CONV_TIME_332   = 0b00000010,
    INA226_CONV_TIME_588   = 0b00000011,
    INA226_CONV_TIME_1100  = 0b00000100,
    INA226_CONV_TIME_2116  = 0b00000101,
    INA226_CONV_TIME_4156  = 0b00000110,
    INA226_CONV_TIME_8244  = 0b00000111
} INA226_CONV_TIME;

/* 测量模式枚举 */
typedef enum {
    INA226_POWER_DOWN      = 0b00000000,
    INA226_TRIGGERED       = 0b00000011,
    INA226_CONTINUOUS      = 0b00000111
} INA226_MEASURE_MODE;

/* 警报类型枚举 */
typedef enum {
    INA226_SHUNT_OVER     = 0x8000,
    INA226_SHUNT_UNDER    = 0x4000,
    INA226_BUS_OVER       = 0x2000,
    INA226_BUS_UNDER      = 0x1000,
    INA226_POWER_OVER     = 0x0800,
    INA226_CURRENT_OVER   = 0xFFFE,
    INA226_CURRENT_UNDER  = 0xFFFF
} INA226_ALERT_TYPE;

/* INA226 器件结构体 */
typedef struct {
    uint8_t i2c_address;           // I2C地址
    uint16_t cal_value;            // 校准值
    float corr_factor;             // 校正因子
    uint16_t conf_reg_copy;        // 配置寄存器备份
    float current_divider_mA;      // 电流除数(mA)
    float pwr_multiplier_mW;       // 功率乘数(mW)
    INA226_AVERAGES averages;      // 平均模式
    INA226_MEASURE_MODE measure_mode;  // 测量模式
    INA226_ALERT_TYPE alert_type;  // 警报类型
    bool overflow;                 // 溢出标志
    bool conv_alert;               // 转换警报
    bool limit_alert;              // 限制警报
    uint8_t i2c_error_code;        // I2C错误代码
} INA226_Device;

/* 函数声明 */

/**
 * @brief 初始化INA226器件
 * @param device 指向INA226设备结构体的指针
 * @param i2c_address I2C地址 (默认0x40)
 * @return true初始化成功，false初始化失败
 */
bool ina226_init(INA226_Device *device, uint8_t i2c_address);

/**
 * @brief 复位INA226器件
 * @param device 指向INA226设备结构体的指针
 */
void ina226_reset(INA226_Device *device);

/**
 * @brief 设置校正因子
 * @param device 指向INA226设备结构体的指针
 * @param corr 校正因子
 */
void ina226_set_correction_factor(INA226_Device *device, float corr);

/**
 * @brief 设置平均采样次数
 * @param device 指向INA226设备结构体的指针
 * @param averages 平均采样模式
 */
void ina226_set_average(INA226_Device *device, INA226_AVERAGES averages);

/**
 * @brief 设置转换时间
 * @param device 指向INA226设备结构体的指针
 * @param shunt_conv_time 分流转换时间
 * @param bus_conv_time 总线转换时间
 */
void ina226_set_conversion_time(INA226_Device *device, INA226_CONV_TIME shunt_conv_time, INA226_CONV_TIME bus_conv_time);

/**
 * @brief 设置测量模式
 * @param device 指向INA226设备结构体的指针
 * @param mode 测量模式
 */
void ina226_set_measure_mode(INA226_Device *device, INA226_MEASURE_MODE mode);

/**
 * @brief 设置电阻和电流范围
 * @param device 指向INA226设备结构体的指针
 * @param resistor 分流电阻值(欧姆)
 * @param current_range 电流范围(安培)
 */
void ina226_set_resistor_range(INA226_Device *device, float resistor, float current_range);

/**
 * @brief 获取分流电压(伏特)
 * @param device 指向INA226设备结构体的指针
 * @return 分流电压值(V)
 */
float ina226_get_shunt_voltage_V(INA226_Device *device);

/**
 * @brief 获取分流电压(毫伏)
 * @param device 指向INA226设备结构体的指针
 * @return 分流电压值(mV)
 */
float ina226_get_shunt_voltage_mV(INA226_Device *device);

/**
 * @brief 获取总线电压(伏特)
 * @param device 指向INA226设备结构体的指针
 * @return 总线电压值(V)
 */
float ina226_get_bus_voltage_V(INA226_Device *device);

/**
 * @brief 获取电流(毫安)
 * @param device 指向INA226设备结构体的指针
 * @return 电流值(mA)
 */
float ina226_get_current_mA(INA226_Device *device);

/**
 * @brief 获取电流(安培)
 * @param device 指向INA226设备结构体的指针
 * @return 电流值(A)
 */
float ina226_get_current_A(INA226_Device *device);

/**
 * @brief 获取功率
 * @param device 指向INA226设备结构体的指针
 * @return 功率值(mW)
 */
float ina226_get_bus_power(INA226_Device *device);

/**
 * @brief 一次性读取所有数据 - 高效版本
 * @param device 指向INA226设备结构体的指针
 * @param bus_voltage 总线电压输出指针(V)，可为NULL
 * @param shunt_voltage 分流电压输出指针(mV)，可为NULL
 * @param current 电流输出指针(mA)，可为NULL
 * @param power 功率输出指针(mW)，可为NULL
 */
void ina226_read_all_data(INA226_Device *device, float *bus_voltage, float *shunt_voltage, float *current, float *power);

/**
 * @brief 开始单次测量
 * @param device 指向INA226设备结构体的指针
 */
void ina226_start_single_measurement(INA226_Device *device);

/**
 * @brief 开始单次测量(不等待完成)
 * @param device 指向INA226设备结构体的指针
 */
void ina226_start_single_measurement_no_wait(INA226_Device *device);

/**
 * @brief 检查是否忙碌
 * @param device 指向INA226设备结构体的指针
 * @return true忙碌，false空闲
 */
bool ina226_is_busy(INA226_Device *device);

/**
 * @brief 进入省电模式
 * @param device 指向INA226设备结构体的指针
 */
void ina226_power_down(INA226_Device *device);

/**
 * @brief 退出省电模式
 * @param device 指向INA226设备结构体的指针
 */
void ina226_power_up(INA226_Device *device);

/**
 * @brief 等待转换完成
 * @param device 指向INA226设备结构体的指针
 */
void ina226_wait_until_conversion_completed(INA226_Device *device);

/**
 * @brief 设置警报引脚为高电平有效
 * @param device 指向INA226设备结构体的指针
 */
void ina226_set_alert_pin_active_high(INA226_Device *device);

/**
 * @brief 使能警报锁存
 * @param device 指向INA226设备结构体的指针
 */
void ina226_enable_alert_latch(INA226_Device *device);

/**
 * @brief 使能转换就绪警报
 * @param device 指向INA226设备结构体的指针
 */
void ina226_enable_conv_ready_alert(INA226_Device *device);

/**
 * @brief 设置警报类型和限制
 * @param device 指向INA226设备结构体的指针
 * @param type 警报类型
 * @param limit 限制值
 */
void ina226_set_alert_type(INA226_Device *device, INA226_ALERT_TYPE type, float limit);

/**
 * @brief 读取并清除标志
 * @param device 指向INA226设备结构体的指针
 */
void ina226_read_and_clear_flags(INA226_Device *device);

/**
 * @brief 获取I2C错误代码
 * @param device 指向INA226设备结构体的指针
 * @return I2C错误代码
 */
uint8_t ina226_get_i2c_error_code(INA226_Device *device);

/* 内部函数 */
void ina226_write_register(INA226_Device *device, uint8_t reg, uint16_t val);
uint16_t ina226_read_register(INA226_Device *device, uint8_t reg);

#endif /* USER_INA226_H_ */
