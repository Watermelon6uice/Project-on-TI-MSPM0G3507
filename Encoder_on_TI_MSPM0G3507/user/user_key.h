#ifndef USER_KEY_H_
#define USER_KEY_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 宏定义
 */
#define LED_SLOW_DELAY_MS   500    // 慢闪延时：500ms
#define LED_FAST_DELAY_MS   100    // 快闪延时：100ms

/*
 * 全局变量声明
 */
extern volatile bool key_pressed_flag;   // 按键按下标志
extern volatile bool led_fast_mode;      // LED闪烁模式

/*
 * 函数声明
 */

/**
 * @brief 初始化按键GPIO和中断
 */
void user_key_init(void);

/**
 * @brief 按键处理函数
 */
void user_key_process(void);

/**
 * @brief 获取当前LED延时时间
 * @return 当前模式下的延时时间（毫秒）
 */
uint32_t get_led_delay(void);

/**
 * @brief GROUP1中断服务函数
 */
void GROUP1_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_KEY_H_ */
