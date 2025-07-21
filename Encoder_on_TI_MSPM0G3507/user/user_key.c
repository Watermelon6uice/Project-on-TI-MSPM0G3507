#include "ti_msp_dl_config.h"
#include "user_key.h"
#include "delay.h"
#include "user_Encoder.h"  // 添加编码器头文件

/*
 * 全局变量定义
 */
volatile bool key_pressed_flag = false;  // 按键按下标志
volatile bool led_fast_mode = false;     // LED闪烁模式：false=慢闪（默认），true=快闪

/**
 * @brief 初始化按键GPIO和中断
 * @note 按键已在ti_msp_dl_config中配置，这里主要是启用NVIC中断
 */
void user_key_init(void)
{
    // 清除可能存在的中断标志
    DL_GPIO_clearInterruptStatus(KEY_PORT, KEY_KEY1_PIN);
    
    // 启用GROUP1中断（按键属于GROUP1）
    NVIC_EnableIRQ(KEY_INT_IRQN);
}

/**
 * @brief 按键处理函数
 * @note 在主循环中调用，处理按键按下事件
 */
void user_key_process(void)
{
    static uint32_t last_key_time = 0;  // 上次按键处理时间
    extern volatile uint32_t system_time_ms;  // 引用主程序中的系统时间
    
    if (key_pressed_flag)
    {
        // 防抖处理：确保两次按键间隔至少50ms
        if ((system_time_ms - last_key_time) >= 50) {
            key_pressed_flag = false;  // 清除标志
            
            // 切换LED闪烁模式
            led_fast_mode = !led_fast_mode;
            
            // 更新按键处理时间
            last_key_time = system_time_ms;
        } else {
            // 如果在防抖时间内，只清除标志，不处理按键
            key_pressed_flag = false;
        }
    }
}

/**
 * @brief 获取当前LED延时时间
 * @return 当前模式下的延时时间（毫秒）
 */
uint32_t get_led_delay(void)
{
    return led_fast_mode ? LED_FAST_DELAY_MS : LED_SLOW_DELAY_MS;
}

/**
 * @brief GROUP1中断服务函数
 * @note 按键和编码器共享GROUP1中断，需要区分处理
 */
void GROUP1_IRQHandler(void)
{
    // 读取Group1的中断寄存器并检查中断源
    switch(DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1))
    {
        // 检查是否是按键的GPIO中断 (GPIOB)
        case KEY_INT_IIDX:
            // 检查具体是哪个GPIO引脚触发中断
            switch (DL_GPIO_getPendingInterrupt(KEY_PORT))
            {
                case KEY_KEY1_IIDX:
                    // 按键按下时设置标志（下降沿触发）
                    key_pressed_flag = true;
                    break;
                
                default:
                    break;
            }
            break;
            
        // 检查是否是编码器的GPIO中断 (GPIOA)
        case ENCODER_INT_IIDX:
            {
                uint32_t interrupts = DL_GPIO_getEnabledInterruptStatus(GPIOA, 
                    ENCODER_PIN_A | ENCODER_PIN_B);
                
                if (interrupts != 0) {
                    // 只处理PIN_A中断 (PA17) - 避免重复计数
                    if (interrupts & ENCODER_PIN_A) {
                        // 延时消抖 - 100us提高稳定性
                        delay_us(100);
                        
                        // 读取引脚状态确保稳定
                        uint8_t pin_a = DL_GPIO_readPins(ENCODER_PIN_A_PORT, ENCODER_PIN_A) ? 1 : 0;
                        uint8_t pin_b = DL_GPIO_readPins(ENCODER_PIN_B_PORT, ENCODER_PIN_B) ? 1 : 0;
                        
                        // 只有在A引脚确实为低电平时才处理（确认下降沿）
                        if (pin_a == 0) {
                            // 根据PIN_B状态判断方向
                            if (pin_b == 0) {
                                // PIN_B为低电平，逆时针旋转，计数减1
                                g_encoder_state.count--;
                            } else {
                                // PIN_B为高电平，顺时针旋转，计数加1
                                g_encoder_state.count++;
                            }
                            
                            // 限制计数范围
                            if (g_encoder_state.count < ENCODER_COUNT_MIN) {
                                g_encoder_state.count = ENCODER_COUNT_MIN;
                            }
                            if (g_encoder_state.count > ENCODER_COUNT_MAX) {
                                g_encoder_state.count = ENCODER_COUNT_MAX;
                            }
                            
                            // 设置变化标志
                            if (g_encoder_state.count != g_encoder_state.last_count) {
                                g_encoder_state.count_changed = true;
                                g_encoder_state.last_count = g_encoder_state.count;
                                
                                // 更新电压和DAC值
                                g_encoder_state.target_voltage = user_encoder_count_to_voltage(g_encoder_state.count);
                                g_encoder_state.dac_value = user_encoder_voltage_to_dac(g_encoder_state.target_voltage);
                            }
                        }
                        
                        // 清除中断标志
                        DL_GPIO_clearInterruptStatus(ENCODER_PIN_A_PORT, ENCODER_PIN_A);
                    }
                    
                    // PIN_B中断处理 - 只清除中断标志，不处理逻辑
                    if (interrupts & ENCODER_PIN_B) {
                        // 简单的清除中断标志，避免重复计数
                        DL_GPIO_clearInterruptStatus(ENCODER_PIN_B_PORT, ENCODER_PIN_B);
                    }
                }
            }
            break;
        
        default:
            break;
    }
}

