#include "led.h"


#define GPIOCEN			(1U<<2)// 0b 0000 0000 0000	0000 0000 0000 0000 0100
#define LED_PIN			(1U<<13)

void led_init(void)
{
	/*Enable Clock Access to LED Port (Port C)*/
	RCC->AHB1ENR |= GPIOCEN;
	GPIOC->MODER |= (1U<<26);
	GPIOC->MODER &= ~(1U<<27);


	/*Set Led Pin as output pin*/

}

void led_on(void)
{
	/*Set led pin HIGH (PC13)*/
	GPIOC->ODR |= LED_PIN;

}

void led_off(void)
{
	/*Set led pin LOW (PC13)*/
	GPIOC->ODR &= ~LED_PIN;
}
