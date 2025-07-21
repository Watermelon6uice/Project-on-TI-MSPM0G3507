#ifndef USER_DAC_H
#define USER_DAC_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <math.h>

// 正弦波表大小
#define SINE_TABLE_SIZE 256

// DAC分辨率 (12位)
#define DAC_MAX_VALUE   4095

// DAC实例定义（根据生成的配置）
#define DAC_INST        DAC0

// 函数声明
void DAC_init(void);
void DAC_generateSineTable(void);
void DAC_startSineWave(void);
void DAC_stopSineWave(void);
void DAC_setSineFrequency(uint32_t sample_rate);
void DAC_manualUpdate(void);  // 添加手动更新函数
void DAC_checkStatus(void);   // 添加状态检查函数

// 中断处理函数（使用生成的名称）
void DAC0_IRQHandler(void);

#endif /* USER_DAC_H */
