#include "user_DAC.h"
#include "user_uart.h"
#include "delay.h"
#include <stdio.h>

// 正弦波数据表
static uint16_t sine_table[SINE_TABLE_SIZE];
static volatile uint32_t sine_index = 0;
static volatile bool dac_running = false;
static volatile uint32_t interrupt_count = 0;  // 添加中断计数器用于调试

/**
 * @brief 初始化DAC
 */
void DAC_init(void)
{
    // 生成正弦波表
    DAC_generateSineTable();
    
    // 重置状态
    sine_index = 0;
    dac_running = false;
    
    // 清除中断标志
    DL_DAC12_clearInterruptStatus(DAC_INST, DL_DAC12_INTERRUPT_FIFO_ONE_QTR_EMPTY);
    
    // NVIC中断配置（确保NVIC层面启用）
    NVIC_ClearPendingIRQ(DAC12_INT_IRQN);
    NVIC_EnableIRQ(DAC12_INT_IRQN);
    
    // 注意：DAC和中断在SYSCFG_DL_init()中已经初始化和启用
    // 这里只需要确保是启用状态
    DL_DAC12_enable(DAC_INST);
    
    user_uart_send_string("DAC: Initialization complete\r\n");
}

/**
 * @brief 生成正弦波数据表 (0V到3.3V，即0到4095)
 */
void DAC_generateSineTable(void)
{
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        // 生成0到3.3V的正弦波（全正值）
        // sin(2πi/N) 范围是-1到1，转换为0到1，再映射到0-4095
        float angle = 2.0f * M_PI * i / SINE_TABLE_SIZE;
        float sine_value = (sin(angle) + 1.0f) / 2.0f;  // 转换为0-1范围
        sine_table[i] = (uint16_t)(sine_value * DAC_MAX_VALUE);
    }
}

/**
 * @brief 启动正弦波输出
 */
void DAC_startSineWave(void)
{
    if (dac_running) {
        return; // 已经在运行
    }
    
    sine_index = 0;
    dac_running = true;
    
    user_uart_send_string("DAC: Starting sine wave\r\n");
    
    // 基本输出测试
    DL_DAC12_output12(DAC_INST, 2048);
    user_uart_send_string("DAC: Basic test output\r\n");
    delay_ms(10);
    
    // 清除中断标志
    DL_DAC12_clearInterruptStatus(DAC_INST, DL_DAC12_INTERRUPT_FIFO_ONE_QTR_EMPTY);
    user_uart_send_string("DAC: Cleared interrupts\r\n");
    
    // 填充FIFO初始数据
    user_uart_send_string("DAC: Filling FIFO\r\n");
    for (int i = 0; i < 4 && i < SINE_TABLE_SIZE; i++) {
        if (!DL_DAC12_isFIFOFull(DAC_INST)) {
            DL_DAC12_output12(DAC_INST, sine_table[sine_index]);
            sine_index = (sine_index + 1) % SINE_TABLE_SIZE;
        }
    }
    user_uart_send_string("DAC: FIFO filled\r\n");
    
    // 确保中断启用（即使配置中已设置）
    DL_DAC12_enableInterrupt(DAC_INST, DL_DAC12_INTERRUPT_FIFO_ONE_QTR_EMPTY);
    user_uart_send_string("DAC: Interrupt enabled\r\n");
    
    // 启动采样定时器（即使配置中已设置）
    DL_DAC12_enableSampleTimeGenerator(DAC_INST);
    user_uart_send_string("DAC: Sample timer started\r\n");
    
    user_uart_send_string("DAC: Auto mode ready\r\n");
}

/**
 * @brief 停止正弦波输出
 */
void DAC_stopSineWave(void)
{
    if (!dac_running) {
        return; // 已经停止
    }
    
    // 停止采样定时器
    DL_DAC12_disableSampleTimeGenerator(DAC_INST);
    
    // 禁用中断
    DL_DAC12_disableInterrupt(DAC_INST, DL_DAC12_INTERRUPT_FIFO_ONE_QTR_EMPTY);
    
    // 等待FIFO处理完成（简单延时）
    for (volatile int i = 0; i < 1000; i++) {
        // 简单延时等待
    }
    
    // 输出0V
    DL_DAC12_output12(DAC_INST, 0);
    
    dac_running = false;
    user_uart_send_string("DAC: Auto sine wave stopped\r\n");
}

/**
 * @brief 检查DAC状态（调试用）
 */
void DAC_checkStatus(void)
{
    char msg[100];
    
    bool dac_enabled = DL_DAC12_isEnabled(DAC_INST);
    bool fifo_full = DL_DAC12_isFIFOFull(DAC_INST);
    bool sample_timer_enabled = DL_DAC12_isSampleTimeGeneratorEnabled(DAC_INST);
    
    snprintf(msg, sizeof(msg), "DAC Status: EN=%d, FIFO_FULL=%d, TIMER=%d, INT_CNT=%u\r\n", 
             dac_enabled, fifo_full, sample_timer_enabled, (unsigned int)interrupt_count);
    user_uart_send_string(msg);
}

/**
 * @brief 手动更新DAC FIFO（用于备份模式）
 */
void DAC_manualUpdate(void)
{
    if (!dac_running) {
        return;
    }
    
    // 如果FIFO不满，填充数据
    while (!DL_DAC12_isFIFOFull(DAC_INST)) {
        DL_DAC12_output12(DAC_INST, sine_table[sine_index]);
        sine_index = (sine_index + 1) % SINE_TABLE_SIZE;
    }
}

/**
 * @brief 设置正弦波频率（通过调整采样率）
 * @param sample_rate 采样率 (Hz)
 */
void DAC_setSineFrequency(uint32_t sample_rate)
{
    // 注意：这个函数需要重新配置采样定时器
    // 对于当前的8kHz固定配置，这个函数暂时不实现
    // 如果需要动态调整频率，需要重新配置DAC的采样定时器
    (void)sample_rate; // 避免未使用参数警告
}

/**
 * @brief DAC中断处理函数（使用生成的函数名）
 */
void DAC0_IRQHandler(void)
{
    // 检查是否是FIFO 1/4空中断
    uint32_t status = DL_DAC12_getInterruptStatus(DAC_INST, DL_DAC12_INTERRUPT_FIFO_ONE_QTR_EMPTY);
    if (status != 0) {
        interrupt_count++;  // 增加中断计数
        
        // FIFO 1/4空时，补充数据
        if (dac_running) {
            // 填充FIFO直到满或者没有更多数据
            while (!DL_DAC12_isFIFOFull(DAC_INST)) {
                DL_DAC12_output12(DAC_INST, sine_table[sine_index]);
                sine_index = (sine_index + 1) % SINE_TABLE_SIZE;
            }
        }
        // 清除中断标志
        DL_DAC12_clearInterruptStatus(DAC_INST, DL_DAC12_INTERRUPT_FIFO_ONE_QTR_EMPTY);
        
        // 每100次中断打印一次状态（仅用于调试）
        if (interrupt_count % 100 == 0) {
            // 在实际应用中，应避免在中断中使用串口
            // 这里仅用于调试验证中断是否工作
        }
    }
}
