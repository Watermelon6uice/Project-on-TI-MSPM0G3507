#include "firewater_protocol.h"
#include "user_uart.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 发送电压数据（单通道）
 */
void firewater_send_voltage(float voltage) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.3f\n", voltage);
    user_uart_send_string(buffer);
}

/**
 * @brief 发送电流数据（单通道）
 */
void firewater_send_current(float current) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f\n", current);
    user_uart_send_string(buffer);
}

/**
 * @brief 发送功率数据（单通道）
 */
void firewater_send_power(float power) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f\n", power);
    user_uart_send_string(buffer);
}

/**
 * @brief 发送多通道数据
 */
void firewater_send_multi_channel(float *values, uint8_t count, const char *prefix) {
    char buffer[256];
    int offset = 0;
    
    // 添加前缀（如果有）
    if (prefix != NULL) {
        offset = snprintf(buffer, sizeof(buffer), "%s:", prefix);
    }
    
    // 添加数据
    for (uint8_t i = 0; i < count; i++) {
        if (i == 0) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%.3f", values[i]);
        } else {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, ",%.3f", values[i]);
        }
        
        // 防止缓冲区溢出
        if (offset >= sizeof(buffer) - 10) {
            break;
        }
    }
    
    // 添加换行符
    snprintf(buffer + offset, sizeof(buffer) - offset, "\n");
    
    user_uart_send_string(buffer);
}

/**
 * @brief 发送完整的INA226数据包（电压、电流、功率）
 */
void firewater_send_ina226_data(float voltage, float current, float power) {
    float values[3] = {voltage, current, power};
    firewater_send_multi_channel(values, 3, NULL);
}

/**
 * @brief 发送带时间戳的INA226数据包
 */
void firewater_send_ina226_with_timestamp(float voltage, float current, float power, uint32_t timestamp) {
    float values[4] = {voltage, current, power, (float)timestamp};
    firewater_send_multi_channel(values, 4, "ina226");
}

/**
 * @brief 发送ADC电压数据（优化版本，用于高频采样）
 */
void firewater_send_adc_voltage(float voltage, uint32_t sample_id) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "adc:%.4f,%u\n", voltage, sample_id);
    user_uart_send_string(buffer);
}

/**
 * @brief 发送ADC电压数据（简化版本，只发送电压值）
 * 优化版本：减少小数位数以提高传输效率
 */
void firewater_send_adc_voltage_simple(float voltage) {
    char buffer[16];
    // 改为3位小数，减少数据长度：例如 "1.234\n" (6字节)
    snprintf(buffer, sizeof(buffer), "%.2f\n", voltage);
    user_uart_send_string(buffer);
}

/**
 * @brief 发送ADC批量数据（修改为每个数据单独一行发送）
 */
void firewater_send_adc_batch(float *voltages, uint8_t count, uint32_t start_sample_id) {
    // 逐个发送每个电压值，每个值占用一行，保证数据在一个通道里
    for (uint8_t i = 0; i < count; i++) {
        firewater_send_adc_voltage_simple(voltages[i]);
    }
}

/**
 * @brief 测试VOFA+数据发送连接
 */
void firewater_test_vofa_connection(void) {
    // 发送一些测试数据
    float test_voltages[] = {1.0, 1.5, 2.0, 2.5, 3.0};
    
    for (int i = 0; i < 5; i++) {
        firewater_send_adc_voltage_simple(test_voltages[i]);
        delay_ms(100);  // 100ms间隔
    }
}
