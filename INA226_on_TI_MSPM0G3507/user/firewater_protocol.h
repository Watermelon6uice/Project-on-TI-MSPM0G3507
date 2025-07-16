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

#endif /* FIREWATER_PROTOCOL_H_ */