#include "firewater_protocol.h"
#include "user_uart.h"
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
