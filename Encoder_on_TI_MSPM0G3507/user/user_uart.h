#ifndef USER_UART_H
#define USER_UART_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// UART接收缓冲区大小
#define UART_RX_BUFFER_SIZE 128
#define UART_TX_BUFFER_SIZE 128

// UART状态枚举
typedef enum {
    UART_OK = 0,
    UART_ERROR,
    UART_BUSY,
    UART_TIMEOUT
} uart_status_t;

// UART库初始化
void user_uart_init(void);

// 发送函数
uart_status_t user_uart_send_byte(uint8_t data);
uart_status_t user_uart_send_string(const char* str);
uart_status_t user_uart_send_data(const uint8_t* data, uint16_t length);

// 接收函数
bool user_uart_is_data_available(void);
uint8_t user_uart_receive_byte(void);
uint16_t user_uart_receive_string(char* buffer, uint16_t max_length);
uint16_t user_uart_get_rx_count(void);

// 清空接收缓冲区
void user_uart_clear_rx_buffer(void);

// 基础回显功能 - 发送什么返回什么
void user_uart_echo_process(void);

// 调试函数
uint32_t user_uart_get_irq_count(void);

#endif // USER_UART_H