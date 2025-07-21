#include "ti_msp_dl_config.h"
#include "user/delay.h"
#include "user/user_uart.h"
#include "user/firewater_protocol.h"
#include "user/user_ADC.h"
#include "user/user_DAC.h"
#include "user/user_OLED.h"
#include "user/user_Encoder.h"
#include <stdio.h>

// 全局时间计数器（毫秒）
volatile uint32_t system_time_ms = 0;

int main(void)
{
    char msg[100];
    
    // 系统初始化
    SYSCFG_DL_init();
    
    // 初始化各个模块
    user_uart_init();
    delay_ms(100);  // 等待UART稳定
    
    user_uart_send_string("\r\n=== DAC Sine Wave System Starting ===\r\n");
    
    // 初始化OLED显示屏
    user_uart_send_string("Initializing OLED...\r\n");
    OLED_Init();
    
    // 设置固定显示标签（只设置一次）
    OLED_ShowString(1, 1, "Encoder DAC");
    OLED_ShowString(2, 1, "MSPM0G3507");
    OLED_ShowString(3, 1, "V: 0.000 V");  // 简化格式："V: X.XXX V"
    OLED_ShowString(4, 1, "Count: 16");
    
    user_uart_send_string("OLED initialized\r\n");
    
    user_adc_init();
    user_uart_send_string("ADC initialized\r\n");
    
    DAC_init();
    user_uart_send_string("DAC initialized\r\n");
    
    // 初始化编码器
    user_encoder_init();
    user_uart_send_string("Encoder initialized\r\n");
    
    // 设置初始DAC输出电压(1.6V)
    user_encoder_update_dac();
    user_uart_send_string("Initial DAC voltage set to 1.6V\r\n");;
    
    // 系统就绪指示
    DL_GPIO_setPins(LED_PORT, LED_LED1_PIN);
    user_uart_send_string("System ready - Encoder DAC control active\r\n");
    
    // 发送初始状态信息
    char init_msg[100];
    sprintf(init_msg, "Initial: Count=%d, Voltage=%.1fV\r\n", 
            user_encoder_get_count(), g_encoder_state.target_voltage);
    user_uart_send_string(init_msg);
    
    // 主循环：编码器控制DAC，ADC采样和VOFA+显示
    float voltage;
    
    // 非阻塞延时变量 - 独立的更新频率控制
    uint32_t last_encoder_check = 0;
    uint32_t last_oled_update = 0;
    uint32_t last_adc_update = 0;
    uint32_t encoder_check_interval = 5;   // 编码器检查间隔5ms
    uint32_t oled_update_interval = 100;   // OLED更新间隔100ms - 10Hz刷新率
    uint32_t adc_update_interval = 50;     // ADC更新间隔50ms - 降低到20Hz采样率
    
    // 系统启动完成
    uint32_t init_time = get_system_time_ms();
    uint32_t sample_count = 0;
    
    while (1) {
        uint32_t current_time = get_system_time_ms();  // 使用时间获取函数
        
        // 检查编码器状态并更新DAC输出
        if (current_time - last_encoder_check >= encoder_check_interval) {
            
            if (user_encoder_is_changed()) {
                // 编码器计数改变，更新DAC输出
                user_encoder_update_dac();
                
                // 发送状态信息到串口
                char encoder_msg[100];
                sprintf(encoder_msg, "Encoder: Count=%d, Voltage=%.1fV, DAC=%d\r\n", 
                        g_encoder_state.count, g_encoder_state.target_voltage, g_encoder_state.dac_value);
                user_uart_send_string(encoder_msg);
            }
            
            last_encoder_check = current_time;
        }
        
        // ADC采样（按时间间隔执行）
        if (current_time - last_adc_update >= adc_update_interval) {
            if (user_adc_read_voltage(ADC_CHANNEL_0, &voltage) == ADC_STATUS_OK) {
                // 发送到VOFA+显示
                firewater_send_adc_voltage_simple(voltage);
                sample_count++;
            }
            last_adc_update = current_time;
        }
        
        // OLED显示更新
        if (current_time - last_oled_update >= oled_update_interval) {
            static uint8_t oled_update_step = 0;  // 轮换更新步骤
            
            // 计算整数和小数部分用于显示
            uint32_t int_part = (uint32_t)voltage;
            uint32_t dec_part = (uint32_t)((voltage - int_part) * 1000);
            
            // 轮换更新不同的数值，减少单次I2C阻塞时间
            switch(oled_update_step) {
                case 0:
                    // 更新电压整数部分 (第4列，1位)
                    OLED_ShowNum(3, 4, int_part, 1);
                    break;
                case 1:
                    // 更新电压小数部分 (第6列开始，3位)  
                    OLED_ShowNum(3, 6, dec_part, 3);
                    break;
                case 2:
                    // 更新编码器计数显示
                    OLED_ShowNum(4, 8, g_encoder_state.count, 2);
                    break;
            }
            
            oled_update_step = (oled_update_step + 1) % 3;  // 循环3个步骤
            
            last_oled_update = current_time;
        }
    }
}
