#include "ti_msp_dl_config.h"
#include "user/delay.h"
#include "user/user_key.h"
#include "user/user_uart.h"
#include "user/user_INA226.h"
#include "user/firewater_protocol.h"
#include <stdio.h>

// 全局时间计数器（毫秒）
volatile uint32_t system_time_ms = 0;

// INA226设备实例
static INA226_Device ina226_device;
static bool ina226_initialized = false;

// 数据采集状态
static uint32_t last_voltage_read_time = 0;
static uint32_t last_current_read_time = 0;
static uint32_t last_power_read_time = 0;

/**
 * @brief 初始化INA226传感器
 */
void ina226_system_init(void) {
    if (ina226_init(&ina226_device, INA226_ADDRESS)) {
        // 不调用 ina226_set_resistor_range，使用Arduino库的默认值
        // Arduino库默认：calVal=2048, currentDivider_mA=40.0, pwrMultiplier_mW=0.625
        
        ina226_initialized = true;
        //user_uart_send_string("INA226 初始化成功! (连续模式, 无平均, 10Hz采样)\r\n");
        
        // 发送初始化完成状态（使用文本格式）
        // user_uart_send_string("status:1\n");
    } else {
        //user_uart_send_string("INA226 初始化失败!\r\n");
        // user_uart_send_string("status:255\n");
    }
}

/**
 * @brief 非阻塞式INA226数据读取和发送
 */
void ina226_data_process(void) {
    if (!ina226_initialized) {
        return;
    }
    
    // 每50ms读取电压并发送（恢复正常的读取频率）
    if ((system_time_ms - last_voltage_read_time) >= 50) {
        // 使用专门的电压读取函数，内置转换完成等待
        float voltage = ina226_get_bus_voltage_V(&ina226_device);
        
        // 检查I2C错误状态
        if (ina226_device.i2c_error_code == 0) {
            // 发送电压信息
            char debug_str[64];
            sprintf(debug_str, "V:%.3f\n", voltage);
            user_uart_send_string(debug_str);
        } else {
            // 发送错误信息
            char error_str[64];
            sprintf(error_str, "I2C_ERR:%d\n", ina226_device.i2c_error_code);
            user_uart_send_string(error_str);
        }
        
        last_voltage_read_time = system_time_ms;
    }
}

int main(void)
{
    static uint32_t last_led_time = 0;  // 上次LED切换时间
    static uint32_t last_status_time = 0; // 上次状态发送时间
    
    // 系统初始化
    SYSCFG_DL_init();
    user_uart_init();   // 初始化UART
    // SysTick已在delay库中初始化，不需要重复初始化
    
    // 发送启动信息
    // user_uart_send_string("INA226 Firewater Protocol Demo Started!\r\n");
    // user_uart_send_string("Initializing INA226...\r\n");
    
    // 初始化INA226（延时一段时间确保I2C稳定）
    delay_ms(100);
    ina226_system_init();
    
    while (1) {
        
        // 每5秒发送一次系统状态
        // if ((system_time_ms - last_status_time) >= 5000) {
        //     if (ina226_initialized) {
        //         firewater_send_status(0x00, system_time_ms);  // 正常运行状态
        //     } else {
        //         firewater_send_status(0xFF, system_time_ms);  // 错误状态
        //     }
        //     last_status_time = system_time_ms;
        // }
        
        // 处理按键事件
        user_key_process();
        
        // 非阻塞式INA226数据读取和发送
        ina226_data_process();
        
    }
}

