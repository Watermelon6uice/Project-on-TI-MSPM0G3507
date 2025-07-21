#include "user_OLED.h"
#include "user_OLED_Font.h"
#include "user/delay.h"
#include <stdbool.h>

// OLED设备地址 (7位地址，硬件会自动左移)
#define OLED_ADDRESS 0x3C  // 改回0x3C

// I2C写入函数封装 - 增强稳定性版本
static void OLED_I2C_Write(uint8_t reg, uint8_t data)
{
    uint32_t timeout = 10000;  // 增加超时时间
    
    // 等待I2C空闲，带超时
    while ((DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) && timeout--);
    
    if (timeout == 0) return;  // 超时退出
    
    // 开始I2C传输
    DL_I2C_startControllerTransfer(I2C_0_INST, OLED_ADDRESS, DL_I2C_CONTROLLER_DIRECTION_TX, 2);
    
    // 发送控制字节
    DL_I2C_transmitControllerData(I2C_0_INST, reg);
    
    // 增加延时确保数据传输稳定
    for(volatile int i = 0; i < 500; i++);
    
    // 发送数据
    DL_I2C_transmitControllerData(I2C_0_INST, data);
    
    // 增加延时等待传输完成
    for(volatile int i = 0; i < 2000; i++);
}

// 带状态返回的I2C写入函数 - 用于调试
static bool OLED_I2C_Write_Debug(uint8_t reg, uint8_t data)
{
    uint32_t timeout = 10000;
    
    // 检查I2C状态
    uint32_t status = DL_I2C_getControllerStatus(I2C_0_INST);
    if (status & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
        // 等待空闲
        while ((DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) && timeout--);
        if (timeout == 0) return false;  // 超时
    }
    
    // 开始I2C传输
    DL_I2C_startControllerTransfer(I2C_0_INST, OLED_ADDRESS, DL_I2C_CONTROLLER_DIRECTION_TX, 2);
    
    // 发送控制字节
    DL_I2C_transmitControllerData(I2C_0_INST, reg);
    
    // 减小延时以提升DAC响应速度
    for(volatile int i = 0; i < 50; i++);  // 从500减少到50
    
    // 发送数据
    DL_I2C_transmitControllerData(I2C_0_INST, data);
    
    // 等待传输完成 - 减小延时
    for(volatile int i = 0; i < 200; i++);  // 从2000减少到200
    
    return true;  // 成功
}

/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Write(0x00, Command);  // 0x00表示写命令
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Write(0x40, Data);     // 0x40表示写数据
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);					//设置Y位置
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
    OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for(i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      	
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
    }
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)							
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++)							
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)							
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        }
        else
        {
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
        }
    }
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)							
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/**
  * @brief  OLED显示汉字
  * @param  Line 行位置
  * @param  Column 列位置
  * @param  Chinese 显示汉字编号
  * @retval 无
  */
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t Chinese)
{
    uint8_t i, j;
    
    for(i=0; i<2; i++) {
        OLED_SetCursor(Line+i, Column*16);	//循环2次显示上/下半部分内容	
        for (j=0; j<16; j++) {//字宽为16
            OLED_WriteData(Chinese_16x16[Chinese][j+i*16]);			
        }
    }
}

/**
  * @brief  OLED初始化 - 快速版本
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{
    // 上电延时 - 这个必须保留
    delay_ms(50);  // 减少到50ms
    
    // OLED初始化序列 - 移除中间延时
    OLED_WriteCommand(0xAE);	//关闭显示
    
    OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
    OLED_WriteCommand(0x80);
    
    OLED_WriteCommand(0xA8);	//设置多路复用率
    OLED_WriteCommand(0x3F);
    
    OLED_WriteCommand(0xD3);	//设置显示偏移
    OLED_WriteCommand(0x00);
    
    OLED_WriteCommand(0x40);	//设置显示开始行
    OLED_WriteCommand(0xA1);	//设置左右方向
    OLED_WriteCommand(0xC8);	//设置上下方向

    OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
    OLED_WriteCommand(0x12);
    
    OLED_WriteCommand(0x81);	//设置对比度控制
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);	//设置预充电周期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭
    OLED_WriteCommand(0xA6);	//设置正常/倒转显示

    OLED_WriteCommand(0x8D);	//设置充电泵
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF);	//开启显示
        
    OLED_Clear();				//OLED清屏
    delay_ms(5);  // 最小延时确保初始化完成
}

/**
  * @brief  OLED显示浮点数
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的浮点数
  * @param  IntLen 整数部分长度
  * @param  DecLen 小数部分长度
  * @retval 无
  */
void OLED_ShowFloat(uint8_t Line, uint8_t Column, float Number, uint8_t IntLen, uint8_t DecLen)
{
    uint8_t i;
    uint32_t IntPart, DecPart;
    
    // 处理负数
    if (Number < 0)
    {
        OLED_ShowChar(Line, Column, '-');
        Number = -Number;
        Column++;
    }
    else
    {
        OLED_ShowChar(Line, Column, '+');
        Column++;
    }
    
    // 分离整数和小数部分
    IntPart = (uint32_t)Number;
    DecPart = (uint32_t)((Number - IntPart) * OLED_Pow(10, DecLen));
    
    // 显示整数部分
    for (i = 0; i < IntLen; i++)
    {
        OLED_ShowChar(Line, Column + i, IntPart / OLED_Pow(10, IntLen - i - 1) % 10 + '0');
    }
    
    // 显示小数点
    OLED_ShowChar(Line, Column + IntLen, '.');
    
    // 显示小数部分
    for (i = 0; i < DecLen; i++)
    {
        OLED_ShowChar(Line, Column + IntLen + 1 + i, DecPart / OLED_Pow(10, DecLen - i - 1) % 10 + '0');
    }
}

/**
  * @brief  测试OLED通信
  * @param  无
  * @retval 通信是否成功
  */
bool OLED_TestCommunication(void)
{
    // 尝试发送几个测试命令
    bool result1 = OLED_I2C_Write_Debug(0x00, 0xAE);  // 关闭显示
    bool result2 = OLED_I2C_Write_Debug(0x00, 0xAF);  // 开启显示
    bool result3 = OLED_I2C_Write_Debug(0x40, 0x00);  // 发送数据测试
    
    return result1 && result2 && result3;
}
