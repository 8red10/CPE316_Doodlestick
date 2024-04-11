/*
 * uart.h
 *
 *  Created on: Dec 5, 2023
 *      Author: jackkrammer
 *      as of 4:54pm 20231207
 */

#ifndef UART_H_
#define UART_H_


//#define F_CLK 4000000 	// bus clock is 4 MHz
#define F_CLK 32000000 // clock for ADC is 32MHz

/* Private function prototypes -----------------------------------------------*/
void USART_Print(const char* message);
void USART_Escape_Code(const char* msg);
void USART_init();


void USART_init()
{
	// configure GPIO pins for USART2 (PA2, PA3) follow order of configuring registers
	// AFR, OTYPER, PUPDR, OSPEEDR, MODDER otherwise a glitch is created on the output pin
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN);

	GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);		// mask AF selection
	GPIOA->AFR[0] |= ((7 << GPIO_AFRL_AFSEL2_Pos ) |			// select USART2 (AF7)
				   (7 << GPIO_AFRL_AFSEL3_Pos));

	GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);	// enable alternate function
	GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);    // for PA2 and PA3

	// Configure USART2 connected to the debugger virtual COM port
	RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;	// enable USART by turning on system clock

	USART2->CR1 &= ~(USART_CR1_M1 | USART_CR1_M0);	// set data to 8 bits
	USART2->BRR = F_CLK / 115200;
	USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);		// enable transmit and receive for USART

	// enable interrupts for USART2 receive
	USART2->CR1 |= USART_CR1_RXNEIE;			// enable RXNE interrupt on USART2

	USART2->ISR &= ~(USART_ISR_RXNE);			// clear interrupt flag while (message[i] != 0)

	NVIC->ISER[1] = (1 << (USART2_IRQn & 0x1F));		// enable USART2 ISR

	__enable_irq();

	USART2->CR1 |= USART_CR1_UE;			// enable USART


	// clear screen
	USART_Escape_Code("[2J");
	// top left cursor
	USART_Escape_Code("[H");
}

// use a for loop to output one byte at a time to TDR
void USART_Print(const char* message)
{
	uint8_t i;
	for(i=0; message[i] != 0; i++){				// check for terminating NULL character
		while(!(USART2->ISR & USART_ISR_TXE));	// wait for transmit buffer to be empty
		USART2->TDR = message[i];				// transmit character to USART
	}
}

// add /ESC to TDR before the actual escape code
void USART_Escape_Code(const char* msg)
{
	while(!(USART2->ISR & USART_ISR_TXE));	// wait for transmit buffer to be empty
	USART2->TDR = 0x1B;
	USART_Print(msg);
}

// uses the corresponding color escape code or just prints out character
// enter adds \n after the \r it defaults to
void USART2_IRQHandler(void)
{
	if (USART2->ISR & USART_ISR_RXNE)
	{
//		// writes keyboard input to the serial display
//		while(!(USART2->ISR & USART_ISR_TXE));
//			USART2->TDR = USART2->RDR;

		// clears the flag
		USART2->ISR &= ~(USART_ISR_RXNE);
	}
}


#endif /* UART_H_ */
