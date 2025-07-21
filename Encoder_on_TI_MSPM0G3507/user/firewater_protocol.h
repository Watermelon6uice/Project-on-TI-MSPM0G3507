#ifndef FIREWATER_PROTOCOL_H_
#define FIREWATER_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>

/* Firewater协议定义 - 符合VOFA+规范 */
/* 数据格式: "ch0,ch1,ch2,...,chN\n" 或 "prefix:ch0,ch1,ch2,...,chN\n" */

/**
 * @brief 发送电压数据（单通道）
 * @param voltage 电压值(V)
 */
void firewater_send_voltage(float voltage);

/**
 * @brief 发送电流数据（单通道）
 * @param current 电流值(mA)
 */
void firewater_send_current(float current);

/**
 * @brief 发送功率数据（单通道）
 * @param power 功率值(mW)
 */
void firewater_send_power(float power);

/**
 * @brief 发送多通道数据
 * @param values 数据数组
 * @param count 数据个数
 * @param prefix 可选前缀，如"samples"，可为NULL
 */
void firewater_send_multi_channel(float *values, uint8_t count, const char *prefix);

/**
 * @brief 发送完整的INA226数据包（电压、电流、功率）
 * @param voltage 电压值(V)
 * @param current 电流值(mA) 
 * @param power 功率值(mW)
 */
void firewater_send_ina226_data(float voltage, float current, float power);

/**
 * @brief 发送带时间戳的INA226数据包
 * @param voltage 电压值(V)
 * @param current 电流值(mA)
 * @param power 功率值(mW)
 * @param timestamp 时间戳(ms)
 */
void firewater_send_ina226_with_timestamp(float voltage, float current, float power, uint32_t timestamp);

/**
 * @brief 发送ADC电压数据（优化版本，用于高频采样）
 * @param voltage 电压值(V)
 * @param sample_id 采样ID
 */
void firewater_send_adc_voltage(float voltage, uint32_t sample_id);

/**
 * @brief 发送ADC电压数据（简化版本，只发送电压值）
 * @param voltage 电压值(V)
 */
void firewater_send_adc_voltage_simple(float voltage);

/**
 * @brief 发送ADC批量数据
 * @param voltages 电压数组
 * @param count 数据个数
 * @param start_sample_id 起始采样ID
 */
void firewater_send_adc_batch(float *voltages, uint8_t count, uint32_t start_sample_id);

/**
 * @brief 测试VOFA+数据发送连接
 */
void firewater_test_vofa_connection(void);

#endif /* FIREWATER_PROTOCOL_H_ */