#ifndef USER_DELAY_H_
#define USER_DELAY_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 全局变量声明
 */
extern volatile unsigned int delay_times;
extern volatile uint32_t system_time_ms;  // 系统时间计数器（毫秒）

/*
 * 函数声明
 */

/**
 * @brief 毫秒级精确延时函数
 * @param ms 延时时间，单位：毫秒
 * @note 基于SysTick定时器实现，需要先初始化SysTick
 */
void delay_ms(unsigned int ms);

/**
 * @brief 微秒级延时函数
 * @param us 延时时间，单位：微秒
 * @note 基于CPU指令周期实现，适用于短时间延时
 */
void delay_us(unsigned int us);

/**
 * @brief CPU指令周期延时函数
 * @param cycles 延时的CPU周期数
 */
void delay_cycles(unsigned int cycles);

/**
 * @brief 获取系统运行时间
 * @return 系统运行时间，单位：毫秒
 */
uint32_t get_system_time_ms(void);


/**
 * @brief SysTick定时器中断服务函数
 * @note 每1ms触发一次，用于delay_ms计时
 */
void SysTick_Handler(void);


#ifdef __cplusplus
}
#endif

#endif /* USER_DELAY_H_ */