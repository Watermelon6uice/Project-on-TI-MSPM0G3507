#include "user_INA226.h"
#include "delay.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>

// I2C实例定义
#define I2C_INST I2C_0_INST

/**
 * @brief 写入INA226寄存器 - 使用SysTick非阻塞时序版本
 */
void ina226_write_register(INA226_Device *device, uint8_t reg, uint16_t val) {
    extern volatile uint32_t system_time_ms;  // 使用全局SysTick时间
    uint8_t tx_data[3];
    tx_data[0] = reg;
    tx_data[1] = (uint8_t)(val >> 8);    // 高字节
    tx_data[2] = (uint8_t)(val & 0xFF);  // 低字节
    
    // 清空TX FIFO
    DL_I2C_flushControllerTXFIFO(I2C_INST);
    
    // 设置目标地址
    DL_I2C_setTargetAddress(I2C_INST, device->i2c_address);
    
    // 填充TX FIFO
    DL_I2C_fillControllerTXFIFO(I2C_INST, tx_data, 3);
    
    // 开始传输
    DL_I2C_startControllerTransfer(I2C_INST, device->i2c_address, 
                                   DL_I2C_CONTROLLER_DIRECTION_TX, 3);
    
    // 等待传输完成（非阻塞）
    while (DL_I2C_getControllerStatus(I2C_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
        // 等待总线空闲
    }
    
    // 非阻塞延时2ms确保传输完成
    uint32_t start_time = system_time_ms;
    while ((system_time_ms - start_time) < 2) {
        // 非阻塞等待
    }
}

/**
 * @brief 读取INA226寄存器 - 使用SysTick非阻塞时序版本
 */
uint16_t ina226_read_register(INA226_Device *device, uint8_t reg) {
    extern volatile uint32_t system_time_ms;  // 使用全局SysTick时间
    uint8_t rx_data[2] = {0};
    device->i2c_error_code = 0;
    
    // 重试机制，最多重试3次
    for (int retry = 0; retry < 3; retry++) {
        // 步骤1：发送寄存器地址
        DL_I2C_flushControllerTXFIFO(I2C_INST);
        DL_I2C_flushControllerRXFIFO(I2C_INST);
        
        // 确保总线空闲（非阻塞等待）
        uint32_t start_time = system_time_ms;
        while ((DL_I2C_getControllerStatus(I2C_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) {
            if ((system_time_ms - start_time) > 10) {  // 10ms超时
                device->i2c_error_code = 2;  // 总线忙超时
                break;
            }
        }
        
        if (device->i2c_error_code != 0) {
            continue;
        }
        
        // 写寄存器地址
        DL_I2C_setTargetAddress(I2C_INST, device->i2c_address);
        DL_I2C_fillControllerTXFIFO(I2C_INST, &reg, 1);
        DL_I2C_startControllerTransfer(I2C_INST, device->i2c_address, 
                                       DL_I2C_CONTROLLER_DIRECTION_TX, 1);
        
        // 等待写操作完成（非阻塞）
        start_time = system_time_ms;
        while ((DL_I2C_getControllerStatus(I2C_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) {
            if ((system_time_ms - start_time) > 10) {  // 10ms超时
                device->i2c_error_code = 3;  // 写操作超时
                break;
            }
        }
        
        if (device->i2c_error_code != 0) {
            continue;
        }
        
        // 非阻塞延时2ms（等待I2C稳定）
        start_time = system_time_ms;
        while ((system_time_ms - start_time) < 2) {
            // 非阻塞等待
        }
        
        // 步骤2：读取16位数据
        DL_I2C_flushControllerRXFIFO(I2C_INST);
        DL_I2C_startControllerTransfer(I2C_INST, device->i2c_address, 
                                       DL_I2C_CONTROLLER_DIRECTION_RX, 2);
        
        // 等待读操作完成（非阻塞）
        start_time = system_time_ms;
        while ((DL_I2C_getControllerStatus(I2C_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) {
            if ((system_time_ms - start_time) > 10) {  // 10ms超时
                device->i2c_error_code = 4;  // 读操作超时
                break;
            }
        }
        
        if (device->i2c_error_code != 0) {
            continue;
        }
        
        // 接收数据（非阻塞）
        uint8_t bytes_received = 0;
        start_time = system_time_ms;
        
        while (bytes_received < 2) {
            if (!DL_I2C_isControllerRXFIFOEmpty(I2C_INST)) {
                rx_data[bytes_received] = DL_I2C_receiveControllerData(I2C_INST);
                bytes_received++;
            } else if ((system_time_ms - start_time) > 10) {  // 10ms超时
                device->i2c_error_code = 5;  // 数据接收不完整
                break;
            }
        }
        
        if (bytes_received == 2) {
            // 成功读取2字节数据
            uint16_t result = ((uint16_t)rx_data[0] << 8) | rx_data[1];
            return result;
        }
        
        // 重试前非阻塞延时
        start_time = system_time_ms;
        while ((system_time_ms - start_time) < 10) {
            // 非阻塞等待
        }
    }
    
    // 3次重试都失败
    return 0;
}

/**
 * @brief 触发单次测量并等待完成 - 按照Arduino库逻辑修复
 */
static void ina226_trigger_and_wait(INA226_Device *device) {
    // 第1步：读取MASK_EN_REG来清除CVRF(转换完成)标志
    ina226_read_register(device, INA226_MASK_EN_REG);
    
    // 第2步：读取配置寄存器然后写回相同值来触发转换
    uint16_t config = ina226_read_register(device, INA226_CONF_REG);
    ina226_write_register(device, INA226_CONF_REG, config);
    
    // 第3步：轮询等待转换完成
    uint16_t conv_ready = 0x0000;
    uint16_t timeout = 0;
    
    while (!conv_ready && timeout < 50) {  // 最多等待50ms
        delay_ms(1);  // 每次等待1ms
        
        // 检查CVRF位（第3位，0x0008）
        uint16_t mask_reg = ina226_read_register(device, INA226_MASK_EN_REG);
        conv_ready = mask_reg & 0x0008;
        
        timeout++;
    }
    
    // 如果超时，设置错误码
    if (timeout >= 50) {
        device->i2c_error_code = 2;  // 转换超时错误
    } else {
        device->i2c_error_code = 0;  // 转换成功
    }
}
    
/**
 * @brief 初始化INA226器件
 */
bool ina226_init(INA226_Device *device, uint8_t i2c_address) {
    if (device == NULL) {
        return false;
    }
    
    // 初始化设备参数
    device->i2c_address = i2c_address;
    device->cal_value = 2048;  // 默认校准值
    device->corr_factor = 1.0;
    device->current_divider_mA = 40.0;
    device->pwr_multiplier_mW = 0.625;
    device->conv_alert = false;
    device->limit_alert = false;
    device->overflow = false;
    device->i2c_error_code = 0;
    
    // 第一步：测试I2C连接
    uint16_t test_val = ina226_read_register(device, INA226_CONF_REG);
    if (test_val == 0) {
        return false;
    }
    
    // 第二步：复位设备
    ina226_reset(device);
    delay_ms(10);
    
    // 第三步：设置与Arduino库完全一致的配置
    ina226_write_register(device, INA226_CAL_REG, device->cal_value);
    ina226_set_average(device, INA226_AVERAGE_1);  // 与Arduino库保持一致：不使用平均
    // 使用与Arduino库相同的转换时间
    ina226_set_conversion_time(device, INA226_CONV_TIME_1100, INA226_CONV_TIME_1100);
    
    // 设置为连续模式（与Arduino库一致）
    ina226_set_measure_mode(device, INA226_CONTINUOUS);
    delay_ms(20);  // 稳定时间
    
    // 第四步：验证制造商ID
    delay_ms(50);
    uint16_t man_id = ina226_read_register(device, INA226_MAN_ID_REG);
    
    if (man_id != 0x5449) {
        return false;
    }
    
    return true;
}

/**
 * @brief 复位INA226器件 - 使用SysTick非阻塞时序版本
 */
void ina226_reset(INA226_Device *device) {
    extern volatile uint32_t system_time_ms;  // 使用全局SysTick时间
    ina226_write_register(device, INA226_CONF_REG, INA226_RST);
    
    // 非阻塞延时10ms等待复位完成
    uint32_t start_time = system_time_ms;
    while ((system_time_ms - start_time) < 10) {
        // 非阻塞等待
    }
}

/**
 * @brief 设置校正因子
 */
void ina226_set_correction_factor(INA226_Device *device, float corr) {
    device->corr_factor = corr;
    uint16_t cal_val_corrected = (uint16_t)(device->cal_value * device->corr_factor);
    ina226_write_register(device, INA226_CAL_REG, cal_val_corrected);
}

/**
 * @brief 设置平均采样次数
 */
void ina226_set_average(INA226_Device *device, INA226_AVERAGES averages) {
    device->averages = averages;
    uint16_t current_conf_reg = ina226_read_register(device, INA226_CONF_REG);
    current_conf_reg &= ~(0x0E00);  // 清除平均位
    current_conf_reg |= averages;
    ina226_write_register(device, INA226_CONF_REG, current_conf_reg);
}

/**
 * @brief 设置转换时间
 */
void ina226_set_conversion_time(INA226_Device *device, INA226_CONV_TIME shunt_conv_time, INA226_CONV_TIME bus_conv_time) {
    uint16_t current_conf_reg = ina226_read_register(device, INA226_CONF_REG);
    current_conf_reg &= ~(0x01C0);  // 清除总线转换时间位 [8:6]
    current_conf_reg &= ~(0x0038);  // 清除分流转换时间位 [5:3]
    
    // 按照Arduino库的实现方式
    uint16_t conv_mask = ((uint16_t)shunt_conv_time) << 3;
    current_conf_reg |= conv_mask;
    conv_mask = bus_conv_time << 6;  // 移除多余的强制转换
    current_conf_reg |= conv_mask;
    
    ina226_write_register(device, INA226_CONF_REG, current_conf_reg);
}

/**
 * @brief 设置测量模式
 */
void ina226_set_measure_mode(INA226_Device *device, INA226_MEASURE_MODE mode) {
    device->measure_mode = mode;
    uint16_t current_conf_reg = ina226_read_register(device, INA226_CONF_REG);
    current_conf_reg &= ~(0x0007);  // 清除模式位
    current_conf_reg |= mode;
    ina226_write_register(device, INA226_CONF_REG, current_conf_reg);
}

/**
 * @brief 设置电阻和电流范围
 */
void ina226_set_resistor_range(INA226_Device *device, float resistor, float current_range) {
    float current_LSB = current_range / 32768.0;
    
    device->cal_value = (uint16_t)(0.00512 / (current_LSB * resistor));
    device->current_divider_mA = 0.001 / current_LSB;
    device->pwr_multiplier_mW = 1000.0 * 25.0 * current_LSB;
    
    ina226_write_register(device, INA226_CAL_REG, device->cal_value);
}

/**
 * @brief 获取分流电压(伏特)
 */
float ina226_get_shunt_voltage_V(INA226_Device *device) {
    // 连续模式下直接读取
    int16_t val = (int16_t)ina226_read_register(device, INA226_SHUNT_REG);
    return (val * 0.0000025 * device->corr_factor);
}

/**
 * @brief 获取分流电压(毫伏)
 */
float ina226_get_shunt_voltage_mV(INA226_Device *device) {
    // 连续模式下直接读取
    int16_t val = (int16_t)ina226_read_register(device, INA226_SHUNT_REG);
    return (val * 0.0025 * device->corr_factor);
}

/**
 * @brief 获取总线电压(伏特) - 连续模式，直接读取（Arduino库方式）
 */
float ina226_get_bus_voltage_V(INA226_Device *device) {
    // 连续模式下直接读取，如Arduino库
    uint16_t val = ina226_read_register(device, INA226_BUS_REG);
    
    return (val * 0.00125);
}

/**
 * @brief 获取电流(毫安)
 */
float ina226_get_current_mA(INA226_Device *device) {
    // 连续模式下直接读取
    int16_t val = (int16_t)ina226_read_register(device, INA226_CURRENT_REG);
    return (val / device->current_divider_mA);
}

/**
 * @brief 获取电流(安培)
 */
float ina226_get_current_A(INA226_Device *device) {
    return (ina226_get_current_mA(device) / 1000.0);
}

/**
 * @brief 获取功率
 */
float ina226_get_bus_power(INA226_Device *device) {
    // 连续模式下直接读取
    uint16_t val = ina226_read_register(device, INA226_PWR_REG);
    return (val * device->pwr_multiplier_mW);
}

/**
 * @brief 一次性读取所有数据 - 连续模式版本
 */
void ina226_read_all_data(INA226_Device *device, float *bus_voltage, float *shunt_voltage, float *current, float *power) {
    // 连续模式下直接读取所有寄存器
    uint16_t shunt_val = ina226_read_register(device, INA226_SHUNT_REG);
    uint16_t bus_val = ina226_read_register(device, INA226_BUS_REG);
    uint16_t current_val = ina226_read_register(device, INA226_CURRENT_REG);
    uint16_t power_val = ina226_read_register(device, INA226_PWR_REG);
    
    // 转换为实际值
    if (bus_voltage) *bus_voltage = bus_val * 0.00125;
    if (shunt_voltage) *shunt_voltage = (int16_t)shunt_val * 0.0025 * device->corr_factor;
    if (current) *current = (int16_t)current_val / device->current_divider_mA;
    if (power) *power = power_val * device->pwr_multiplier_mW;
}

/**
 * @brief 开始单次测量
 */
void ina226_start_single_measurement(INA226_Device *device) {
    uint16_t val = ina226_read_register(device, INA226_MASK_EN_REG); // 清除CNVR标志
    val = ina226_read_register(device, INA226_CONF_REG);
    ina226_write_register(device, INA226_CONF_REG, val);        // 开始转换
    
    uint16_t conv_ready = 0x0000;
    uint32_t conv_start = 0; // 这里需要一个系统时间函数
    while (!conv_ready && (conv_start < 2000)) { // 2秒超时
        conv_ready = (ina226_read_register(device, INA226_MASK_EN_REG)) & 0x0008;
        delay_ms(1);
        conv_start++;
    }
}

/**
 * @brief 开始单次测量(不等待完成)
 */
void ina226_start_single_measurement_no_wait(INA226_Device *device) {
    uint16_t val = ina226_read_register(device, INA226_MASK_EN_REG); // 清除CNVR标志
    val = ina226_read_register(device, INA226_CONF_REG);
    ina226_write_register(device, INA226_CONF_REG, val);        // 开始转换
}

/**
 * @brief 检查是否忙碌
 */
bool ina226_is_busy(INA226_Device *device) {
    return (!(ina226_read_register(device, INA226_MASK_EN_REG) & 0x0008));
}

/**
 * @brief 进入省电模式
 */
void ina226_power_down(INA226_Device *device) {
    device->conf_reg_copy = ina226_read_register(device, INA226_CONF_REG);
    ina226_set_measure_mode(device, INA226_POWER_DOWN);
}

/**
 * @brief 退出省电模式
 */
void ina226_power_up(INA226_Device *device) {
    ina226_write_register(device, INA226_CONF_REG, device->conf_reg_copy);
    delay_ms(1);  
}

/**
 * @brief 等待转换完成 - 优化版本
 */
void ina226_wait_until_conversion_completed(INA226_Device *device) {
    // 检查转换就绪标志 (CVRF - bit 3)
    uint16_t conv_ready = 0x0000;
    uint32_t timeout = 0;
    
    while (!conv_ready && timeout < 20) { // 减少到20ms超时，提高响应速度
        uint16_t mask_reg = ina226_read_register(device, INA226_MASK_EN_REG);
        conv_ready = mask_reg & 0x0008; // 检查CVRF位
        if (!conv_ready) {
            delay_ms(1);
            timeout++;
        }
    }
    
    // 转换完成后再等待一小段时间确保数据稳定
    if (conv_ready) {
        delay_ms(2);
    }
}

/**
 * @brief 设置警报引脚为高电平有效
 */
void ina226_set_alert_pin_active_high(INA226_Device *device) {
    uint16_t val = ina226_read_register(device, INA226_MASK_EN_REG);
    val |= 0x0002;
    ina226_write_register(device, INA226_MASK_EN_REG, val);
}

/**
 * @brief 使能警报锁存
 */
void ina226_enable_alert_latch(INA226_Device *device) {
    uint16_t val = ina226_read_register(device, INA226_MASK_EN_REG);
    val |= 0x0001;
    ina226_write_register(device, INA226_MASK_EN_REG, val);
}

/**
 * @brief 使能转换就绪警报
 */
void ina226_enable_conv_ready_alert(INA226_Device *device) {
    uint16_t val = ina226_read_register(device, INA226_MASK_EN_REG);
    val |= 0x0400;
    ina226_write_register(device, INA226_MASK_EN_REG, val);
}

/**
 * @brief 设置警报类型和限制
 */
void ina226_set_alert_type(INA226_Device *device, INA226_ALERT_TYPE type, float limit) {
    device->alert_type = type;
    uint16_t alert_limit = 0;
    
    switch (device->alert_type) {
        case INA226_SHUNT_OVER:
            alert_limit = (uint16_t)(limit * 400);
            break;
        case INA226_SHUNT_UNDER:
            alert_limit = (uint16_t)(limit * 400);
            break;
        case INA226_CURRENT_OVER:
            device->alert_type = INA226_SHUNT_OVER;
            alert_limit = (uint16_t)(limit * 2048 * device->current_divider_mA / device->cal_value);
            break;
        case INA226_CURRENT_UNDER:
            device->alert_type = INA226_SHUNT_UNDER;
            alert_limit = (uint16_t)(limit * 2048 * device->current_divider_mA / device->cal_value);
            break;
        case INA226_BUS_OVER:
            alert_limit = (uint16_t)(limit * 800);
            break;
        case INA226_BUS_UNDER:
            alert_limit = (uint16_t)(limit * 800);
            break;
        case INA226_POWER_OVER:
            alert_limit = (uint16_t)(limit / device->pwr_multiplier_mW);
            break;
    }
    
    ina226_write_register(device, INA226_ALERT_LIMIT_REG, alert_limit);
    
    uint16_t value = ina226_read_register(device, INA226_MASK_EN_REG);
    value &= ~(0xF800);
    value |= device->alert_type;
    ina226_write_register(device, INA226_MASK_EN_REG, value);
}

/**
 * @brief 读取并清除标志
 */
void ina226_read_and_clear_flags(INA226_Device *device) {
    uint16_t value = ina226_read_register(device, INA226_MASK_EN_REG);
    device->overflow = (value >> 2) & 0x0001;
    device->conv_alert = (value >> 3) & 0x0001;
    device->limit_alert = (value >> 4) & 0x0001;
}

/**
 * @brief 获取I2C错误代码
 */
uint8_t ina226_get_i2c_error_code(INA226_Device *device) {
    return device->i2c_error_code;
}
