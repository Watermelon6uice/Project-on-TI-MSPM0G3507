#include "user_uart.h"

// 静态变量
static uint8_t rx_buffer[UART_RX_BUFFER_SIZE];  // 接收缓冲区
static volatile uint16_t rx_head = 0;           // 接收缓冲区头指针
static volatile uint16_t rx_tail = 0;           // 接收缓冲区尾指针
static volatile uint16_t rx_count = 0;          // 接收缓冲区数据计数
static volatile uint32_t irq_counter = 0;      // 中断计数器（调试用）

/**
 * @brief UART库初始化
 */
void user_uart_init(void)
{
    // 清空接收缓冲区
    user_uart_clear_rx_buffer();
    
    // 清除UART0中断挂起状态
    DL_UART_Main_clearInterruptStatus(UART_0_INST, DL_UART_MAIN_INTERRUPT_RX);
    
    // 启用NVIC中断 - 这是关键步骤！
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    
    // UART硬件已经在SYSCFG_DL_init()中初始化完成
    // 中断也已经在syscfg中配置启用
}

/**
 * @brief 发送单个字节
 * @param data 要发送的字节
 * @return uart_status_t 发送状态
 */
uart_status_t user_uart_send_byte(uint8_t data)
{
    // 等待发送缓冲区空闲
    while (!DL_UART_Main_isTXFIFOEmpty(UART_0_INST)) {
        // 可以添加超时检查
    }
    
    // 发送数据
    DL_UART_Main_transmitData(UART_0_INST, data);
    
    return UART_OK;
}

/**
 * @brief 发送字符串
 * @param str 要发送的字符串
 * @return uart_status_t 发送状态
 */
uart_status_t user_uart_send_string(const char* str)
{
    if (str == NULL) {
        return UART_ERROR;
    }
    
    while (*str != '\0') {
        if (user_uart_send_byte(*str) != UART_OK) {
            return UART_ERROR;
        }
        str++;
    }
    
    return UART_OK;
}

/**
 * @brief 发送数据数组
 * @param data 要发送的数据指针
 * @param length 数据长度
 * @return uart_status_t 发送状态
 */
uart_status_t user_uart_send_data(const uint8_t* data, uint16_t length)
{
    if (data == NULL) {
        return UART_ERROR;
    }
    
    for (uint16_t i = 0; i < length; i++) {
        if (user_uart_send_byte(data[i]) != UART_OK) {
            return UART_ERROR;
        }
    }
    
    return UART_OK;
}

/**
 * @brief 检查是否有数据可读
 * @return bool 有数据返回true，无数据返回false
 */
bool user_uart_is_data_available(void)
{
    return (rx_count > 0);
}

/**
 * @brief 接收单个字节
 * @return uint8_t 接收到的字节
 */
uint8_t user_uart_receive_byte(void)
{
    uint8_t data = 0;
    
    if (rx_count > 0) {
        data = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % UART_RX_BUFFER_SIZE;
        rx_count--;
    }
    
    return data;
}

/**
 * @brief 接收字符串
 * @param buffer 接收缓冲区
 * @param max_length 最大长度
 * @return uint16_t 实际接收到的字节数
 */
uint16_t user_uart_receive_string(char* buffer, uint16_t max_length)
{
    uint16_t count = 0;
    
    if (buffer == NULL || max_length == 0) {
        return 0;
    }
    
    while (count < (max_length - 1) && user_uart_is_data_available()) {
        buffer[count] = user_uart_receive_byte();
        count++;
    }
    
    buffer[count] = '\0';  // 添加字符串结束符
    return count;
}

/**
 * @brief 获取接收缓冲区中的数据数量
 * @return uint16_t 数据数量
 */
uint16_t user_uart_get_rx_count(void)
{
    return rx_count;
}

/**
 * @brief 清空接收缓冲区
 */
void user_uart_clear_rx_buffer(void)
{
    rx_head = 0;
    rx_tail = 0;
    rx_count = 0;
}

/**
 * @brief 基础回显功能处理函数 - 发送什么返回什么
 * 在主循环中调用此函数来处理回显
 */
void user_uart_echo_process(void)
{
    static char rx_line[UART_RX_BUFFER_SIZE];
    static uint16_t line_pos = 0;
    
    // 处理接收到的每一个字节 - 行回显模式
    while (user_uart_is_data_available()) {
        uint8_t received_byte = user_uart_receive_byte();
        
        // 不立即回显字符，只在收到完整行时处理
        if (received_byte == '\r' || received_byte == '\n') {
            // 收到回车或换行，处理完整的一行
            if (line_pos > 0) {
                rx_line[line_pos] = '\0';
                
                // 发送换行
                user_uart_send_string("\r\n");
                
                // 回显整行内容
                user_uart_send_string("Echo: ");
                user_uart_send_string(rx_line);
                user_uart_send_string("\r\n");
                
                // 重置行缓冲区
                line_pos = 0;
            } else {
                // 空行，只发送换行
                user_uart_send_string("\r\n");
            }
        }
        else if (received_byte == '\b' || received_byte == 127) {
            // 处理退格键
            if (line_pos > 0) {
                line_pos--;
                user_uart_send_string("\b \b");  // 退格-空格-退格
            }
        }
        else if (line_pos < (UART_RX_BUFFER_SIZE - 1)) {
            // 普通字符，添加到行缓冲区
            rx_line[line_pos++] = received_byte;
        }
    }
}

/**
 * @brief UART接收中断服务函数（内部函数）
 * 在UART0_IRQHandler中调用此函数
 */
static void user_uart_rx_isr(void)
{
    uint8_t received_data;  // 在函数开头声明变量
    
    // 增加中断计数器
    irq_counter++;
    
    // 使用与示例代码相同的函数和常量
    switch (DL_UART_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_IIDX_RX:  // 接收中断
            // 接收到数据
            received_data = DL_UART_Main_receiveData(UART_0_INST);
            
            // 存储到缓冲区
            if (rx_count < UART_RX_BUFFER_SIZE) {
                rx_buffer[rx_head] = received_data;
                rx_head = (rx_head + 1) % UART_RX_BUFFER_SIZE;
                rx_count++;
            }
            break;
        default:
            break;
    }
}


/**
 * @brief UART0中断服务函数
 * 直接在UART库中实现，无需在main.c中调用
 * 函数名必须与ti_msp_dl_config.h中的定义一致
 */
void UART0_IRQHandler(void)
{
    user_uart_rx_isr();
}

/**
 * @brief 获取中断计数器（调试用）
 * @return uint32_t 中断次数
 */
uint32_t user_uart_get_irq_count(void)
{
    return irq_counter;
}
