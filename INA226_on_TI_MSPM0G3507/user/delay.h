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
 * @brief SysTick定时器中断服务函数
 * @note 每1ms触发一次，用于delay_ms计时
 */
void SysTick_Handler(void);


#ifdef __cplusplus
}
#endif

#endif /* USER_DELAY_H_ */