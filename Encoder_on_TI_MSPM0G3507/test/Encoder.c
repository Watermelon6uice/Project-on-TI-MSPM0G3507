#include "stm32f10x.h"                  // Device header
#include "Delay.h"

int16_t Encoder_Count;

void Encoder_Init(uint32_t RCC_APB2Periph, 
					GPIO_TypeDef* GPIOx, 
					uint8_t GPIO_PortSource, 
					uint8_t GPIO_PinSource_1, 
					uint8_t GPIO_PinSource_2, 
					uint16_t GPIO_Pin,
                    uint32_t EXTI_Line,
					EXTITrigger_TypeDef EXTI_Trigger,
					IRQn_Type NVIC_IRQChannel1,
					IRQn_Type NVIC_IRQChannel2) {
						
	

	RCC_APB2PeriphClockCmd(RCC_APB2Periph, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx, &GPIO_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSource,  GPIO_PinSource_1);
	GPIO_EXTILineConfig(GPIO_PortSource,  GPIO_PinSource_2);
	
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = NVIC_IRQChannel1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = NVIC_IRQChannel2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&NVIC_InitStructure);
	
}

int16_t Encoder_Get(void){
	int16_t Temp;
	Temp = Encoder_Count;
	Encoder_Count = 0;
	
	return Temp;
}

void EXTI0_IRQHandler(void){
	uint32_t EXTI_Line = EXTI_Line0;
	uint16_t GPIO_PinA = GPIO_Pin_0;
	uint16_t GPIO_PinB = GPIO_Pin_1;
	GPIO_TypeDef* GPIOx = GPIOB;
	
	if(EXTI_GetITStatus(EXTI_Line) == SET){
		
		if(GPIO_ReadInputDataBit(GPIOx, GPIO_PinB) == 0){
			Encoder_Count --;
		}
		
		EXTI_ClearITPendingBit(EXTI_Line);
	}
}

void EXTI1_IRQHandler(void){
	uint32_t EXTI_Line = EXTI_Line1;
	uint16_t GPIO_PinA = GPIO_Pin_1;
	uint16_t GPIO_PinB = GPIO_Pin_0;
	GPIO_TypeDef* GPIOx = GPIOB;
	
	if(EXTI_GetITStatus(EXTI_Line) == SET){
		
		if(GPIO_ReadInputDataBit(GPIOx, GPIO_PinB) == 0){
			Encoder_Count ++;
		}
		
		EXTI_ClearITPendingBit(EXTI_Line);
	}
}
