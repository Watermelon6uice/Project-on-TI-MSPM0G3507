/**
 * @file myINA226.cpp
 * @brief 双INA226电流电压传感器管理库实现文件
 * @author watermelon6uice
 * @details 
 * 本库实现了两个INA226传感器的管理，分别用于读取输入端和输出端的电压、电流值。
 * 支持I2C通信，自动计算功率和效率，并提供UI更新功能。
 * @date 2025-07-13
 */

#include "myINA226.h"
#include "myTFT.h"

// 全局实例定义
MyINA226* ina226 = nullptr;

// 构造函数
MyINA226::MyINA226() {
    inputSensor = nullptr;
    outputSensor = nullptr;
    inputI2C = nullptr;
    outputI2C = nullptr;
    
    inputVoltage = 0.0f;
    inputCurrent = 0.0f;
    inputPower = 0.0f;
    
    outputVoltage = 0.0f;
    outputCurrent = 0.0f;
    outputPower = 0.0f;
    
    efficiency = 0.0f;
    
    initialized = false;
    inputSensorOK = false;
    outputSensorOK = false;
    
    // 创建互斥量
    dataMutex = xSemaphoreCreateMutex();
}

// 析构函数
MyINA226::~MyINA226() {
    if (inputSensor) {
        delete inputSensor;
    }
    if (outputSensor) {
        delete outputSensor;
    }
    if (inputI2C) {
        delete inputI2C;
    }
    if (outputI2C) {
        delete outputI2C;
    }
    if (dataMutex) {
        vSemaphoreDelete(dataMutex);
    }
}

// 初始化INA226传感器
bool MyINA226::init() {
    Serial.println("开始初始化INA226传感器...");
    
    // 创建输入端I2C总线实例
    inputI2C = new TwoWire(0);  // 使用I2C0
    inputI2C->begin(INPUT_I2C_SDA_PIN, INPUT_I2C_SCL_PIN);
    Serial.printf("输入端I2C初始化完成 (SDA: %d, SCL: %d)\n", INPUT_I2C_SDA_PIN, INPUT_I2C_SCL_PIN);
    
    // 创建输出端I2C总线实例
    outputI2C = new TwoWire(1);  // 使用I2C1
    outputI2C->begin(OUTPUT_I2C_SDA_PIN, OUTPUT_I2C_SCL_PIN);
    Serial.printf("输出端I2C初始化完成 (SDA: %d, SCL: %d)\n", OUTPUT_I2C_SDA_PIN, OUTPUT_I2C_SCL_PIN);
    
    // 扫描输入端I2C设备
    Serial.println("扫描输入端I2C设备...");
    for (byte address = 1; address < 127; address++) {
        inputI2C->beginTransmission(address);
        if (inputI2C->endTransmission() == 0) {
            Serial.printf("输入端发现I2C设备地址: 0x%02X\n", address);
        }
    }
    
    // 扫描输出端I2C设备
    Serial.println("扫描输出端I2C设备...");
    for (byte address = 1; address < 127; address++) {
        outputI2C->beginTransmission(address);
        if (outputI2C->endTransmission() == 0) {
            Serial.printf("输出端发现I2C设备地址: 0x%02X\n", address);
        }
    }
    
    // 创建输入端传感器实例
    inputSensor = new INA226_WE(inputI2C, INA226_INPUT_ADDR);
    if (!inputSensor->init()) {
        Serial.printf("输入端INA226传感器初始化失败 (地址: 0x%02X)\n", INA226_INPUT_ADDR);
        inputSensorOK = false;
    } else {
        Serial.printf("输入端INA226传感器初始化成功 (地址: 0x%02X)\n", INA226_INPUT_ADDR);
        inputSensorOK = true;
        
        // 配置输入端传感器
        inputSensor->setResistorRange(SHUNT_RESISTOR_OHMS, MAX_CURRENT_A);
        inputSensor->setMeasureMode(CONTINUOUS);
        inputSensor->setConversionTime(CONV_TIME_1100);  // 1.1ms转换时间，平衡速度和精度
        inputSensor->setAverage(AVERAGE_64);             // 64次平均，提高精度
        Serial.printf("输入端传感器配置：分流电阻=%.3fΩ, 最大电流=%.1fA\n", SHUNT_RESISTOR_OHMS, MAX_CURRENT_A);
    }
    
    // 创建输出端传感器实例
    outputSensor = new INA226_WE(outputI2C, INA226_OUTPUT_ADDR);
    if (!outputSensor->init()) {
        Serial.printf("输出端INA226传感器初始化失败 (地址: 0x%02X)\n", INA226_OUTPUT_ADDR);
        outputSensorOK = false;
    } else {
        Serial.printf("输出端INA226传感器初始化成功 (地址: 0x%02X)\n", INA226_OUTPUT_ADDR);
        outputSensorOK = true;
        
        // 配置输出端传感器
        outputSensor->setResistorRange(SHUNT_RESISTOR_OHMS, MAX_CURRENT_A);
        outputSensor->setMeasureMode(CONTINUOUS);
        outputSensor->setConversionTime(CONV_TIME_1100);  // 1.1ms转换时间，平衡速度和精度
        outputSensor->setAverage(AVERAGE_64);             // 64次平均，提高精度
        Serial.printf("输出端传感器配置：分流电阻=%.3fΩ, 最大电流=%.1fA\n", SHUNT_RESISTOR_OHMS, MAX_CURRENT_A);
    }
    
    // 检查初始化状态
    initialized = (inputSensorOK || outputSensorOK);
    
    if (initialized) {
        Serial.println("INA226传感器初始化完成");
    } else {
        Serial.println("INA226传感器初始化失败 - 所有传感器都无响应");
    }
    
    return initialized;
}

// 更新所有传感器数据
bool MyINA226::update() {
    if (!initialized) {
        return false;
    }
    
    bool updateSuccess = false;
    
    // 获取互斥量
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        
        // 更新输入端数据
        if (inputSensorOK && inputSensor) {
            float newVoltage = inputSensor->getBusVoltage_V();
            float newCurrent = inputSensor->getCurrent_A();
            float newPower = inputSensor->getBusPower() / 1000.0f; // 转换为瓦特
            
            // 检查数据合理性
            if (newVoltage >= 0 && newVoltage <= 50.0f && newCurrent >= 0 && newCurrent <= MAX_CURRENT_A) {
                inputVoltage = newVoltage;
                inputCurrent = newCurrent;
                inputPower = newPower;
                
                #ifdef INA226_DETAILED_DEBUG
                Serial.printf("输入端 - 电压: %.3fV, 电流: %.3fA, 功率: %.3fW\n", 
                             inputVoltage, inputCurrent, inputPower);
                #endif
                
                updateSuccess = true;
            } else {
                Serial.printf("警告: 输入端数据异常 - 电压: %.3fV, 电流: %.3fA\n", newVoltage, newCurrent);
            }
        }
        
        // 更新输出端数据
        if (outputSensorOK && outputSensor) {
            float newVoltage = outputSensor->getBusVoltage_V();
            float newCurrent = outputSensor->getCurrent_A();
            float newPower = outputSensor->getBusPower() / 1000.0f; // 转换为瓦特
            
            // 检查数据合理性
            if (newVoltage >= 0 && newVoltage <= 50.0f && newCurrent >= 0 && newCurrent <= MAX_CURRENT_A) {
                outputVoltage = newVoltage;
                outputCurrent = newCurrent;
                outputPower = newPower;
                
                #ifdef INA226_DETAILED_DEBUG
                Serial.printf("输出端 - 电压: %.3fV, 电流: %.3fA, 功率: %.3fW\n", 
                             outputVoltage, outputCurrent, outputPower);
                #endif
                
                updateSuccess = true;
            } else {
                Serial.printf("警告: 输出端数据异常 - 电压: %.3fV, 电流: %.3fA\n", newVoltage, newCurrent);
            }
        }
        
        // 计算效率
        calculateEfficiency();
        
        xSemaphoreGive(dataMutex);
    }
    
    return updateSuccess;
}

// 更新UI显示
void MyINA226::updateUI() {
    if (!initialized) {
        return;
    }
    
    char buffer[16];
    
    // 获取互斥量保护数据访问
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        
        // 更新输入电压显示
        formatVoltage(inputVoltage, buffer, sizeof(buffer));
        lv_label_set_text(guider_ui.screen_U_IN, buffer);
        
        // 更新输入电流显示
        formatCurrent(inputCurrent, buffer, sizeof(buffer));
        lv_label_set_text(guider_ui.screen_I_IN, buffer);
        
        // 更新输入功率显示
        formatPower(inputPower, buffer, sizeof(buffer));
        lv_label_set_text(guider_ui.screen_P_IN, buffer);
        
        // 更新输出电压显示
        formatVoltage(outputVoltage, buffer, sizeof(buffer));
        lv_label_set_text(guider_ui.screen_Uout, buffer);
        
        // 更新输出电流显示
        formatCurrent(outputCurrent, buffer, sizeof(buffer));
        lv_label_set_text(guider_ui.screen_Iout, buffer);
        
        // 更新输出功率显示
        formatPower(outputPower, buffer, sizeof(buffer));
        lv_label_set_text(guider_ui.screen_Pout, buffer);
        
        // 更新效率显示
        formatEfficiency(efficiency, buffer, sizeof(buffer));
        lv_label_set_text(guider_ui.screen_Efficiency, buffer);
        
        xSemaphoreGive(dataMutex);
    }
}

// 获取输入电压
float MyINA226::getInputVoltage() {
    float value = 0.0f;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = inputVoltage;
        xSemaphoreGive(dataMutex);
    }
    return value;
}

// 获取输入电流
float MyINA226::getInputCurrent() {
    float value = 0.0f;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = inputCurrent;
        xSemaphoreGive(dataMutex);
    }
    return value;
}

// 获取输入功率
float MyINA226::getInputPower() {
    float value = 0.0f;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = inputPower;
        xSemaphoreGive(dataMutex);
    }
    return value;
}

// 获取输出电压
float MyINA226::getOutputVoltage() {
    float value = 0.0f;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = outputVoltage;
        xSemaphoreGive(dataMutex);
    }
    return value;
}

// 获取输出电流
float MyINA226::getOutputCurrent() {
    float value = 0.0f;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = outputCurrent;
        xSemaphoreGive(dataMutex);
    }
    return value;
}

// 获取输出功率
float MyINA226::getOutputPower() {
    float value = 0.0f;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = outputPower;
        xSemaphoreGive(dataMutex);
    }
    return value;
}

// 获取效率
float MyINA226::getEfficiency() {
    float value = 0.0f;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        value = efficiency;
        xSemaphoreGive(dataMutex);
    }
    return value;
}

// 检查初始化状态
bool MyINA226::isInitialized() {
    return initialized;
}

// 检查输入传感器状态
bool MyINA226::isInputSensorOK() {
    return inputSensorOK;
}

// 检查输出传感器状态
bool MyINA226::isOutputSensorOK() {
    return outputSensorOK;
}

// 重置传感器
void MyINA226::reset() {
    if (inputSensor && inputSensorOK) {
        inputSensor->reset_INA226();
    }
    if (outputSensor && outputSensorOK) {
        outputSensor->reset_INA226();
    }
    
    // 重新初始化
    init();
}

// 计算效率
void MyINA226::calculateEfficiency() {
    if (inputPower > 0.001f) { // 避免除零错误，输入功率大于1mW时才计算
        efficiency = (outputPower / inputPower) * 100.0f;
        
        // 限制效率在合理范围内
        if (efficiency > 100.0f) {
            efficiency = 100.0f;
        } else if (efficiency < 0.0f) {
            efficiency = 0.0f;
        }
    } else {
        efficiency = 0.0f;
    }
}

// 格式化电压显示字符串
void MyINA226::formatVoltage(float voltage, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%.2f", voltage);
}

// 格式化电流显示字符串
void MyINA226::formatCurrent(float current, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%.2f", current);
}

// 格式化功率显示字符串
void MyINA226::formatPower(float power, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%.2f", power);
}

// 格式化效率显示字符串
void MyINA226::formatEfficiency(float efficiency, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%.0f", efficiency);
}
