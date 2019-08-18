œ»ƒŒ–¿—»Õ¿
#include <stm32f10x.h>
#include <stdio.h>
#include <math.h>

#include "st7735.h"
#include "misc.h"

uint16_t Pot_V,Therm_V,Power_V;
ST7735_ProgressBar Pot_V_pb,Therm_V_pb,Power_pb;
ST7735_Button btn,btn1;

int16_t reg_e=0;

long range(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void ADC1_2_IRQHandler(void)
{
  if(ADC1->SR & ADC_SR_JEOC)
	{
    Pot_V = ADC1->JDR1;
		Therm_V = ADC1->JDR2;	
	}
	ADC1->SR&=~ADC_SR_JEOC;
}

void TIM4_IRQHandler(void)
{
	float power;
	char str[5];
	char str1[5];
	
	TIM4->SR &= ~TIM_SR_UIF;
	
	reg_e = Pot_V - Therm_V;
		
	if (reg_e > 0)
	{
		power = log(reg_e)*10;
		
		if(power>100.0f)
			power = 100.0f;
		
		Power_V = (uint16_t) power;
		TIM2->CCR2 = Power_V;
	}
	else
	{
		TIM2->CCR2 = 0;
		Power_V = 0;
	}
	sprintf(str,"%d",Pot_V);
	sprintf(str1,"%d",Therm_V);
	
	btn.txt =(uint8_t *) str;
	ST7735_Button_Draw(&btn);
	
	btn1.txt = (uint8_t *) str1;
	ST7735_Button_Draw(&btn1);
	
	ST7735_ProgressBar_SetProgress(&Pot_V_pb,range(Pot_V,0,4095,0,100));
	ST7735_ProgressBar_SetProgress(&Therm_V_pb,range(Therm_V,0,4095,0,100));
	ST7735_ProgressBar_SetProgress(&Power_pb,Power_V);		
}

void TIM1_IRQHandler(void)
{
	
}

void InitADC(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	RCC->CFGR |= RCC_CFGR_ADCPRE_DIV8;
	
	GPIOB->CRL &= ~(GPIO_CRL_MODE0_0|GPIO_CRL_MODE0_1|GPIO_CRL_CNF0_0|GPIO_CRL_CNF0_1);
	GPIOA->CRL &= ~(GPIO_CRL_MODE2_0|GPIO_CRL_MODE2_1|GPIO_CRL_CNF2_0|GPIO_CRL_CNF2_1);
	
	ADC1->CR2 |= (ADC_CR2_ADON);
	
	ADC1->CR2 |= ADC_CR2_RSTCAL;
	while ((ADC1->CR2 & ADC_CR2_RSTCAL) == ADC_CR2_RSTCAL)
	{
	}

	ADC1->CR2 |= ADC_CR2_CAL;
	while ((ADC1->CR2 & ADC_CR2_RSTCAL) == ADC_CR2_CAL)
	{
	}
	
	ADC1->CR1 |=(ADC_CR1_JEOCIE|ADC_CR1_JAUTO|ADC_CR1_SCAN);
	ADC1->CR2 |= (ADC_CR2_JEXTSEL | ADC_CR2_JEXTTRIG | ADC_CR2_CONT);
	ADC1->SMPR2 = ADC_SMPR2_SMP8|ADC_SMPR2_SMP2; 
	ADC1->JSQR|=(ADC_JSQR_JL_0|ADC_JSQR_JSQ3_3|ADC_JSQR_JSQ4_1);
	ADC1->JSQR&=~(ADC_JSQR_JL_1);
	
	NVIC_EnableIRQ(ADC1_2_IRQn);
	
	
	ADC1->CR2 |= ADC_CR2_JSWSTART;
}

void InitTimers(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN|RCC_APB1ENR_TIM4EN;
	RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN);
	
	GPIOA->CRL |=(GPIO_CRL_MODE1_0|GPIO_CRL_MODE1_1|GPIO_CRL_CNF1_1);
	GPIOA->CRL &=~(GPIO_CRL_CNF1_0);
	
	
	TIM2->PSC = 7200;
	TIM2->ARR = 100;
	TIM2->CCER |= (TIM_CCER_CC2E);
	TIM2->CCMR1|=(TIM_CCMR1_OC2M_0| TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2);
	TIM2->CR1 |= TIM_CR1_CEN;
	
	TIM2->CCR2 = 50;
	
	TIM4->PSC = 7200;
	TIM4->ARR = 10000;
	TIM4->DIER |= TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM4_IRQn);
	
	TIM4->CR1 |= TIM_CR1_CEN;
}


void InitEncoderTimer(void)
{
	//PA8(TIM1 CH1)
	GPIOA->CRH |= GPIO_CRH_CNF8_1;
	GPIOA->CRH &= ~(GPIO_CRH_CNF8_0|GPIO_CRH_MODE8);
	GPIOA->ODR |= GPIO_ODR_ODR8;
	//PA9(TIM1 CH1)
	GPIOA->CRH |= GPIO_CRH_CNF9_1;
	GPIOA->CRH &= ~(GPIO_CRH_CNF9_0|GPIO_CRH_MODE9);
	GPIOA->ODR |= GPIO_ODR_ODR9;
	
	RCC->APB2ENR |= (RCC_APB2ENR_TIM1EN);
	
//	TIM1->CCMR1 |= TIM_CCMR1_IC1F | TIM_CCMR1_IC2F;
	TIM1->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);
	TIM1->CCMR1 &= ~(TIM_CCMR1_CC2S_0 |TIM_CCMR1_CC1S_0);
	TIM1->SMCR |= TIM_SMCR_SMS_0;
	TIM1->ARR = 800;
	TIM1->CR1 |= TIM_CR1_CEN;
}

int main(void)
{
	ST7735_Init();
	ST7735_Clear(COLOR565_BLACK);
	
	ST7735_ProgressBar_StructInit(&Pot_V_pb,10,50);
	ST7735_ProgressBar_StructInit(&Therm_V_pb,10,75);
	ST7735_ProgressBar_StructInit(&Power_pb,10,100);
	
	Pot_V_pb.w = 100;
	Pot_V_pb.ProgressColor = COLOR565_BLUE;
	
	Therm_V_pb.w = 100;
	Therm_V_pb.ProgressColor = COLOR565_RED;
	
	Power_pb.w = 100;
	Power_pb.ProgressColor = COLOR565_YELLOW;
	
	ST7735_Button_StructInit(&btn,10,10,"",4);
	ST7735_Button_StructInit(&btn1,90,10,"",4);
	
	InitADC();
	
	InitTimers();
	
	InitEncoderTimer();
	
	while(1)
	{
		
	}
}
