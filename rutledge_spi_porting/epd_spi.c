/**************************************************************************//**

 *
 ******************************************************************************/

#include "epd_spi.h"
#include "em_cmu.h"
#include "em_usart.h"
#ifdef HAL_CONFIG
  #include "hal-config.h"
  #include "ember-hal-config.h"
#endif

#include "thread-debug-print.h"
#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_HAL
#include "serial/com.h"
#include EMBER_AF_API_SERIAL

#define emberAfGuaranteedPrint(...) \
    emberSerialGuaranteedPrintf(APP_SERIAL, __VA_ARGS__)
  #define emberAfGuaranteedPrintln(format, ...) \
    emberSerialGuaranteedPrintf(APP_SERIAL, format "\r\n", __VA_ARGS__);

void SPI_Write(unsigned char value);

#ifdef EPD_USE_USART
#define USART_EPD_DEFAULT                                                              \
{                                                                                           \
  usartEnableTx,       /* Enable RX/TX when init completed. */                                \
  0,                 /* Use current configured reference clock for configuring baudrate. */ \
  2000000,           /* 1 Mbits/s. */                                                       \
  usartDatabits8,    /* 8 databits. */                                                      \
  true,              /* Master mode. */                                                     \
  true,             /* Send least significant bit first. */                                \
  usartClockMode2,   /* Clock idle low, sample on rising edge. */                           \
  false,             /* Not USART PRS input mode. */                                        \
  usartPrsRxCh0,     /* PRS channel 0. */                                                   \
  true,             /* No AUTOTX mode. */                                                  \
  false,             /* No AUTOCS mode */                                                   \
  0,                 /* Auto CS Hold cycles */                                              \
  0                  /* Auto CS Setup cycles */                                             \
}
#endif




/*
 * Function:       read status Busy of EPD
 * Arguments:      None.
 * Description:
 * Return Message: 1 busy, 0 otherwise.
 */
int16_t EPD_SPI_Busy_read(void) {
	int16_t ret = GPIO_PinInGet(EPD_PORT_BUSY, EPD_PIN_BUSY);
	//emberAfGuaranteedPrint("-%d",ret);
	return ret;
//	return GPIO_PinInGet(EPD_PORT_BUSY, EPD_PIN_BUSY);
}


/*
 * Function:       reset EPD
 * Arguments:      None.
 * Description:
 * Return Message: none.
 */
void EPD_SPI_Reset(void) {
	GPIO_PinOutClear(EPD_PORT_RESET,EPD_PIN_RESET);
	halCommonDelayMilliseconds(200);
	GPIO_PinOutSet(EPD_PORT_RESET,EPD_PIN_RESET);
	halCommonDelayMilliseconds(200);
}


/*
 * Function:       CS_Low, CS_High
 * Arguments:      None.
 * Description:    Chip select go low / high.
 * Return Message: None.
 */
void EPD_SPI_CS_Low(void) {
	GPIO_PinOutClear(EPD_PORT_CS, EPD_PIN_CS);
	halCommonDelayMicroseconds(1);
}

void EPD_SPI_CS_High(void) {
	halCommonDelayMicroseconds(1);
	GPIO_PinOutSet(EPD_PORT_CS, EPD_PIN_CS);
}

/*
 * Function:       DC_Low, DC_High
 * Arguments:      None.
 * Description:    Data or command
 * Return Message: None.
 */
void EPD_SPI_DC_Low(void) {

	GPIO_PinOutClear(EPD_PORT_DC, EPD_PIN_DC);
	halCommonDelayMicroseconds(1);
}

void EPD_SPI_DC_High(void) {
	GPIO_PinOutSet(EPD_PORT_DC, EPD_PIN_DC);
	halCommonDelayMicroseconds(1);
}



void EPD_SPI_CLK_Low(void) {
	GPIO_PinOutClear(EPD_PORT_CLK, EPD_PIN_CLK);
}

void EPD_SPI_CLK_High(void) {
	GPIO_PinOutSet(EPD_PORT_CLK, EPD_PIN_CLK);
}


void EPD_SPI_MOSI_Low(void) {
	GPIO_PinOutClear(EPD_PORT_MOSI, EPD_PIN_MOSI);
}

void EPD_SPI_MOSI_High(void) {
	GPIO_PinOutSet(EPD_PORT_MOSI, EPD_PIN_MOSI);
}

void EPD_SPI_RESET_Low(void) {
	GPIO_PinOutClear(EPD_PORT_RESET, EPD_PIN_RESET);
}

void EPD_SPI_RESET_High(void) {
	GPIO_PinOutSet(EPD_PORT_RESET, EPD_PIN_RESET);
}

void EPD_POWERON_Low(void) {
	GPIO_PinOutClear(EPD_POWER_ON_PORT, EPD_POWER_ON_PIN);
}

void EPD_POWERON_High(void) {
	GPIO_PinOutSet(EPD_POWER_ON_PORT, EPD_POWER_ON_PIN);
}

/**************************************************************************//**
 * @brief
 *  Initialize the SPI peripheral for EPD usage.
 *  SPI pins and speed etc. is defined in rutlegde-app-hal.h.
 *****************************************************************************/
void EPD_Spi_Init(void) {

#ifdef EPD_USE_USART
	USART_InitSync_TypeDef init = USART_EPD_DEFAULT;

	CMU_ClockEnable(cmuClock_GPIO, true);
	CMU_ClockEnable(EPD_USART_CLK, true);

	init.msbf = true;
	init.baudrate = 2000000;
	USART_InitSync(EPD_USART, &init);

	/* IO config for SPI*/
	GPIO_PinModeSet(EPD_PORT_MOSI, EPD_PIN_MOSI, gpioModePushPull, 1);
	GPIO_PinModeSet(EPD_PORT_DC, EPD_PIN_DC, gpioModePushPull, 1);
	GPIO_PinModeSet(EPD_PORT_CLK, EPD_PIN_CLK, gpioModePushPull, 1);
	GPIO_PinModeSet(EPD_PORT_CS, EPD_PIN_CS, gpioModePushPull, 1);

	GPIO_PinModeSet(EPD_PORT_RESET, EPD_PIN_RESET, gpioModePushPull, 1);

	GPIO_PinModeSet(EPD_PORT_BUSY, EPD_PIN_BUSY, gpioModeInputPull, 0);


	EPD_USART ->ROUTELOC0 =
			(/*USART_ROUTELOC0_RXLOC_LOC11 |*/USART_ROUTELOC0_TXLOC_LOC0
					| USART_ROUTELOC0_CLKLOC_LOC0);
	EPD_USART ->ROUTEPEN = (/*USART_ROUTEPEN_RXPEN |*/USART_ROUTEPEN_TXPEN
			| USART_ROUTEPEN_CLKPEN);
#else

	CMU_ClockEnable(cmuClock_GPIO, true);
	/* IO config for SPI*/
	GPIO_PinModeSet(EPD_PORT_MOSI, EPD_PIN_MOSI, gpioModePushPull, 1);
	GPIO_PinModeSet(EPD_PORT_DC, EPD_PIN_DC, gpioModePushPull, 1);
	GPIO_PinModeSet(EPD_PORT_CLK, EPD_PIN_CLK, gpioModePushPull, 1);
	GPIO_PinModeSet(EPD_PORT_CS, EPD_PIN_CS, gpioModePushPull, 1);

	GPIO_PinModeSet(EPD_PORT_RESET, EPD_PIN_RESET, gpioModePushPull, 1);

	//GPIO_PinModeSet(EPD_PORT_BS, EPD_PIN_BS, gpioModePushPull, 1);

	GPIO_PinModeSet(EPD_PORT_BUSY, EPD_PIN_BUSY, gpioModeInputPull, 0);
#endif
}

/**************************************************************************//**
 * @brief
 *  Deinitialize SPI peripheral.
 *  Turn off the SPI peripheral and disable SPI GPIO pins.
 *****************************************************************************/
void EPD_Spi_Deinit(void) {
	USART_Reset(EPD_USART );

	/* IO configuration (USART 0, Location #0) */

	GPIO_PinModeSet(EPD_PORT_MOSI, EPD_PIN_MOSI, gpioModeDisabled, 0);
	GPIO_PinModeSet(EPD_PORT_DC, EPD_PIN_DC, gpioModeDisabled, 0);
	GPIO_PinModeSet(EPD_PORT_CLK, EPD_PIN_CLK, gpioModeDisabled, 0);
	GPIO_PinModeSet(EPD_PORT_CS, EPD_PIN_CS, gpioModeDisabled, 0);

	GPIO_PinModeSet(EPD_PORT_CS, EPD_PIN_CS, gpioModeDisabled, 0);

	GPIO_PinModeSet(EPD_PORT_BUSY, EPD_PIN_BUSY, gpioModeDisabled, 0);
}

/**************************************************************************//**
 * @brief
 *  Do one SPI transfer.
 *
 * @param data
 *  Byte to transmit.
 *
 * @return
 *  none
 **********************************************
 *****************************************************************************/
void EPD_SPI_Xfer(uint8_t data) {

#if 1
SPI_Write(data);
#else
	while (!(EPD_USART->STATUS & USART_STATUS_TXBL))
	    ;
	EPD_USART->TXDATA = (uint32_t)data;

//	  while (!(EPD_USART->STATUS & USART_STATUS_TXC))
//	    ;
//	  return (uint8_t)EPD_USART->RXDATA;

	//return USART_SpiTransfer(EPD_USART, data);
#endif
}
