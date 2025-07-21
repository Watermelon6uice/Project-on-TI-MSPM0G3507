/**
 * @file myINA226.h
 * @brief 双INA226电流电压传感器管理库头文件
 * @author watermelon6uice
 * @details 
 * 本库封装了两个INA226传感器的管理，分别用于读取输入端和输出端的电压、电流值。
 * 主要功能包括：
 * - 初始化两个INA226传感器（不同I2C地址）
 * - 读取输入端和输出端的电压、电流值
 * - 计算输入和输出功率
 * - 计算效率
 * - 提供UI更新接口
 * @date 2025-07-13
 */

#ifndef MY_INA226_H
#define MY_INA226_H

#include <Arduino.h>
#include <Wire.h>
#include "INA226_WE.h"
#include "gui_guider.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// INA226传感器配置
#define INA226_INPUT_ADDR   0x40    // 输入端INA226地址
#define INA226_OUTPUT_ADDR  0x41    // 输出端INA226地址

// 输入端I2C引脚配置（I2C0）
#define INPUT_I2C_SDA_PIN   20      // 输入端I2C数据线引脚
#define INPUT_I2C_SCL_PIN   18      // 输入端I2C时钟线引脚

// 输出端I2C引脚配置（I2C1）
#define OUTPUT_I2C_SDA_PIN  48      // 输出端I2C数据线引脚
#define OUTPUT_I2C_SCL_PIN  47      // 输出端I2C时钟线引脚

#define SHUNT_RESISTOR_OHMS 0.005f  // 分流电阻值（欧姆）
#define MAX_CURRENT_A       10.0f   // 最大电流（安培）

// 调试开关
#define INA226_DEBUG                    // 启用调试输出
//#define INA226_DETAILED_DEBUG         // 启用详细调试输出

class MyINA226 {
private:
    INA226_WE* inputSensor;     // 输入端传感器
    INA226_WE* outputSensor;    // 输出端传感器
    
    TwoWire* inputI2C;          // 输入端I2C总线
    TwoWire* outputI2C;         // 输出端I2C总线
    
    // 当前测量值
    float inputVoltage;         // 输入电压 (V)
    float inputCurrent;         // 输入电流 (A)
    float inputPower;           // 输入功率 (W)
    
    float outputVoltage;        // 输出电压 (V)
    float outputCurrent;        // 输出电流 (A)
    float outputPower;          // 输出功率 (W)
    
    float efficiency;           // 效率 (%)
    
    // 互斥量保护数据
    SemaphoreHandle_t dataMutex;
    
    // 初始化状态
    bool initialized;
    bool inputSensorOK;
    bool outputSensorOK;
    
public:
    /**
     * @brief 构造函数
     */
    MyINA226();
    
    /**
     * @brief 析构函数
     */
    ~MyINA226();
    
    /**
     * @brief 初始化INA226传感器
     * @return true 初始化成功，false 初始化失败
     */
    bool init();
    
    /**
     * @brief 更新所有传感器数据
     * @return true 更新成功，false 更新失败
     */
    bool update();
    
    /**
     * @brief 更新UI显示
     */
    void updateUI();
    
    /**
     * @brief 获取输入电压
     * @return 输入电压值 (V)
     */
    float getInputVoltage();
    
    /**
     * @brief 获取输入电流
     * @return 输入电流值 (A)
     */
    float getInputCurrent();
    
    /**
     * @brief 获取输入功率
     * @return 输入功率值 (W)
     */
    float getInputPower();
    
    /**
     * @brief 获取输出电压
     * @return 输出电压值 (V)
     */
    float getOutputVoltage();
    
    /**
     * @brief 获取输出电流
     * @return 输出电流值 (A)
     */
    float getOutputCurrent();
    
    /**
     * @brief 获取输出功率
     * @return 输出功率值 (W)
     */
    float getOutputPower();
    
    /**
     * @brief 获取效率
     * @return 效率值 (%)
     */
    float getEfficiency();
    
    /**
     * @brief 检查初始化状态
     * @return true 已初始化，false 未初始化
     */
    bool isInitialized();
    
    /**
     * @brief 检查输入传感器状态
     * @return true 输入传感器正常，false 输入传感器异常
     */
    bool isInputSensorOK();
    
    /**
     * @brief 检查输出传感器状态
     * @return true 输出传感器正常，false 输出传感器异常
     */
    bool isOutputSensorOK();
    
    /**
     * @brief 重置传感器
     */
    void reset();
    
private:
    /**
     * @brief 计算效率
     */
    void calculateEfficiency();
    
    /**
     * @brief 格式化电压显示字符串
     * @param voltage 电压值
     * @param buffer 输出缓冲区
     * @param bufferSize 缓冲区大小
     */
    void formatVoltage(float voltage, char* buffer, size_t bufferSize);
    
    /**
     * @brief 格式化电流显示字符串
     * @param current 电流值
     * @param buffer 输出缓冲区
     * @param bufferSize 缓冲区大小
     */
    void formatCurrent(float current, char* buffer, size_t bufferSize);
    
    /**
     * @brief 格式化功率显示字符串
     * @param power 功率值
     * @param buffer 输出缓冲区
     * @param bufferSize 缓冲区大小
     */
    void formatPower(float power, char* buffer, size_t bufferSize);
    
    /**
     * @brief 格式化效率显示字符串
     * @param efficiency 效率值
     * @param buffer 输出缓冲区
     * @param bufferSize 缓冲区大小
     */
    void formatEfficiency(float efficiency, char* buffer, size_t bufferSize);
};

// 全局实例声明
extern MyINA226* ina226;

#endif // MY_INA226_H
