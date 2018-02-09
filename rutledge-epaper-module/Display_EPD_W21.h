
#include "rutledge_spi_porting/epd_spi.h"
#include "rutledge-common/common.h"
#include "fontsepd.h"
#include "image.h"


#ifndef _DISPLAY_EPD_W21_H_
#define _DISPLAY_EPD_W21_H_


#define  EPD_WIDTH    ((uint16_t)295)
#define  EPD_HEIGHT   ((uint16_t)127)

#define EPD_W21_WRITE_DATA 1
#define EPD_W21_WRITE_CMD  0


#define EPD_W21_SPI_SPEED 0x01


extern const unsigned char LUTDefault_part[31];


void EPD_W21_POWERON(void);
void EPD_W21_POWEROFF(void);
void EPD_W21_Update(unsigned char cmdUpdate);
void EPD_W21_EnableChargepump(void);
void EPD_W21_DisableChargepump(void);
void EPD_W21_WriteLUT(unsigned char *LUTvalue);
void EPD_W21_SetRamPointer(unsigned char addrX,unsigned char addrY,unsigned char addrY1);
void EPD_W21_SetRamArea(unsigned char Xstart,unsigned char Xend,
						unsigned char Ystart,unsigned char Ystart1,unsigned char Yend,unsigned char Yend1);

void EPD_W21_SetRamPointer_bc(unsigned int addrX, unsigned int addrY);
void EPD_W21_SetRamArea_bc(unsigned char Xstart, unsigned char Xend,
		unsigned int Ystart, unsigned int Yend);



void EPD_init(int selectLut) ;
void EPD_W21_UpdataDisplay(unsigned char *imgbuff,unsigned char xram,unsigned int yram);
void  part_display(unsigned char RAM_XST,unsigned char RAM_XEND,unsigned char RAM_YST,unsigned char RAM_YST1,unsigned char RAM_YEND,unsigned char RAM_YEND1);
void EPD_W21_WriteDispRam(unsigned char XSize,unsigned int YSize,
							unsigned char *Dispbuff);
void EPD_W21_WriteDispRamMono(unsigned char XSize,unsigned int YSize,
							unsigned char dispdata);

/***********************************************************
						end old file
***********************************************************/
extern sFONT *pFont;

void EPD_clean(void);
void EPD_update_image( unsigned char *data);


// newporting  TODO: cannb

typedef enum
{
	TEXT_CENTER_MODE = 0x01,
	TEXT_RIGHT_MODE  = 0x02,
	TEXT_LEFT_MODE   = 0x03
} Text_AlignModeTypdef;


uint32_t GetXSize(void);

uint32_t GetYSize(void);

void SetFont(sFONT *pFonts);

sFONT *GetFont(void);
void Clear(uint16_t Color);

void DisplayChar(uint16_t Xpos, uint16_t Ypos, uint8_t Ascii);
void DisplayStringAt(uint16_t Xpos, uint16_t Ypos, char *Text,
        Text_AlignModeTypdef Mode);

void EPD_W21_POWERPIN_HIGH(void);
void EPD_W21_POWERPIN_LOW(void);
void EPD_UpdateDisplay();
void EPD_PrintDefaultInfo();
void EPD_DisplayScanInfo(uint8_t index, char * info);


#endif
