#ifndef __ENCODER_H
#define __ENCODER_H

void Encoder_Init(uint32_t RCC_APB2Periph, 
					GPIO_TypeDef* GPIOx, 
					uint8_t GPIO_PortSource, 
					uint8_t GPIO_PinSource_1, 
					uint8_t GPIO_PinSource_2, 
					uint16_t GPIO_Pin,
                    uint32_t EXTI_Line,
					EXTITrigger_TypeDef EXTI_Trigger,
					IRQn_Type NVIC_IRQChannel1,
					IRQn_Type NVIC_IRQChannel2) ;

int16_t Encoder_Get(void);




#endif

