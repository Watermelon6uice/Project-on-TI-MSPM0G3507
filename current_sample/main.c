#include "ti_msp_dl_config.h"
#include "user/delay.h"
#include "user/user_uart.h"
#include "user/firewater_protocol.h"
#include "user/user_ADC.h"
#include "user/user_DAC.h"
#include "user/user_OLED.h"
#include "user/user_Encoder.h"
#include "user/user_current_sensor.h"
#include <stdio.h>

// 全局时间计数器（毫秒）
volatile uint32_t system_time_ms = 0;

// 电流显示更新间隔（毫秒）
#define CURRENT_DISPLAY_INTERVAL_MS    100
#define CALIBRATION_TIME_MS           5000   // 启动后校准时间

// 显示状态枚举
typedef enum {
    DISPLAY_STATE_CALIBRATING = 0,
    DISPLAY_STATE_MEASURING
} display_state_t;

static display_state_t display_state = DISPLAY_STATE_CALIBRATING;

int main(void)
{
    char msg[100];
    static uint32_t last_display_time = 0;
    static uint32_t calibration_start_time = 0;
    float current_value = 0.0f;
    current_measurement_t measurement_data;
    current_sensor_status_t sensor_status;
    
    // 系统初始化
    SYSCFG_DL_init();
    
    // 初始化各个模块
    user_uart_init();
    delay_ms(1000);  // 等待UART稳定
    
    // 初始化OLED显示
    OLED_Init();
    delay_ms(100);
    OLED_Clear();
    
    // 初始化电流传感器
    sensor_status = user_current_sensor_init();
    if (sensor_status != CURRENT_SENSOR_OK) {
        OLED_ShowString(1, 1, "SENSOR ERROR!");
        user_uart_send_string("Current sensor init failed!\r\n");
        while(1); // 停止运行
    }
    
    // 系统就绪指示
    DL_GPIO_setPins(LED_PORT, LED_LED1_PIN);
    DL_GPIO_setPins(OUTPUT_PORT, OUTPUT_OUTPUT1_PIN);
    user_uart_send_string("System ready - Current Measurement Active\r\n");
    
    // 显示校准界面
    OLED_ShowString(1, 1, "Zero Calibrating");
    OLED_ShowString(2, 1, "Please ensure");
    OLED_ShowString(3, 1, "NO current flow");
    OLED_ShowString(4, 1, "Wait 5 seconds..");
    
    user_uart_send_string("Starting zero calibration, please ensure no current flows through sensor\r\n");
    
    // 记录校准开始时间
    calibration_start_time = system_time_ms;
    
    while (1) {
        // 处理校准状态
        if (display_state == DISPLAY_STATE_CALIBRATING) {
            // 显示倒计时
            uint32_t remaining_time = (CALIBRATION_TIME_MS - (system_time_ms - calibration_start_time)) / 1000;
            if (remaining_time > 0) {
                sprintf(msg, "Countdown: %d s", (int)remaining_time);
                OLED_ShowString(4, 1, msg);
            }
            
            if (system_time_ms - calibration_start_time >= CALIBRATION_TIME_MS) {
                // 执行零点校准
                OLED_ShowString(4, 1, "Calibrating...");
                user_uart_send_string("Performing zero calibration...\r\n");
                
                sensor_status = user_current_calibrate_zero();
                if (sensor_status == CURRENT_SENSOR_OK) {
                    display_state = DISPLAY_STATE_MEASURING;
                    OLED_Clear();
                    OLED_ShowString(1, 1, "Current(A):");
                    OLED_ShowString(3, 1, "Voltage(V):");
                    
                    sprintf(msg, "Zero calibration completed! New offset: %.3fV\r\n", 
                           user_current_get_zero_offset());
                    user_uart_send_string(msg);
                    
                    sprintf(msg, "Previous offset was 1.650V, correction: %.3fV\r\n", 
                           user_current_get_zero_offset() - 1.650f);
                    user_uart_send_string(msg);
                } else {
                    OLED_ShowString(4, 1, "CAL FAILED!");
                    user_uart_send_string("Calibration failed!\r\n");
                    delay_ms(2000);
                    // 重新开始校准
                    calibration_start_time = system_time_ms;
                }
                last_display_time = system_time_ms;
            }
        }
        
        // 测量和显示状态  
        else if (display_state == DISPLAY_STATE_MEASURING) {
        // 定时更新显示和测量
        if (system_time_ms - last_display_time >= CURRENT_DISPLAY_INTERVAL_MS) {
            // 使用平均测量获取更稳定的电流值
            sensor_status = user_current_read_average(&current_value, CURRENT_AVERAGE_SAMPLES);
            
            if (sensor_status == CURRENT_SENSOR_OK || 
                sensor_status == CURRENT_SENSOR_OVER_RANGE || 
                sensor_status == CURRENT_SENSOR_UNDER_RANGE) {
                
                // current_value已经是平均值，直接使用
                
                // OLED显示电流值
                OLED_ShowFloat(2, 1, current_value, 3, 3);  // 显示为 XXX.XXX A
                OLED_ShowString(2, 8, "A    ");
                
                // OLED显示电压值（使用全局数据中的电压值）
                OLED_ShowFloat(4, 1, g_current_data.voltage_raw, 1, 3);  // 显示为 X.XXX V
                OLED_ShowString(4, 6, "V    ");
                
                // 状态指示
                if (sensor_status == CURRENT_SENSOR_OVER_RANGE) {
                    OLED_ShowString(1, 11, "HIGH");
                } else if (sensor_status == CURRENT_SENSOR_UNDER_RANGE) {
                    OLED_ShowString(1, 11, "LOW ");
                } else {
                    OLED_ShowString(1, 11, "OK  ");
                }
                
                // 串口输出详细信息（降低频率避免过多输出）
                static uint8_t uart_counter = 0;
                if (uart_counter++ >= 10) { // 每秒输出一次
                    uart_counter = 0;
                    sprintf(msg, "I=%.3fA(avg), V=%.3fV, ADC=%d, Offset=%.3fV\r\n", 
                           current_value, g_current_data.voltage_raw, 
                           g_current_data.adc_raw, user_current_get_zero_offset());
                    user_uart_send_string(msg);
                }
                
            } else {
                // 传感器错误
                OLED_ShowString(2, 1, "ERROR   ");
                OLED_ShowString(4, 1, "CHECK   ");
                user_uart_send_string("Sensor reading error!\r\n");
            }
            
            last_display_time = system_time_ms;
        }
        }
        
        // LED闪烁指示系统运行
        static uint32_t led_toggle_time = 0;
        if (system_time_ms - led_toggle_time >= 500) {
            DL_GPIO_togglePins(LED_PORT, LED_LED1_PIN);
            led_toggle_time = system_time_ms;
        }
        
        // 短暂延时，避免CPU占用过高
        delay_ms(1);
    }
}
