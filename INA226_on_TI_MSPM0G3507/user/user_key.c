#include "ti_msp_dl_config.h"
#include "user_key.h"
#include "delay.h"

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
 * @note 按键中断处理函数，使用TI的Group中断方式
 */
void GROUP1_IRQHandler(void)
{
    // 读取Group1的中断寄存器并检查中断源
    switch(DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1))
    {
        // 检查是否是按键的GPIO中断
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
        
        default:
            break;
    }
}

