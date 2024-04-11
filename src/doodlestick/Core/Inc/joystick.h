/*
 * joystick.h
 *
 *  Created on: Dec 7, 2023
 *      Author: jackkrammer
 *
 *
 *  HEADER FILE FOR THE ARDUINO JOYSTICK MODULE
 *
 *  	ADC PER COORDINATE (X AND Y)
 *  		the joystick has both a x Vref pin (PA0) and a y Vref pin (PA1)
 *  		this header file uses 2 ADC's (pin PA0 for ADC1 and pin PA1 for ADC2)
 *
 *  	CALIBRATION STEPS
 *			1) 	need to calibrate with a reliable DC voltage source
 *			2) 	graph the voltage input to the ADC vs the data received
 *			   		to get the calibration equation directly from the trendline equation
 *			3) 	graph should have y-axis = Vin, x-axis = data
 *			   		so that can input data to equation and get the voltage
 *
 *		USART COMPATIBILITY
 *			1)	if using the USART in other sections of the code
 *					need to replace the FLCK value with the one here
 *					this ensures a faster conversion
 *
 *		CLOCK FREQUENCY
 *			1)	sets the clock to 32MHz for quicker ADC conversions
 *					check that doesn't interfere with USART
 *					comment out the FCLK declaration in USART header
 *
 *		INTERRUPTS
 *			1)	uses interrupts to trigger each new ADC conversion
 *
 *
 *		RANGE OF VALUES
 *			the input voltage to the joystick should be 3.3V to work properly
 *
 *
 *		IMPLEMENTATIONS
 *			1)	the user of this file must include the IRQ for the ADC's
 *					the IRQ must check for the ADC1 (xcoord) and ADC2 (ycoord) individually
 *					example code commented out in this file-------------------------------------------------------------
 *			2)	the ADC for the X coordinate must be initialized before the ADC for the Y coordinate
 *
 *					maybe doesn't matter, need to check with ADC123 clock initialization
 *
 *
 */

#ifndef SRC_JOYSTICK_H_
#define SRC_JOYSTICK_H_


// function declarations
void SystemClock_Config(void); // configures the system clock to 32MHz
void Error_Handler(void); // error handler for system clock configuration
void init_ADCs(); // initializes the system clock to 32MHz and ADC for x coordinate and ADC for y coordinate
void xcoord_adc_init(); // initializes ADC1 for reading the X coordinate on PA0
void ycoord_adc_init(); // initializes ADC2 for reading the Y coordinate on PA1
void xcoord_start_conversion(); // starts the ADC conversion for the x coordinate
void ycoord_start_conversion(); // starts the ADC conversion for the y coordinate
void joystick_button_init(); // initializes the GPIO pin PA4 for the input from the button (as interrupt ? )
uint8_t get_joystick_button(); // gets the value of the button where 1 is being pressed and 0 is not pressed


// THIS FUNCTION DECLARATION INDICATES THAT NEED TO IMPLEMENT IN THE MAIN.C FILE
// clear flag and set globals
void ADC1_2_IRQHandler();/*
{
	// Vref for X is the input to ADC1
	if (ADC1->ISR & ADC_ISR_EOC)
	{
		xcoord_data = ADC1->DR;
		xcoord_adc_flag = 1;
	}

	// Vref for Y is the input to ADC2
	if(ADC2->ISR & ADC_ISR_EOC)
	{
		ycoord_data = ADC2->DR;
		ycoord_adc_flag = 1;
	}
}*/

// gets the value of the button where 1 is being pressed and 0 is not pressed
uint8_t get_joystick_button()
{
	return !(GPIOA->IDR & GPIO_IDR_ID4);
}

// initializes the GPIO pin PA4 as the input from the active low joystick button
void joystick_button_init()
{
	// starts the GPIOA peripheral clock
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN);

	// input mode
	GPIOA->MODER &= ~(GPIO_MODER_MODE4);

	// pull up resistor
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD4);
	GPIOA->PUPDR |=  (GPIO_PUPDR_PUPD4_0);
}

// starts the ADC conversion for the y coordinate
void ycoord_start_conversion()
{
	// Vref for y is the input to ADC2
	ADC2->CR |= ADC_CR_ADSTART;
}

// starts the ADC conversion for the x coordinate
void xcoord_start_conversion()
{
	// Vref for x is the input to ADC1
	ADC1->CR |= ADC_CR_ADSTART;
}

// initializes ADC2 for reading the Y coordinate on PA1
void ycoord_adc_init()
{
	// turns on the clock for ADC
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
	ADC123_COMMON->CCR = ((ADC123_COMMON->CCR & ~(ADC_CCR_CKMODE)) | ADC_CCR_CKMODE_0);

	// make sure conversion isn't started
	ADC2->CR &= ~(ADC_CR_ADSTART);

	// take the ADC out of deep power down mode
	ADC2->CR &= ~(ADC_CR_DEEPPWD);

	// enable to voltage regulator guards the voltage
	ADC2->CR |= (ADC_CR_ADVREGEN);

	// delay at least 20 microseconds (to power up) before calibration
	for(uint16_t i =0; i<1000; i++)
		for(uint16_t j = 0;  j<100; j++); //delay at least 20us

	// calibrate, you need to digitally calibrate
	ADC2->CR &= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF); //ensure ADC is not enabled, also choose single ended calibration
	ADC2->CR |= ADC_CR_ADCAL;       // start calibration
	while(ADC2->CR & ADC_CR_ADCAL); // wait for calibration

	// configure single ended mode before enabling ADC
	ADC2->DIFSEL &= ~(ADC_DIFSEL_DIFSEL_6); // PA1 is ADC1_IN6 (found via nucleo_board.pdf), single ended mode, DIFFERENT PER CHANNEL
	ADC2->SMPR1 |= 0x7 << 18; // configures sample period, DIFFERENT FOR EACH CHANNEL

	// enable ADC
	ADC2->ISR |= (ADC_ISR_ADRDY); // tells hardware that ADC is ready for conversion
	ADC2->CR |= ADC_CR_ADEN; // enables the ADC
	while(!(ADC2->ISR & ADC_ISR_ADRDY)); // waits until the ADC sets this flag low
	ADC2->ISR |= (ADC_ISR_ADRDY); // sets the flag high again

	// configure ADC
	ADC2->SQR1 = (ADC2->SQR1 & ~(ADC_SQR1_SQ1_Msk | ADC_SQR1_L_Msk)) |(6 << ADC_SQR1_SQ1_Pos); // DIFFERENT PER CHANNEL
	ADC2->ISR &= ~(ADC_ISR_EOC); // clears end of conversion flag

	// enables interrupts
	ADC2->IER |= (ADC_IER_EOC); // enables ADC interrupt
	NVIC->ISER[0] |= (1 << (ADC1_2_IRQn & 0x1F)); // enables NVIC interrupt
	__enable_irq(); // enables ARM interrupts

	//configure GPIO pin PA0
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN); 	// enables the GPIOA peripheral clock
	GPIOA->MODER |= (GPIO_MODER_MODE1); 	// analog mode for PA1
	GPIOA->ASCR |= GPIO_ASCR_ASC1; 			// set PA1 to analog
}

// initializes ADC1 for reading the X coordinate on PA0
void xcoord_adc_init()
{
//	// configures the system clock to be 32MHz for ADC
//	SystemClock_Config();

	// turns on the clock for ADC
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
	ADC123_COMMON->CCR = ((ADC123_COMMON->CCR & ~(ADC_CCR_CKMODE)) | ADC_CCR_CKMODE_0);

	// make sure conversion isn't started
	ADC1->CR &= ~(ADC_CR_ADSTART);

	// take the ADC out of deep power down mode
	ADC1->CR &= ~(ADC_CR_DEEPPWD);

	// enable to voltage regulator guards the voltage
	ADC1->CR |= (ADC_CR_ADVREGEN);

	// delay at least 20 microseconds (to power up) before calibration
	for(uint16_t i =0; i<1000; i++)
		for(uint16_t j = 0;  j<100; j++); //delay at least 20us

	// calibrate, you need to digitally calibrate
	ADC1->CR &= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF); //ensure ADC is not enabled, also choose single ended calibration
	ADC1->CR |= ADC_CR_ADCAL;       // start calibration
	while(ADC1->CR & ADC_CR_ADCAL); // wait for calibration

	// configure single ended mode before enabling ADC
	ADC1->DIFSEL &= ~(ADC_DIFSEL_DIFSEL_5); // PA0 is ADC1_IN5 (found via nucleo_board.pdf), single ended mode
	ADC1->SMPR1 |= 0x7 << 15; // configures sample period, DIFFERENT FOR EACH CHANNEL

	// enable ADC
	ADC1->ISR |= (ADC_ISR_ADRDY); // tells hardware that ADC is ready for conversion
	ADC1->CR |= ADC_CR_ADEN; // enables the ADC
	while(!(ADC1->ISR & ADC_ISR_ADRDY)); // waits until the ADC sets this flag low
	ADC1->ISR |= (ADC_ISR_ADRDY); // sets the flag high again

	// configure ADC
	ADC1->SQR1 = (ADC1->SQR1 & ~(ADC_SQR1_SQ1_Msk | ADC_SQR1_L_Msk)) |(5 << ADC_SQR1_SQ1_Pos); // DIFFERENT PER CHANNEL
	ADC1->ISR &= ~(ADC_ISR_EOC); // clears end of conversion flag

	// enables interrupts
	ADC1->IER |= (ADC_IER_EOC); // enables ADC interrupt
	NVIC->ISER[0] |= (1 << (ADC1_2_IRQn & 0x1F)); // enables NVIC interrupt
	__enable_irq(); // enables ARM interrupts

	//configure GPIO pin PA0
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN); 	// enables the GPIOA peripheral clock
	GPIOA->MODER |= (GPIO_MODER_MODE0); 	// analog mode for PA0
	GPIOA->ASCR |= GPIO_ASCR_ASC0; 			// set PA0 to analog

//	// start conversion
//	ADC1->CR |= ADC_CR_ADSTART;
}

// initializes the system clock to 32MHz and ADC for x coordinate and ADC for y coordinate
void init_ADCs()
{
	SystemClock_Config();
	xcoord_adc_init();
	ycoord_adc_init();
}

// error handler for system clock configuration
void Error_Handler(void)
{
 /* USER CODE BEGIN Error_Handler_Debug */
 /* User can add his own implementation to report the HAL error return state */
 __disable_irq();
 while (1)
 {
 }
 /* USER CODE END Error_Handler_Debug */
}


// configures the system clock to 32MHz
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	/** Configure the main internal regulator output voltage
	*/
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
	//RCC_OscInitStruct.MSIState = RCC_MSI_ON;  //datasheet says NOT to turn on the MSI then change the frequency.
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_10;
	/* from stm32l4xx_hal_rcc.h
	#define RCC_MSIRANGE_0                 MSI = 100 KHz
	#define RCC_MSIRANGE_1                 MSI = 200 KHz
	#define RCC_MSIRANGE_2                 MSI = 400 KHz
	#define RCC_MSIRANGE_3                 MSI = 800 KHz
	#define RCC_MSIRANGE_4                 MSI = 1 MHz
	#define RCC_MSIRANGE_5                 MSI = 2 MHz
	#define RCC_MSIRANGE_6                 MSI = 4 MHz
	#define RCC_MSIRANGE_7                 MSI = 8 MHz
	#define RCC_MSIRANGE_8                 MSI = 16 MHz
	#define RCC_MSIRANGE_9                 MSI = 24 MHz
	#define RCC_MSIRANGE_10                MSI = 32 MHz
	#define RCC_MSIRANGE_11                MSI = 48 MHz   dont use this one*/
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;  //datasheet says NOT to turn on the MSI then change the frequency.
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
									| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
		Error_Handler();
	}
}

#endif /* SRC_JOYSTICK_H_ */
