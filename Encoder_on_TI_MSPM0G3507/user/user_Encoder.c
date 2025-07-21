#include "user_Encoder.h"
#include "user/user_DAC.h"
#include "user/delay.h"
#include "user/user_uart.h"

// 编码器状态全局变量
volatile encoder_state_t g_encoder_state = {
    .count = 16,            // 初始值设为中间值(1.6V对应的计数)
    .last_count = 16,
    .count_changed = false,
    .target_voltage = 1.6f, // 初始电压1.6V
    .dac_value = 0
};

// 内部函数声明
static void encoder_limit_count(void);
static void encoder_update_voltage_and_dac(void);

/**
 * @brief 初始化旋转编码器
 * GPIO配置和中断已经在SYSCFG_DL_init()中完成
 */
void user_encoder_init(void)
{
    user_uart_send_string("Encoder initialized\r\n");
    
    // GPIO和中断配置已经在ti_msp_dl_config.c中完成，包括：
    // - PA17和PA24配置为数字输入，启用上拉电阻
    // - 下降沿触发中断
    
    // 重新配置引脚以确保设置正确
    DL_GPIO_initDigitalInputFeatures(ENCODER_PIN_A_IOMUX,
         DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
         DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
         
    DL_GPIO_initDigitalInputFeatures(ENCODER_PIN_B_IOMUX,
         DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
         DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
         
    // 确保下降沿触发设置正确 - 只对PIN_A设置下降沿触发
    DL_GPIO_setUpperPinsPolarity(GPIOA, DL_GPIO_PIN_17_EDGE_FALL);
    
    // 确保中断标志被清除
    DL_GPIO_clearInterruptStatus(ENCODER_PIN_A_PORT, ENCODER_PIN_A);
    DL_GPIO_clearInterruptStatus(ENCODER_PIN_B_PORT, ENCODER_PIN_B);
    
    // 只启用PIN_A的GPIO中断，避免重复触发
    DL_GPIO_enableInterrupt(ENCODER_PIN_A_PORT, ENCODER_PIN_A);
    
    // 手动配置NVIC中断（使用ENCODER_INT_IRQN，即GPIOA_INT_IRQn）
    NVIC_SetPriority(ENCODER_INT_IRQN, 1);  // 设置编码器中断优先级
    NVIC_ClearPendingIRQ(ENCODER_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_INT_IRQN);       // 启用编码器NVIC中断
    
    // 初始化DAC值
    encoder_update_voltage_and_dac();
}

/**
 * @brief 获取编码器当前计数值
 * @return 当前计数值
 */
int16_t user_encoder_get_count(void)
{
    return g_encoder_state.count;
}

/**
 * @brief 检查编码器计数是否发生变化
 * @return true: 计数已改变, false: 计数未改变
 */
bool user_encoder_is_changed(void)
{
    bool changed = g_encoder_state.count_changed;
    if (changed) {
        g_encoder_state.count_changed = false;  // 清除标志
    }
    return changed;
}

/**
 * @brief 将编码器计数值转换为电压值
 * @param count 编码器计数值
 * @return 对应的电压值
 */
float user_encoder_count_to_voltage(int16_t count)
{
    float voltage = count * DAC_VOLTAGE_STEP;
    
    // 限制电压范围
    if (voltage < DAC_VOLTAGE_MIN) voltage = DAC_VOLTAGE_MIN;
    if (voltage > DAC_VOLTAGE_MAX) voltage = DAC_VOLTAGE_MAX;
    
    return voltage;
}

/**
 * @brief 将电压值转换为DAC数字值
 * @param voltage 电压值 (0.0V ~ 3.3V)
 * @return 对应的DAC数字值 (0 ~ 4095)
 */
uint16_t user_encoder_voltage_to_dac(float voltage)
{
    if (voltage < 0.0f) voltage = 0.0f;
    if (voltage > 3.3f) voltage = 3.3f;
    
    uint16_t dac_value = (uint16_t)((voltage / 3.3f) * DAC_MAX_COUNT_VALUE);
    
    return dac_value;
}

/**
 * @brief 更新DAC输出
 * 根据当前编码器计数值更新DAC输出电压
 */
void user_encoder_update_dac(void)
{
    // 更新电压和DAC值
    encoder_update_voltage_and_dac();
    
    // 设置DAC输出值
    DL_DAC12_output12(DAC0, g_encoder_state.dac_value);
}

/**
 * @brief 重置编码器计数值为0
 */
void user_encoder_reset_count(void)
{
    g_encoder_state.count = 0;
    g_encoder_state.last_count = 0;
    g_encoder_state.count_changed = true;
    encoder_update_voltage_and_dac();
}

/**
 * @brief 设置编码器计数值
 * @param count 要设置的计数值
 */
void user_encoder_set_count(int16_t count)
{
    g_encoder_state.count = count;
    encoder_limit_count();
    g_encoder_state.count_changed = true;
    encoder_update_voltage_and_dac();
}

/**
 * @brief 限制编码器计数值在有效范围内
 */
static void encoder_limit_count(void)
{
    if (g_encoder_state.count < ENCODER_COUNT_MIN) {
        g_encoder_state.count = ENCODER_COUNT_MIN;
    }
    if (g_encoder_state.count > ENCODER_COUNT_MAX) {
        g_encoder_state.count = ENCODER_COUNT_MAX;
    }
}

/**
 * @brief 更新电压值和DAC值
 */
static void encoder_update_voltage_and_dac(void)
{
    g_encoder_state.target_voltage = user_encoder_count_to_voltage(g_encoder_state.count);
    g_encoder_state.dac_value = user_encoder_voltage_to_dac(g_encoder_state.target_voltage);
}
