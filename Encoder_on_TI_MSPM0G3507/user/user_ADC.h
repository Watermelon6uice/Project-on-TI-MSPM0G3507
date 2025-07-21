#ifndef USER_ADC_H
#define USER_ADC_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ADC相关定义
#define ADC_RESOLUTION_BITS         12
#define ADC_MAX_VALUE              (4095)      // 2^12 - 1
#define ADC_REFERENCE_VOLTAGE       3.3f       // VDDA参考电压
#define ADC_SAMPLES_FOR_AVERAGE     10         // 平均采样次数

// 高频采样相关定义
#define ADC_MAX_BATCH_SIZE          50         // 批处理最大数据数量（增加批量大小）
#define ADC_BUFFER_SIZE             200        // 采样缓冲区大小（增加缓冲区）
#define ADC_MAX_SAMPLE_RATE         1000       // 最大采样频率(Hz) - 适配50000波特率

// ADC状态枚举
typedef enum {
    ADC_STATUS_OK = 0,
    ADC_STATUS_ERROR,
    ADC_STATUS_BUSY
} adc_status_t;

// ADC通道枚举
typedef enum {
    ADC_CHANNEL_0 = 0,      // GPIOA.27
    ADC_CHANNEL_MAX
} adc_channel_t;

// 全局变量声明
extern volatile bool gCheckADC;  // ADC采集成功标志位

// 函数声明
void user_adc_init(void);
adc_status_t user_adc_read_raw(adc_channel_t channel, uint16_t *value);
adc_status_t user_adc_read_voltage(adc_channel_t channel, float *voltage);
adc_status_t user_adc_read_voltage_average(adc_channel_t channel, float *voltage, uint8_t samples);
uint16_t user_adc_raw_to_millivolts(uint16_t raw_value);
float user_adc_raw_to_voltage(uint16_t raw_value);

// ADC中断服务函数（修正为配置文件中的正确名称：ADC0_IRQHandler）
void ADC0_IRQHandler(void);

// 测试和调试函数
void user_adc_test_interrupt(void);
bool user_adc_is_ready(void);
adc_status_t user_adc_read_raw_polling(adc_channel_t channel, uint16_t *value);
adc_status_t user_adc_read_raw_fast(uint16_t *value);  // 高速采样专用函数

// 高频采样函数
void user_adc_start_high_speed_sampling(uint32_t sample_rate_hz);
void user_adc_stop_high_speed_sampling(void);
void user_adc_high_speed_process(void);
bool user_adc_is_sampling(void);
void user_adc_set_batch_size(uint8_t batch_size);

#ifdef __cplusplus
}
#endif

#endif /* USER_ADC_H */
