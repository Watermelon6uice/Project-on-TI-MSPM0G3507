#include "ti_msp_dl_config.h"
#include "delay.h"

volatile unsigned int delay_times = 0;

// 声明外部的系统时间计数器
extern volatile uint32_t system_time_ms;


/**
 * @brief 毫秒级精确延时函数
 * @param ms 延时时间，单位：毫秒
 * @note 基于SysTick定时器实现，搭配中断服务函数使用
 */
void delay_ms(unsigned int ms)
{
    if (ms == 0) return;  // 防止0延时导致死循环
    
    delay_times = ms;
    while(delay_times != 0)
    {
        // 等待SysTick中断递减delay_times
        // 可以在这里添加低功耗模式进入代码
        __WFI();  // 等待中断，降低功耗
    }
}


/**
 * @brief SysTick定时器中断服务函数
 * @note 每1ms触发一次，用于delay_ms函数的计时
 */
void SysTick_Handler(void)
{
    if(delay_times != 0)
    {
        delay_times--;
    }
    
    // 同时更新系统时间计数器
    system_time_ms++;
}