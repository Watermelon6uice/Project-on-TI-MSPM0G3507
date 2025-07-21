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

/**
 * @brief 获取系统运行时间
 * @return 系统运行时间，单位：毫秒
 */
uint32_t get_system_time_ms(void)
{
    return system_time_ms;
}

/**
 * @brief 微秒级延时函数
 * @param us 延时时间，单位：微秒
 * @note 基于CPU指令周期实现，适用于短时间延时
 */
void delay_us(unsigned int us)
{
    // 假设CPU频率为32MHz，每微秒需要32个时钟周期
    // 考虑到函数调用开销，实际周期会略少
    delay_cycles(us * 32);
}

/**
 * @brief CPU指令周期延时函数
 * @param cycles 延时的CPU周期数
 */
void delay_cycles(unsigned int cycles)
{
    // 简单的循环延时，每次循环约消耗几个CPU周期
    // 这个实现可能不够精确，但对于简单延时足够
    volatile unsigned int i;
    for (i = 0; i < cycles / 4; i++) {
        // 空操作，编译器优化时会保留volatile变量的操作
        __asm("nop");
    }
}