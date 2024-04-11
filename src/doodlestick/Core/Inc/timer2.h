/*
 * timer2.h
 *
 *  Created on: Oct 18, 2023
 *      Author: jackkrammer
 *
 *  a header file to make the initialization of TIM2 easier
 */

#ifndef SRC_TIMER2_H_
#define SRC_TIMER2_H_


void TIM2_init(uint32_t arr, uint32_t psc, uint32_t ccr1); // initializes TIM2 with the provided ARR PSC CCR1 values

// initializes TIM2 with the provided ARR PSC CCR1 values
void TIM2_init(uint32_t arr, uint32_t psc, uint32_t ccr1)
{
	// enable the clock for TIM2
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN; // TIM2 clock

	// configure timer count settings
	TIM2->CR1 &= ~TIM_CR1_CMS; // sets count to be one directional
	TIM2->CR1 &= ~TIM_CR1_DIR; // sets timer to count up
	TIM2->PSC = psc; // divides timer clock by PSC + 1
	TIM2->ARR = arr;//804 // timer counts to ARR + 1 // 399 for 50% (w/o CCR1) // 799 when w CCR1
	//TIM2->CCR1 = ccr1;//594 // compares count to CCR1 // 599 for 25% duty b/c 599 ~= 0.75*799

	// enable interrupts
	__enable_irq(); // enables ARM interrupts
	NVIC->ISER[0] = (1 << (TIM2_IRQn & 0x1F)); // enables NVIC TIM2 interrupt
	TIM2->SR &= ~TIM_SR_UIF; // resets TIM2 update interrupt flag
	//TIM2->SR &= ~TIM_SR_CC1IF; // resets TIM2 CC1 interrupt flag
	TIM2->DIER |= TIM_DIER_UIE; // enable TIM2 update interrupt
	//TIM2->DIER |= TIM_DIER_CC1IE; // enable TIM2 CC1 interrupt

	// start timer
	TIM2->CR1 |= TIM_CR1_CEN; // enables the counter
}



#endif /* SRC_TIMER2_H_ */
