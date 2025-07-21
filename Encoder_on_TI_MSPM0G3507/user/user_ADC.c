#include "user_ADC.h"
#include "delay.h"
#include "firewater_protocol.h"  // 引入firewater协议
#include <string.h>

// 全局变量定义
volatile bool gCheckADC = false;  // ADC采集成功标志位

// 高频采样相关全局变量
static volatile bool g_adc_sampling_active = false;    // 采样激活标志
static uint32_t g_sample_rate = 0;                     // 采样频率
static uint32_t g_adc_sample_counter = 0;              // 采样计数器
static float g_voltage_buffer[ADC_MAX_BATCH_SIZE];     // 电压数据缓冲区
static uint8_t g_buffer_index = 0;                     // 缓冲区索引
static uint8_t g_batch_size = 10;                      // 批处理大小

/**
 * @brief 初始化ADC模块
 */
void user_adc_init(void)
{
    // 清除ADC标志位
    gCheckADC = false;
    
    // 清除任何待处理的中断状态
    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    
    // 确保ADC已启用
    DL_ADC12_enablePower(ADC12_0_INST);
    
    // 等待ADC稳定
    delay_ms(1);
    
    // 启用ADC转换
    DL_ADC12_enableConversions(ADC12_0_INST);
    
    // 启用ADC中断
    NVIC_ClearPendingIRQ(ADC12_0_INST_INT_IRQN);
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
}

/**
 * @brief 读取ADC原始值（正确的中断模式实现）
 * @param channel ADC通道
 * @param value 输出的原始值指针
 * @return ADC状态
 */
adc_status_t user_adc_read_raw(adc_channel_t channel, uint16_t *value)
{
    if (value == NULL || channel >= ADC_CHANNEL_MAX) {
        return ADC_STATUS_ERROR;
    }
    
    // 清除中断状态和标志位
    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    gCheckADC = false;
    
    // 在单次采集模式下，重新启用转换
    DL_ADC12_enableConversions(ADC12_0_INST);
    
    // 软件触发ADC开始转换
    DL_ADC12_startConversion(ADC12_0_INST);
    
    // 如果当前状态为正在转换中则等待转换结束
    while (false == gCheckADC) {
        __WFE();
    }
    
    // 获取数据
    *value = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_ADC_CH0);
    
    // 清除标志位
    gCheckADC = false;
    
    return ADC_STATUS_OK;
}

/**
 * @brief 读取ADC电压值
 * @param channel ADC通道
 * @param voltage 输出的电压值指针（单位：V）
 * @return ADC状态
 */
adc_status_t user_adc_read_voltage(adc_channel_t channel, float *voltage)
{
    if (voltage == NULL) {
        return ADC_STATUS_ERROR;
    }
    
    uint16_t raw_value;
    adc_status_t status = user_adc_read_raw(channel, &raw_value);
    
    if (status == ADC_STATUS_OK) {
        *voltage = user_adc_raw_to_voltage(raw_value);
    }
    
    return status;
}

/**
 * @brief 读取ADC平均电压值
 * @param channel ADC通道
 * @param voltage 输出的电压值指针（单位：V）
 * @param samples 采样次数
 * @return ADC状态
 */
adc_status_t user_adc_read_voltage_average(adc_channel_t channel, float *voltage, uint8_t samples)
{
    if (voltage == NULL || samples == 0) {
        return ADC_STATUS_ERROR;
    }
    
    uint32_t sum = 0;
    uint16_t raw_value;
    uint8_t valid_samples = 0;
    
    for (uint8_t i = 0; i < samples; i++) {
        if (user_adc_read_raw(channel, &raw_value) == ADC_STATUS_OK) {
            sum += raw_value;
            valid_samples++;
        }
        
        // 采样间隔，使用毫秒延时
        delay_ms(1);
    }
    
    if (valid_samples == 0) {
        return ADC_STATUS_ERROR;
    }
    
    // 计算平均值并转换为电压
    uint16_t average_raw = sum / valid_samples;
    *voltage = user_adc_raw_to_voltage(average_raw);
    
    return ADC_STATUS_OK;
}

/**
 * @brief 将ADC原始值转换为毫伏
 * @param raw_value ADC原始值
 * @return 毫伏值
 */
uint16_t user_adc_raw_to_millivolts(uint16_t raw_value)
{
    // 计算: (raw_value / 4095) * 3300mV
    return (uint16_t)((uint32_t)raw_value * 3300 / ADC_MAX_VALUE);
}

/**
 * @brief 将ADC原始值转换为电压（V）
 * @param raw_value ADC原始值
 * @return 电压值（V）
 */
float user_adc_raw_to_voltage(uint16_t raw_value)
{
    // 计算: (raw_value / 4095) * 3.3V
    return ((float)raw_value * ADC_REFERENCE_VOLTAGE) / (float)ADC_MAX_VALUE;
}

/**
 * @brief ADC中断服务函数（按照配置文件的正确名称：ADC0_IRQHandler）
 */
void ADC0_IRQHandler(void)
{
    // 查询并清除ADC中断
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST))
    {
        // 检查是否完成数据采集
        case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
            gCheckADC = true;//将标志位置1
            break;
        default:
            break;
    }
}

/**
 * @brief 测试ADC中断功能
 */
void user_adc_test_interrupt(void)
{
    // 清除中断状态和标志位
    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    gCheckADC = false;
    
    // 在单次采集模式下，重新启用转换
    DL_ADC12_enableConversions(ADC12_0_INST);
    
    // 手动触发ADC转换，测试中断是否正常工作
    DL_ADC12_startConversion(ADC12_0_INST);
    
    // 等待一段时间看中断是否触发
    for(volatile int i = 0; i < 500000; i++);
    
    // 如果中断没有触发，手动清除状态为下次做准备
    if (!gCheckADC) {
        DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    }
}

/**
 * @brief 检查ADC是否就绪
 */
bool user_adc_is_ready(void)
{
    return gCheckADC;
}

/**
 * @brief 轮询模式读取ADC原始值（备选方案）
 * @param channel ADC通道
 * @param value 输出的原始值指针
 * @return ADC状态
 */
adc_status_t user_adc_read_raw_polling(adc_channel_t channel, uint16_t *value)
{
    if (value == NULL || channel >= ADC_CHANNEL_MAX) {
        return ADC_STATUS_ERROR;
    }
    
    // 清除任何待处理的中断状态
    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    
    // 在单次采集模式下，重新启用转换
    DL_ADC12_enableConversions(ADC12_0_INST);
    
    // 软件触发ADC开始转换
    DL_ADC12_startConversion(ADC12_0_INST);
    
    // 轮询等待转换完成（不使用中断）
    uint32_t timeout = 100000;
    while (timeout > 0) {
        // 检查转换是否完成
        if (DL_ADC12_getRawInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED)) {
            // 转换完成，清除中断状态
            DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
            
            // 读取转换结果
            *value = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_ADC_CH0);
            
            return ADC_STATUS_OK;
        }
        timeout--;
        // 短暂延时
        for(volatile int i = 0; i < 10; i++);
    }
    
    return ADC_STATUS_ERROR;
}

/**
 * @brief 高速ADC读取函数（专为高频采样优化）
 * @param value 输出的原始值指针
 * @return ADC状态
 */
adc_status_t user_adc_read_raw_fast(uint16_t *value)
{
    if (value == NULL) {
        return ADC_STATUS_ERROR;
    }
    
    // 直接启动转换，减少清除操作的开销
    DL_ADC12_enableConversions(ADC12_0_INST);
    DL_ADC12_startConversion(ADC12_0_INST);
    
    // 更短的超时时间，更紧凑的轮询
    uint32_t timeout = 10000;
    while (timeout > 0) {
        if (DL_ADC12_getRawInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED)) {
            DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
            *value = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_ADC_CH0);
            return ADC_STATUS_OK;
        }
        timeout--;
    }
    
    return ADC_STATUS_ERROR;
}

/**
 * @brief 开始高频ADC采样
 * @param sample_rate_hz 采样频率(Hz)
 */
void user_adc_start_high_speed_sampling(uint32_t sample_rate_hz)
{
    if (sample_rate_hz > ADC_MAX_SAMPLE_RATE) {
        sample_rate_hz = ADC_MAX_SAMPLE_RATE;
    }
    
    g_sample_rate = sample_rate_hz;
    g_adc_sampling_active = true;
    g_adc_sample_counter = 0;
    g_buffer_index = 0;
    
    // 清空缓冲区
    memset((void*)g_voltage_buffer, 0, sizeof(g_voltage_buffer));
}

/**
 * @brief 停止高频ADC采样
 */
void user_adc_stop_high_speed_sampling(void)
{
    g_adc_sampling_active = false;
    
    // 发送剩余的缓冲数据
    if (g_buffer_index > 0) {
        firewater_send_adc_batch(g_voltage_buffer, g_buffer_index, 
                                 g_adc_sample_counter - g_buffer_index);
        g_buffer_index = 0;
    }
}

/**
 * @brief 检查是否正在采样
 */
bool user_adc_is_sampling(void)
{
    return g_adc_sampling_active;
}

/**
 * @brief 设置批处理大小
 */
void user_adc_set_batch_size(uint8_t batch_size)
{
    if (batch_size > 0 && batch_size <= ADC_MAX_BATCH_SIZE) {
        g_batch_size = batch_size;
    }
}

/**
 * @brief 高频采样处理函数（优化版本，减少开销）
 * 需要在定时器中断或主循环中高频调用
 */
void user_adc_high_speed_process(void)
{
    if (!g_adc_sampling_active) {
        return;
    }
    
    // 使用专门的高速读取函数
    uint16_t raw_value;
    if (user_adc_read_raw_fast(&raw_value) == ADC_STATUS_OK) {
        float voltage = user_adc_raw_to_voltage(raw_value);
        
        // 添加到缓冲区
        g_voltage_buffer[g_buffer_index] = voltage;
        g_buffer_index++;
        g_adc_sample_counter++;
        
        // 当缓冲区满时，发送数据
        if (g_buffer_index >= g_batch_size) {
            firewater_send_adc_batch(g_voltage_buffer, g_buffer_index, 
                                     g_adc_sample_counter - g_buffer_index);
            g_buffer_index = 0;
        }
    }
}
