/**************************************************************************//**
 * @file
 * @brief
 * @version
 ******************************************************************************
 * @section License
 * <b></b>
 *******************************************************************************
 *
 *
 *
 ******************************************************************************/

#ifndef __EPD_SPI_H
#define __EPD_SPI_H

#include "em_device.h"
#include "em_gpio.h"



#define EPD_HI_SPI_FREQ     24000000

#define EPD_LO_SPI_FREQ     100000

void EPD_SPI_CS_Low(void);

void EPD_SPI_CS_High(void);

void EPD_SPI_DC_Low(void);

void EPD_SPI_DC_High(void);

void EPD_POWERON_High(void) ;
void EPD_POWERON_Low(void);


// TODO: FIXME, cannb: tmp remove later
void EPD_SPI_CLK_Low(void);
void EPD_SPI_CLK_High(void) ;
void EPD_SPI_MOSI_Low(void);
void EPD_SPI_MOSI_High(void);
void EPD_SPI_RESET_Low(void) ;
void EPD_SPI_RESET_High(void);



int16_t EPD_SPI_Busy_read(void);
void EPD_SPI_Reset(void);

void EPD_Spi_Init(void);
void EPD_SPI_Xfer(uint8_t data);



#endif /* EPD_SPI */
