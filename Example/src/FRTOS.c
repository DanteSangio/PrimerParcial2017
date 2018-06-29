/*
===============================================================================
 Name        : FRTOS.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/
#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

TaskHandle_t		Handle_Tarea_Lectura;
TaskHandle_t		Handle_Tarea_Escritura;
TaskHandle_t		Handle_Tarea_Timer;

SemaphoreHandle_t 	SemaforoSincronizacion;

QueueHandle_t 		Queue_Pwm;

#include <cr_section_macros.h>

// TODO: insert other include files here

// TODO: insert other definitions and declarations here
#define PORT(x) 	((uint8_t) x)
#define PIN(x)		((uint8_t) x)

/*	Definicion de pines y puertos	*/

#define LDR_PORT		((uint8_t) 2)
#define SHUT_DOWN_PORT	((uint8_t) 1)
#define PULS_UP_PORT	((uint8_t) 2)
#define PULS_DOWN_PORT	((uint8_t) 2)
#define AUTO_MANU_PORT	((uint8_t) 0)
#define PWM_PORT		((uint8_t) 0)

#define LDR_PIN			((uint8_t) 10)
#define SHUT_DOWN_PIN	((uint8_t) 31)
#define PULS_UP_PIN		((uint8_t) 3)
#define PULS_DOWN_PIN	((uint8_t) 4)
#define AUTO_MANU_PIN	((uint8_t) 16)
#define PWM_PIN			((uint8_t) 22)

/*	Fin Definicion de pines y puertos	*/

#define OUTPUT		((uint8_t) 1)
#define INPUT		((uint8_t) 0)

#define MANUAL_UP	((uint8_t) 6)
#define MANUAL_DOWN	((uint8_t) 5)
#define MANUAL		((uint8_t) 4)
#define AUTO		((uint8_t) 3)
#define ON			((uint8_t) 2)
#define OFF			((uint8_t) 1)
#define IDLE		((uint8_t) 0)

#define PRESSED		((uint8_t) 0)
#define NOT_PRESSED	((uint8_t) 1)

#define TICKRATE1_HZ 10   // NO TERMINADO
#define TICKRATE2_HZ 7	  // NO TERMINADO

uint32_t timerFreq;

typedef struct
{
	uint8_t 	DUTY;
	uint8_t 	STATE;
}PWM_CFIG;

PWM_CFIG PWM_Struct;

void SetDutyCycle(uint8_t duty)
{
	//Configuro los match para lograr el duty que quiero - NO TERMINADO
	Chip_TIMER_SetMatch(LPC_TIMER0, 1, (timerFreq / TICKRATE1_HZ));
	Chip_TIMER_SetMatch(LPC_TIMER0, 2, (timerFreq / TICKRATE2_HZ));
}

uint8_t GetDutyCycle(void)
{
	//Devuelvo el duty cycle actual - NO TERMINADO
}

void Configuracion_Timer ( void )
{
	Chip_TIMER_Init(LPC_TIMER0);
	timerFreq = Chip_Clock_GetPeripheralClockRate(SYSCTL_PCLK_TIMER0);

	Chip_TIMER_Reset(LPC_TIMER0);

	Chip_TIMER_MatchEnableInt(LPC_TIMER0, 1);
	Chip_TIMER_SetMatch(LPC_TIMER0, 1, (timerFreq / TICKRATE1_HZ));
	Chip_TIMER_ResetOnMatchDisable(LPC_TIMER0, 1);

	Chip_TIMER_MatchEnableInt(LPC_TIMER0, 2);
	Chip_TIMER_SetMatch(LPC_TIMER0, 2, (timerFreq / TICKRATE1_HZ));
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER0, 2);

	Chip_TIMER_Enable(LPC_TIMER0);

	/* Enable timer interrupt */
	NVIC_EnableIRQ(TIMER0_IRQn);
	NVIC_ClearPendingIRQ(TIMER0_IRQn);

}

void TIMER0_IRQHandler ( void )
{
	 static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	 if (Chip_TIMER_MatchPending(LPC_TIMER0, 1))
	 {
		 Chip_TIMER_ClearMatch(LPC_TIMER0, 1);
		 xSemaphoreGiveFromISR( SemaforoSincronizacion, &xHigherPriorityTaskWoken );
	 }
	 if (Chip_TIMER_MatchPending(LPC_TIMER0, 2))
	 {
		 Chip_TIMER_ClearMatch(LPC_TIMER0, 2);
		 xSemaphoreGiveFromISR( SemaforoSincronizacion, &xHigherPriorityTaskWoken );
	 }

	 portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void uC_StartUp (void)
{
	//INICIALIZACION PERIFERICO GPIO
	Chip_GPIO_Init (LPC_GPIO);

	/* CONFIGURACION DE PINES */

	Chip_IOCON_PinMux (LPC_IOCON , PORT(LDR_PORT) ,   		PIN(LDR_PIN),  			IOCON_MODE_INACT , IOCON_FUNC0);
	Chip_IOCON_PinMux (LPC_IOCON , PORT(SHUT_DOWN_PORT) ,   PIN(SHUT_DOWN_PIN),  	IOCON_MODE_INACT , IOCON_FUNC0);
	Chip_IOCON_PinMux (LPC_IOCON , PORT(PULS_UP_PORT) ,   	PIN(PULS_UP_PIN), 		IOCON_MODE_INACT , IOCON_FUNC0);
	Chip_IOCON_PinMux (LPC_IOCON , PORT(PULS_DOWN_PORT) ,   PIN(PULS_DOWN_PIN),  	IOCON_MODE_INACT , IOCON_FUNC0);
	Chip_IOCON_PinMux (LPC_IOCON , PORT(AUTO_MANU_PORT) ,   PIN(AUTO_MANU_PIN),  	IOCON_MODE_INACT , IOCON_FUNC0);
	Chip_IOCON_PinMux (LPC_IOCON , PORT(PWM_PORT) ,   		PIN(PWM_PIN),  			IOCON_MODE_INACT , IOCON_FUNC0);

	/* FIN CONFIGURACION DE PINES */

	/* SETEO DE PINES */

	Chip_GPIO_SetDir (LPC_GPIO , PORT(LDR_PORT) ,   		PIN(LDR_PIN), 		INPUT);
	Chip_GPIO_SetDir (LPC_GPIO , PORT(SHUT_DOWN_PORT) ,   	PIN(SHUT_DOWN_PIN), INPUT);
	Chip_GPIO_SetDir (LPC_GPIO , PORT(PULS_UP_PORT) ,   	PIN(PULS_UP_PIN), 	INPUT);
	Chip_GPIO_SetDir (LPC_GPIO , PORT(PULS_DOWN_PORT) ,   	PIN(PULS_DOWN_PIN), INPUT);
	Chip_GPIO_SetDir (LPC_GPIO , PORT(AUTO_MANU_PORT) ,   	PIN(AUTO_MANU_PIN), INPUT);
	Chip_GPIO_SetDir (LPC_GPIO , PORT(PWM_PORT) ,   		PIN(PWM_PIN), 		OUTPUT);

	/* FIN SETEO DE PINES */
}

void xTimerTask (void *pvParameters)
{
	static uint8_t LED_State = OFF;

	while (1)
	{
		xSemaphoreTake( SemaforoSincronizacion, portMAX_DELAY );

		if ( LED_State )
		{
			LED_State = OFF;
			Chip_GPIO_SetPinState( LPC_GPIO , PWM_PORT , PWM_PIN , OFF );
		}
		else
		{
			LED_State = ON;
			Chip_GPIO_SetPinState( LPC_GPIO , PWM_PORT , PWM_PIN , ON );
		}
	}
}

static void vTaskLectura(void *pvParameters)
{
	PWM_CFIG Pwm;

	while (1)
	{
		Pwm.DUTY = 0;
		Pwm.STATE = IDLE;

		if( Chip_GPIO_GetPinState( LPC_GPIO , PORT(SHUT_DOWN_PORT) , PIN(SHUT_DOWN_PIN)) == (bool)PRESSED )
		{
			Pwm.DUTY = 0;
			Pwm.STATE = OFF;
		}
		else
		{
			if( Chip_GPIO_GetPinState( LPC_GPIO , PORT(AUTO_MANU_PORT) , PIN(AUTO_MANU_PIN)) == (bool)PRESSED )
			{
				if( Chip_GPIO_GetPinState( LPC_GPIO , PORT(PULS_UP_PORT) , PIN(PULS_UP_PIN)) == (bool)PRESSED )
				{
					Pwm.DUTY = 0;
					Pwm.STATE = MANUAL_UP;
				}
				else if( Chip_GPIO_GetPinState( LPC_GPIO , PORT(PULS_DOWN_PORT) , PIN(PULS_DOWN_PIN)) == (bool)PRESSED )
				{
					Pwm.DUTY = 0;
					Pwm.STATE = MANUAL_DOWN;
				}
				else
				{
					Pwm.DUTY = 0;
					Pwm.STATE = MANUAL;
				}
			}
			else
			{
				if( Chip_GPIO_GetPinState( LPC_GPIO , PORT(LDR_PORT) , PIN(LDR_PIN)) == (bool)PRESSED )
				{
					Pwm.DUTY = 80;
					Pwm.STATE = AUTO;
				}
				else
				{
					Pwm.DUTY = 50;
					Pwm.STATE = AUTO;
				}
			}
		}

		if( Pwm.STATE != IDLE )
		{
			xQueueSendToBack( Queue_Pwm , &Pwm , portMAX_DELAY );
		}
	}
}

static void xTaskEscritura(void *pvParameters)
{
	PWM_CFIG SalidaPWM;
	uint8_t 	 Duty = 0;

	while (1)
	{
		xQueueReceive( Queue_Pwm , &SalidaPWM , portMAX_DELAY );

		if ( SalidaPWM.STATE != IDLE )
		{
			switch( SalidaPWM.STATE )
			{
				case OFF:
					Chip_TIMER_Disable(LPC_TIMER0);
				break;

				case ON:
					Chip_TIMER_Enable(LPC_TIMER0);
				break;

				case AUTO:
					Chip_TIMER_Enable(LPC_TIMER0);

					Duty = GetDutyCycle();
					SetDutyCycle( Duty );
				break;

				case MANUAL:
					Chip_TIMER_Enable(LPC_TIMER0);

					Duty = GetDutyCycle();
					SetDutyCycle( Duty );

				break;

				case MANUAL_DOWN:
					Chip_TIMER_Enable(LPC_TIMER0);

					Duty = GetDutyCycle();
					if(Duty > 10)
						SetDutyCycle( Duty - 10 );
				break;

				case MANUAL_UP:
					Chip_TIMER_Enable(LPC_TIMER0);

					Duty = GetDutyCycle();
					if(Duty < 90)
					SetDutyCycle( Duty + 10 );
				break;
			}
		}
	}
}

int main(void)
{
	uC_StartUp ();
	SystemCoreClockUpdate();
	Configuracion_Timer();

	PWM_Struct.DUTY = 0;
	PWM_Struct.STATE = IDLE;

	vSemaphoreCreateBinary(SemaforoSincronizacion);

	Queue_Pwm = xQueueCreate( 5 , sizeof(PWM_CFIG) );

	xTaskCreate(	vTaskLectura,
					(char *) "vTaskLectura",
					configMINIMAL_STACK_SIZE,
					&PWM_Struct,
					(tskIDLE_PRIORITY + 1UL),
					(xTaskHandle *) Handle_Tarea_Lectura
	);

	xTaskCreate(	xTaskEscritura,
					(char *) "xTaskEscritura",
					configMINIMAL_STACK_SIZE,
					&PWM_Struct,
					(tskIDLE_PRIORITY + 1UL),
					(xTaskHandle *) Handle_Tarea_Escritura
	);

	xTaskCreate( 	xTimerTask,
					(char *) "Tarea Timer",
					configMINIMAL_STACK_SIZE,
					NULL,
					(tskIDLE_PRIORITY + 1UL),
					(xTaskHandle *) Handle_Tarea_Timer
	);

	vTaskStartScheduler();

	return 0;
}

