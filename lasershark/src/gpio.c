/****************************************************************************
 *   $Id:: gpio.c 6172 2011-01-13 18:22:51Z usb00423                        $
 *   Project: NXP LPC11Uxx GPIO example
 *
 *   Description:
 *     This file contains GPIO code example which include GPIO 
 *     initialization, GPIO interrupt handler, and related APIs for 
 *     GPIO access.
 *
 ****************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
****************************************************************************/
#include "LPC13Uxx.h"			/* LPC11Uxx Peripheral Registers */
#include "gpio.h"

volatile uint32_t PIN_INT0_counter = 0;
volatile uint32_t PIN_INT1_counter = 0;
volatile uint32_t PIN_INT2_counter = 0;
volatile uint32_t PIN_INT3_counter = 0;
volatile uint32_t PIN_INT4_counter = 0;
volatile uint32_t PIN_INT5_counter = 0;
volatile uint32_t PIN_INT6_counter = 0;
volatile uint32_t PIN_INT7_counter = 0;
volatile uint32_t gint0_counter = 0;
volatile uint32_t gint1_counter = 0;
volatile uint32_t PIN_INT0_level_counter = 0;
volatile uint32_t PIN_INT0_rising_edge_counter = 0;
volatile uint32_t PIN_INT0_falling_edge_counter = 0;
volatile uint32_t PIN_INT1_level_counter = 0;
volatile uint32_t PIN_INT1_rising_edge_counter = 0;
volatile uint32_t PIN_INT1_falling_edge_counter = 0;
volatile uint32_t PIN_INT2_level_counter = 0;
volatile uint32_t PIN_INT2_rising_edge_counter = 0;
volatile uint32_t PIN_INT2_falling_edge_counter = 0;
volatile uint32_t PIN_INT3_level_counter = 0;
volatile uint32_t PIN_INT3_rising_edge_counter = 0;
volatile uint32_t PIN_INT3_falling_edge_counter = 0;
volatile uint32_t PIN_INT4_level_counter = 0;
volatile uint32_t PIN_INT4_rising_edge_counter = 0;
volatile uint32_t PIN_INT4_falling_edge_counter = 0;
volatile uint32_t PIN_INT5_level_counter = 0;
volatile uint32_t PIN_INT5_rising_edge_counter = 0;
volatile uint32_t PIN_INT5_falling_edge_counter = 0;
volatile uint32_t PIN_INT6_level_counter = 0;
volatile uint32_t PIN_INT6_rising_edge_counter = 0;
volatile uint32_t PIN_INT6_falling_edge_counter = 0;
volatile uint32_t PIN_INT7_level_counter = 0;
volatile uint32_t PIN_INT7_rising_edge_counter = 0;
volatile uint32_t PIN_INT7_falling_edge_counter = 0;
volatile uint32_t gint0_level_counter = 0;
volatile uint32_t gint0_edge_counter = 0;
volatile uint32_t gint1_level_counter = 0;
volatile uint32_t gint1_edge_counter = 0;

/*****************************************************************************
** Function name:		PIN_INT0_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void PIN_INT0_IRQHandler(void)
{
  PIN_INT0_counter++;
  if ( LPC_GPIO_PIN_INT->IST & (0x1<<0) )
  {
	if ( LPC_GPIO_PIN_INT->ISEL & (0x1<<0) )
	{
	  PIN_INT0_level_counter++;
	}
	else
	{
	  if ( LPC_GPIO_PIN_INT->RISE & (0x1<<0) )
	  {
		PIN_INT0_rising_edge_counter++;
		LPC_GPIO_PIN_INT->RISE = 0x1<<0;
	  }
	  else if ( LPC_GPIO_PIN_INT->FALL & (0x1<<0) )
	  {
		PIN_INT0_falling_edge_counter++;
		LPC_GPIO_PIN_INT->FALL = 0x1<<0;
	  }
	  LPC_GPIO_PIN_INT->IST = 0x1<<0;
	}
  }		
  return;
}

/*****************************************************************************
** Function name:		PIN_INT1_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void PIN_INT1_IRQHandler(void)
{
  PIN_INT1_counter++;
  if ( LPC_GPIO_PIN_INT->IST & (0x1<<1) )
  {
	if ( LPC_GPIO_PIN_INT->ISEL & (0x1<<1) )
	{
	  PIN_INT1_level_counter++;
	}
	else
	{
	  if ( LPC_GPIO_PIN_INT->RISE & (0x1<<1) )
	  {
		PIN_INT1_rising_edge_counter++;
		LPC_GPIO_PIN_INT->RISE = 0x1<<1;
	  }
	  else if ( LPC_GPIO_PIN_INT->FALL & (0x1<<1) )
	  {
		PIN_INT1_falling_edge_counter++;
		LPC_GPIO_PIN_INT->FALL = 0x1<<1;
	  }
	  LPC_GPIO_PIN_INT->IST = 0x1<<1;
	}
  }		
  return;
}

/*****************************************************************************
** Function name:		PIN_INT2_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void PIN_INT2_IRQHandler(void)
{
  PIN_INT2_counter++;
  if ( LPC_GPIO_PIN_INT->IST & (0x1<<2) )
  {
	if ( LPC_GPIO_PIN_INT->ISEL & (0x1<<2) )
	{
	  PIN_INT2_level_counter++;
	}
	else
	{
	  if ( LPC_GPIO_PIN_INT->RISE & (0x1<<2) )
	  {
		PIN_INT2_rising_edge_counter++;
		LPC_GPIO_PIN_INT->RISE = 0x1<<2;
	  }
	  else if ( LPC_GPIO_PIN_INT->FALL & (0x1<<2) )
	  {
		PIN_INT2_falling_edge_counter++;
		LPC_GPIO_PIN_INT->FALL = 0x1<<2;
	  }
	  LPC_GPIO_PIN_INT->IST = 0x1<<2;
	}
  }		
  return;
}

/*****************************************************************************
** Function name:		PIN_INT3_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void PIN_INT3_IRQHandler(void)
{
  PIN_INT3_counter++;
  if ( LPC_GPIO_PIN_INT->IST & (0x1<<3) )
  {
	if ( LPC_GPIO_PIN_INT->ISEL & (0x1<<3) )
	{
	  PIN_INT3_level_counter++;
	}
	else
	{
	  if ( LPC_GPIO_PIN_INT->RISE & (0x1<<3) )
	  {
		PIN_INT3_rising_edge_counter++;
		LPC_GPIO_PIN_INT->RISE = 0x1<<3;
	  }
	  else if ( LPC_GPIO_PIN_INT->FALL & (0x1<<3) )
	  {
		PIN_INT3_falling_edge_counter++;
		LPC_GPIO_PIN_INT->FALL = 0x1<<3;
	  }
	  LPC_GPIO_PIN_INT->IST = 0x1<<3;
	}
  }		
  return;
}

/*****************************************************************************
** Function name:		PIN_INT4_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void PIN_INT4_IRQHandler(void)
{
  PIN_INT4_counter++;
  if ( LPC_GPIO_PIN_INT->IST & (0x1<<4) )
  {
	if ( LPC_GPIO_PIN_INT->ISEL & (0x1<<4) )
	{
	  PIN_INT4_level_counter++;
	}
	else
	{
	  if ( LPC_GPIO_PIN_INT->RISE & (0x1<<4) )
	  {
		PIN_INT4_rising_edge_counter++;
		LPC_GPIO_PIN_INT->RISE = 0x1<<4;
	  }
	  else if ( LPC_GPIO_PIN_INT->FALL & (0x1<<4) )
	  {
		PIN_INT4_falling_edge_counter++;
		LPC_GPIO_PIN_INT->FALL = 0x1<<4;
	  }
	  LPC_GPIO_PIN_INT->IST = 0x1<<4;
	}
  }		
  return;
}

/*****************************************************************************
** Function name:		PIN_INT5_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void PIN_INT5_IRQHandler(void)
{
  PIN_INT5_counter++;
  if ( LPC_GPIO_PIN_INT->IST & (0x1<<5) )
  {
	if ( LPC_GPIO_PIN_INT->ISEL & (0x1<<5) )
	{
	  PIN_INT5_level_counter++;
	}
	else
	{
	  if ( LPC_GPIO_PIN_INT->RISE & (0x1<<5) )
	  {
		PIN_INT5_rising_edge_counter++;
		LPC_GPIO_PIN_INT->RISE = 0x1<<5;
	  }
	  else if ( LPC_GPIO_PIN_INT->FALL & (0x1<<5) )
	  {
		PIN_INT5_falling_edge_counter++;
		LPC_GPIO_PIN_INT->FALL = 0x1<<5;
	  }
	  LPC_GPIO_PIN_INT->IST = 0x1<<5;
	}
  }		
  return;
}

/*****************************************************************************
** Function name:		PIN_INT6_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void PIN_INT6_IRQHandler(void)
{
  PIN_INT6_counter++;
  if ( LPC_GPIO_PIN_INT->IST & (0x1<<6) )
  {
	if ( LPC_GPIO_PIN_INT->ISEL & (0x1<<6) )
	{
	  PIN_INT6_level_counter++;
	}
	else
	{
	  if ( LPC_GPIO_PIN_INT->RISE & (0x1<<6) )
	  {
		PIN_INT6_rising_edge_counter++;
		LPC_GPIO_PIN_INT->RISE = 0x1<<6;
	  }
	  else if ( LPC_GPIO_PIN_INT->FALL & (0x1<<6) )
	  {
		PIN_INT6_falling_edge_counter++;
		LPC_GPIO_PIN_INT->FALL = 0x1<<6;
	  }
	  LPC_GPIO_PIN_INT->IST = 0x1<<6;
	}
  }		
  return;
}

/*****************************************************************************
** Function name:		PIN_INT7_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void PIN_INT7_IRQHandler(void)
{
  PIN_INT7_counter++;
  if ( LPC_GPIO_PIN_INT->IST & (0x1<<7) )
  {
	if ( LPC_GPIO_PIN_INT->ISEL & (0x1<<7) )
	{
	  PIN_INT7_level_counter++;
	}
	else
	{
	  if ( LPC_GPIO_PIN_INT->RISE & (0x1<<7) )
	  {
		PIN_INT7_rising_edge_counter++;
		LPC_GPIO_PIN_INT->RISE = 0x1<<7;
	  }
	  else if ( LPC_GPIO_PIN_INT->FALL & (0x1<<7) )
	  {
		PIN_INT7_falling_edge_counter++;
		LPC_GPIO_PIN_INT->FALL = 0x1<<7;
	  }
	  LPC_GPIO_PIN_INT->IST = 0x1<<7;
	}
  }		
  return;
}

/*****************************************************************************
** Function name:		GINT0_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void GINT0_IRQHandler(void)
{
  gint0_counter++;
  if ( LPC_GPIO_GROUP_INT0->CTRL & 0x1 )
  {
	if ( LPC_GPIO_GROUP_INT0->CTRL & (0x1<<4) )
	{
	  gint0_level_counter++;
	}
	else
	{
	  gint0_edge_counter++;
	}
	LPC_GPIO_GROUP_INT0->CTRL |= 0x1;
  }		
  return;
}

/*****************************************************************************
** Function name:		GINT1_IRQHandler
**
** Descriptions:		Use one GPIO pin as interrupt source
**
** parameters:			None
** 						
** Returned value:		None
** 
*****************************************************************************/
void GINT1_IRQHandler(void)
{
  gint1_counter++;
  if ( LPC_GPIO_GROUP_INT1->CTRL & 0x1 )
  {
	if ( LPC_GPIO_GROUP_INT1->CTRL & (0x1<<4) )
	{
	  gint1_level_counter++;
	}
	else
	{
	  gint1_edge_counter++;
	}
	LPC_GPIO_GROUP_INT1->CTRL |= 0x1;
  }		
  return;
}

/*****************************************************************************
** Function name:		GPIOInit
**
** Descriptions:		Initialize GPIO, install the
**						GPIO interrupt handler
**
** parameters:			None
** 						
** Returned value:		true or false, return false if the VIC table
**						is full and GPIO interrupt handler can be
**						installed.
** 
*****************************************************************************/
void GPIOInit( void )
{
  /* Enable AHB clock to the GPIO domain. */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);

  /* Enable AHB clock to the FlexInt, GroupedInt domain. */
  LPC_SYSCON->SYSAHBCLKCTRL |= ((1<<19) | (1<<23) | (1<<24));

  return;
}

/*****************************************************************************
** Function name:		GPIOSetFlexInterrupt
**
** Descriptions:		Set interrupt sense, event, etc.
**						sense: edge or level, 0 is edge, 1 is level 
**						event/polarity: 0 is active low/falling, 1 is high/rising.
**
** parameters:			channel #, port #, bit position, sense, event(polarity)
** 						
** Returned value:		None
** 
*****************************************************************************/
void GPIOSetFlexInterrupt( uint32_t channelNum, uint32_t portNum, uint32_t bitPosi,
		uint32_t sense, uint32_t event )
{
  switch ( channelNum )
  {
	case CHANNEL0:
	  if ( portNum )
	  {
		LPC_SYSCON->PINSEL[0] = bitPosi + 24;
	  }
	  else
	  {
		LPC_SYSCON->PINSEL[0] = bitPosi;
	  }
	  NVIC_EnableIRQ(PIN_INT0_IRQn);
	break;
	case CHANNEL1:
	  if ( portNum )
	  {
		LPC_SYSCON->PINSEL[1] = bitPosi + 24;
	  }
	  else
	  {
		LPC_SYSCON->PINSEL[1] = bitPosi;
	  }
	  NVIC_EnableIRQ(PIN_INT1_IRQn);
	break;
	case CHANNEL2:
	  if ( portNum )
	  {
		LPC_SYSCON->PINSEL[2] = bitPosi + 24;
	  }
	  else
	  {
		LPC_SYSCON->PINSEL[2] = bitPosi;
	  }
	  NVIC_EnableIRQ(PIN_INT2_IRQn);
	break;
	case CHANNEL3:
	  if ( portNum )
	  {
		LPC_SYSCON->PINSEL[3] = bitPosi + 24;
	  }
	  else
	  {
		LPC_SYSCON->PINSEL[3] = bitPosi;
	  }
	  NVIC_EnableIRQ(PIN_INT3_IRQn);
	break;
	case CHANNEL4:
	  if ( portNum )
	  {
		LPC_SYSCON->PINSEL[4] = bitPosi + 24;
	  }
	  else
	  {
		LPC_SYSCON->PINSEL[4] = bitPosi;
	  }
	  NVIC_EnableIRQ(PIN_INT4_IRQn);
	break;
	case CHANNEL5:
	  if ( portNum )
	  {
		LPC_SYSCON->PINSEL[5] = bitPosi + 24;
	  }
	  else
	  {
		LPC_SYSCON->PINSEL[5] = bitPosi;
	  }
	  NVIC_EnableIRQ(PIN_INT5_IRQn);
	break;
	case CHANNEL6:
	  if ( portNum )
	  {
		LPC_SYSCON->PINSEL[6] = bitPosi + 24;
	  }
	  else
	  {
		LPC_SYSCON->PINSEL[6] = bitPosi;
	  }
	  NVIC_EnableIRQ(PIN_INT6_IRQn);
	break;
	case CHANNEL7:
	  if ( portNum )
	  {
		LPC_SYSCON->PINSEL[7] = bitPosi + 24;
	  }
	  else
	  {
		LPC_SYSCON->PINSEL[7] = bitPosi;
	  }
	  NVIC_EnableIRQ(PIN_INT7_IRQn);
	break;
	default:
	  break;
  }
  if ( sense == 0 )
  {
	LPC_GPIO_PIN_INT->ISEL &= ~(0x1<<channelNum);	/* Edge trigger */
	if ( event == 0 )
	{
	  LPC_GPIO_PIN_INT->IENF |= (0x1<<channelNum);	/* faling edge */
	}
	else
	{
	  LPC_GPIO_PIN_INT->IENR |= (0x1<<channelNum);	/* Rising edge */
	}
  }
  else
  {
	LPC_GPIO_PIN_INT->ISEL |= (0x1<<channelNum);	/* Level trigger. */
	LPC_GPIO_PIN_INT->IENR |= (0x1<<channelNum);	/* Level enable */
	if ( event == 0 )
	{
	  LPC_GPIO_PIN_INT->IENF &= ~(0x1<<channelNum);	/* active-low */
	}
	else
	{
	  LPC_GPIO_PIN_INT->IENF |= (0x1<<channelNum);	/* active-high */
	}
  }

  return;
}

/*****************************************************************************
** Function name:		GPIOFlexIntEnable
**
** Descriptions:		Enable Interrupt
**
** parameters:			channel num, event(0 is falling edge, 1 is rising edge)
** Returned value:		None
** 
*****************************************************************************/
void GPIOFlexIntEnable( uint32_t channelNum, uint32_t event )
{
  if ( !( LPC_GPIO_PIN_INT->ISEL & (0x1<<channelNum) ) )
  {
	if ( event == 0 )
	{
	  LPC_GPIO_PIN_INT->SIENF |= (0x1<<channelNum);	/* faling edge */
	}
	else
	{
	  LPC_GPIO_PIN_INT->SIENR |= (0x1<<channelNum);	/* Rising edge */
	}
  }
  else
  {
	LPC_GPIO_PIN_INT->SIENR |= (0x1<<channelNum);	/* Level */
  }
  return;
}

/*****************************************************************************
** Function name:		GPIOFlexIntDisable
**
** Descriptions:		Disable Interrupt
**
** parameters:			channel num, event(0 is falling edge, 1 is rising edge)
** 						
** Returned value:		None
** 
*****************************************************************************/
void GPIOFlexIntDisable( uint32_t channelNum, uint32_t event )
{
  if ( !( LPC_GPIO_PIN_INT->ISEL & (0x1<<channelNum) ) )
  {
	if ( event == 0 )
	{
	  LPC_GPIO_PIN_INT->CIENF |= (0x1<<channelNum);	/* faling edge */
	}
	else
	{
	  LPC_GPIO_PIN_INT->CIENR |= (0x1<<channelNum);	/* Rising edge */
	}
  }
  else
  {
	LPC_GPIO_PIN_INT->CIENR |= (0x1<<channelNum);	/* Level */
  }
  return;
}

/*****************************************************************************
** Function name:		GPIOFlexIntStatus
**
** Descriptions:		Get Interrupt status
**
** parameters:			channel num
** 						
** Returned value:		None
** 
*****************************************************************************/
uint32_t GPIOFlexIntStatus( uint32_t channelNum )
{
  if ( LPC_GPIO_PIN_INT->IST & (0x1<<channelNum) )
  {
	return( 1 );
  }
  else
  {
	return( 0 );
  }
}

/*****************************************************************************
** Function name:		GPIOFlexIntClear
**
** Descriptions:		Clear Interrupt
**
** parameters:			channel num
** 						
** Returned value:		None
** 
*****************************************************************************/
void GPIOFlexIntClear( uint32_t channelNum )
{
  if ( !( LPC_GPIO_PIN_INT->ISEL & (0x1<<channelNum) ) )
  {
	LPC_GPIO_PIN_INT->IST = (1<<channelNum);
  }
  return;
}

/*****************************************************************************
** Function name:		GPIOSetGroupedInterrupt
**
** Descriptions:		Set interrupt logic, sense, eventPattern, etc.
**						logic: AND or OR, 0 is OR, 1 is AND 
**						sensePattern: edge or level, 0 is edge, 1 is level 
**						event/polarity: 0 is active low/falling, 1 is high/rising.
**
** parameters:			group #, bit pattern, logic, sense, event(polarity) pattern
** 						
** Returned value:		None
** 
*****************************************************************************/
void GPIOSetGroupedInterrupt( uint32_t groupNum, uint32_t *bitPattern, uint32_t logic,
		uint32_t sense, uint32_t *eventPattern )
{
  switch ( groupNum )
  {
	case GROUP0:
	  if ( sense == 0 )
	  {
		LPC_GPIO_GROUP_INT0->CTRL &= ~(0x1<<2);	/* Edge trigger */
	  }
	  else
	  {
		LPC_GPIO_GROUP_INT0->CTRL |= (0x1<<2);	/* Level trigger. */
	  }
	  LPC_GPIO_GROUP_INT0->CTRL |= (logic<<1);
	  LPC_GPIO_GROUP_INT0->PORT_POL[0] = *((uint32_t *)(eventPattern + 0));
	  LPC_GPIO_GROUP_INT0->PORT_POL[1] = *((uint32_t *)(eventPattern + 1));
	  LPC_GPIO_GROUP_INT0->PORT_ENA[0] = *((uint32_t *)(bitPattern + 0));
	  LPC_GPIO_GROUP_INT0->PORT_ENA[1] = *((uint32_t *)(bitPattern + 1));
      /* as soon as enabled, an edge may be generated       */
	  /* clear interrupt flag and NVIC pending interrupt to */
	  /* workaround the potential edge generated as enabled */
	  LPC_GPIO_GROUP_INT0->CTRL |= (1<<0);
	  NVIC_ClearPendingIRQ(GINT0_IRQn);
	  NVIC_EnableIRQ(GINT0_IRQn);
	break;
	case GROUP1:
	  if ( sense == 0 )
	  {
		LPC_GPIO_GROUP_INT1->CTRL &= ~(0x1<<2);	/* Edge trigger */
	  }
	  else
	  {
		LPC_GPIO_GROUP_INT1->CTRL |= (0x1<<2);	/* Level trigger. */
	  }
	  LPC_GPIO_GROUP_INT1->CTRL |= (logic<<1);
	  LPC_GPIO_GROUP_INT1->PORT_POL[0] = *((uint32_t *)(eventPattern + 0));
	  LPC_GPIO_GROUP_INT1->PORT_POL[1] = *((uint32_t *)(eventPattern + 1));
	  LPC_GPIO_GROUP_INT1->PORT_ENA[0] = *((uint32_t *)(bitPattern + 0));
	  LPC_GPIO_GROUP_INT1->PORT_ENA[1] = *((uint32_t *)(bitPattern + 1));
      /* as soon as enabled, an edge may be generated       */
	  /* clear interrupt flag and NVIC pending interrupt to */
	  /* workaround the potential edge generated as enabled */
	  LPC_GPIO_GROUP_INT1->CTRL |= (1<<0);
	  NVIC_ClearPendingIRQ(GINT1_IRQn);
	  NVIC_EnableIRQ(GINT1_IRQn);
	break;
	default:
	  break;
  }

  return;
}

/*****************************************************************************
** Function name:		GPIOGetPinValue
**
** Descriptions:		Read Current state of port pin, PIN register value
**
** parameters:			port num, bit position
** Returned value:		None
**
*****************************************************************************/
uint32_t GPIOGetPinValue( uint32_t portNum, uint32_t bitPosi )
{
  uint32_t regVal = 0;	

  if( bitPosi < 0x20 )
  {	
	if ( LPC_GPIO->PIN[portNum] & (0x1<<bitPosi) )
	{
	  regVal = 1;
	}
  }
  else if( bitPosi == 0xFF )
  {
	regVal = LPC_GPIO->PIN[portNum];
  }
  return ( regVal );		
}

/*****************************************************************************
** Function name:		GPIOSetBitValue
**
** Descriptions:		Set/clear a bit in a specific position
**
** parameters:			port num, bit position, bit value
** 						
** Returned value:		None
**
*****************************************************************************/
void GPIOSetBitValue( uint32_t portNum, uint32_t bitPosi, uint32_t bitVal )
{
  if ( bitVal )
  {
	LPC_GPIO->SET[portNum] = 1<<bitPosi;
  }
  else
  {
	LPC_GPIO->CLR[portNum] = 1<<bitPosi;
  }
  return;
}

void GPIOToggleValue( uint32_t portNum, uint32_t bitPosi ){
  LPC_GPIO->NOT[portNum] = 1;
}

/*****************************************************************************
** Function name:		GPIOSetDir
**
** Descriptions:		Set the direction in GPIO port
**
** parameters:			portNum, bit position, direction (1 out, 0 input)
** 						
** Returned value:		None
**
*****************************************************************************/
void GPIOSetDir( uint32_t portNum, uint32_t bitPosi, uint32_t dir )
{
  if( dir )
  {
	LPC_GPIO->DIR[portNum] |= (1<<bitPosi);
  }
  else
  {
	LPC_GPIO->DIR[portNum] &= ~(1<<bitPosi);
  }
  return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
