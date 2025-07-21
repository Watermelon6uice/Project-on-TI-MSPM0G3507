#ifndef USER_ENCODER_H
#define USER_ENCODER_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>

// 确保IOMUX定义可用，如果ti_msp_dl_config.h中未定义
#ifndef ENCODER_PIN_A_IOMUX
#define ENCODER_PIN_A_IOMUX                                      (IOMUX_PINCM39)
#endif

#ifndef ENCODER_PIN_B_IOMUX
#define ENCODER_PIN_B_IOMUX                                      (IOMUX_PINCM54)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 编码器引脚定义 (根据ti_msp_dl_config.h中的配置)
#define ENCODER_PIN_A_PORT          ENCODER_PORT        // GPIOA
#define ENCODER_PIN_A               ENCODER_PIN_A_PIN   // DL_GPIO_PIN_17 (PA17)
#define ENCODER_PIN_B_PORT          ENCODER_PORT        // GPIOA  
#define ENCODER_PIN_B               ENCODER_PIN_B_PIN   // DL_GPIO_PIN_24 (PA24)

// 编码器计数范围定义
#define ENCODER_COUNT_MIN           0
#define ENCODER_COUNT_MAX           33      // 0-3.3V，步进0.1V，共34个级别(0-33)

// DAC电压控制参数
#define DAC_VOLTAGE_MIN             0.0f    // 最小电压 0V
#define DAC_VOLTAGE_MAX             3.3f    // 最大电压 3.3V
#define DAC_VOLTAGE_STEP            0.1f    // 电压步进 0.1V
#define DAC_MAX_COUNT_VALUE         4095    // 12位DAC最大值

// 编码器状态结构体
typedef struct {
    int16_t count;              // 当前计数值
    int16_t last_count;         // 上次计数值
    bool count_changed;         // 计数值改变标志
    float target_voltage;       // 目标电压值
    uint16_t dac_value;         // 对应的DAC数字值
} encoder_state_t;

// 全局变量声明
extern volatile encoder_state_t g_encoder_state;

// 函数声明
void user_encoder_init(void);
int16_t user_encoder_get_count(void);
bool user_encoder_is_changed(void);
float user_encoder_count_to_voltage(int16_t count);
uint16_t user_encoder_voltage_to_dac(float voltage);
void user_encoder_update_dac(void);
void user_encoder_reset_count(void);
void user_encoder_set_count(int16_t count);

#ifdef __cplusplus
}
#endif

#endif /* USER_ENCODER_H */
