/***********************************************************
 Copyright(C), Yk Technology
 FileName	:
 Author		: Yukewantong, Zhongxiaodong
 Date		  	: 2011/12/22
 Description	:
 Version		: V1.0
 History		:
 --------------------------------
 2011/12/22: created
 2012/02/29: update the pvi's waveform(20C) as default
 2012/03/18: update the EPD initinal step, add function-->
 2016/10/02: TODO: cannb, update the EPD API draw char, string
 void EPD_W21_PowerOnInit(void)
 ***********************************************************/
#include "Display_EPD_W21.h"
#include "hal.h"
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

sFONT *pFont;
/* for 2.9inch  Command: Panel configuration
 * (296 (0x127) MUX, SM = 0 : Source gate
scanning direction)*/
unsigned char GDOControl[] = { 0x01, 0x27, 0x01, 0x00 };
//unsigned char softstart[] = { 0x0c, 0xd7, 0xd6, 0x9d };
unsigned char softstart[] = { 0x0c, 0xCF, 0xCE, 0x8D };;
unsigned char Rambypass[] = { 0x21, 0x8f };		// Display update
unsigned char MAsequency[] = { 0x22, 0xf0 };		// clock
unsigned char GDVol[] = { 0x03, 0x00 };	// Gate voltage +15V/-15V
unsigned char SDVol[] = { 0x04, 0x0a };	// Source voltage +15V/-15V
unsigned char VCOMVol[] = { 0x2c, 0xa8 };	// VCOM 7c
unsigned char BOOSTERFB[] = { 0xf0, 0x1f };	// Source voltage +15V/-15V
unsigned char DummyLine[] = { 0x3a, 0x1a };	// 4 dummy line per gate
unsigned char Gatetime[] = { 0x3b, 0x08 };	// 2us per line
unsigned char BorderWavefrom[] = { 0x3c, 0x33 };	// Border
unsigned char RamDataEntryMode1[] = { 0x11, 0x01 };	// Ram data entry mode1
unsigned char RamDataEntryMode2[] = { 0x11, 0x02 };	// Ram data entry mode1
unsigned char RamDataEntryMode3[] = { 0x11, 0x03 };	// Ram data entry mode3
unsigned char RamDataEntryMode0[] = { 0x11, 0x00 };	// Ram data entry mode0

/*WAVEFORM LOOK UP TABLE*/
const unsigned char LUTDefault_part[31] =
{
		0x32,	// command
			0x00,0x00,0x00,0x18,
		 0x18,0x18,0x18,0x00,
		 0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,
		 0x40,0x14,0x34,0x01,
		 0x00,0x00,0x00,0x00,
		 0x00,0x00,

		/*0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00*/

};

const unsigned char LUTDefault_full[31] =
{
		0x32,	// command
		/*	0x11,0x11,0x10,0x02,	// full
		 0x02,0x22,0x22,0x22,
		 0x22,0x22,0x51,0x51,
		 0x55,0x88,0x08,0x08,
		 0x88,0x88,0x00,0x00,
		 0x34,0x23,0x12,0x21,
		 0x24,0x28,0x22,0x21,
		 0xA1,0x01
		 */
		0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69, 0x69, 0x59,
		0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
		0x35, 0x51, 0x51, 0x19, 0x01, 0x00 };

//-------------------------------------------------------
//Func	: EPD_W21_UpdataDisplay
//Desc	: updata the display 
//Input	:
//Output: 
//Return: 
//Author: 
//Date	: 2012/3/13
//-------------------------------------------------------
void SPI_Delay(unsigned char xrate)
{
	unsigned char i;
	while(xrate)
	{
		for(i=0;i<EPD_W21_SPI_SPEED;i++);
		xrate--;
	}
}

void SPI_Write(unsigned char value)
{
	unsigned char i;

	SPI_Delay(1);
	for (i = 0; i < 8; i++)
	{
		EPD_SPI_CLK_Low();
		SPI_Delay(1);
		if (value & 0x80)
			EPD_SPI_MOSI_High();
		else
			EPD_SPI_MOSI_Low();
		value = (value << 1);
		SPI_Delay(1);
		EPD_SPI_CLK_High();
		SPI_Delay(1);
	}
}

void EPD_W21_WriteCMD(unsigned char command) {
	SPI_Delay(1);
	EPD_SPI_CS_Low();
	EPD_SPI_DC_Low();		// command write
	EPD_SPI_Xfer(command);
	EPD_SPI_CS_High();
}
void EPD_W21_WriteDATA(unsigned char command) {
	SPI_Delay(1);
	EPD_SPI_CS_Low();
	EPD_SPI_DC_High();		// Data write
	EPD_SPI_Xfer(command);
	EPD_SPI_CS_High();
}

void EPD_W21_WriteCMD_p1(unsigned char command, unsigned char para) {
	while (EPD_SPI_Busy_read() == 1)// wait
		;
	EPD_SPI_CS_Low();
	EPD_SPI_DC_Low();			// command write
	EPD_SPI_Xfer(command);
	EPD_SPI_DC_High();		    // data write
	EPD_SPI_Xfer(para);
	EPD_SPI_CS_High();
}

void EPD_W21_WriteCMD_p2(unsigned char command, unsigned char para1,
		unsigned char para2) {
	while (EPD_SPI_Busy_read() == 1) // wait
		;
	EPD_SPI_CS_Low();
	EPD_SPI_DC_Low();			// command write
	EPD_SPI_Xfer(command);
	EPD_SPI_DC_High();		    // data write
	EPD_SPI_Xfer(para1);
	EPD_SPI_Xfer(para2);
	EPD_SPI_CS_High();
}
void EPD_W21_Write(unsigned char *value, unsigned char datalen) {
	unsigned char i = 0;
	unsigned char *ptemp;

	ptemp = value;

	EPD_SPI_CS_Low();
	EPD_SPI_DC_Low();		// command write
	EPD_SPI_Xfer(*ptemp);
	ptemp++;
	EPD_SPI_DC_High();		// data write

	for (i = 0; i < datalen - 1; i++)	// sub the command
			{
		EPD_SPI_Xfer(*ptemp);
		ptemp++;
	}

	EPD_SPI_CS_High();
}
void EPD_W21_WriteDispRam(unsigned char XSize, unsigned int YSize,
		unsigned char *Dispbuff) {
	int i = 0, j = 0;

	if (XSize % 8 != 0) {
		XSize = XSize + (8 - XSize % 8);
	}
	XSize = XSize / 8;
	while (EPD_SPI_Busy_read() == 1)
		;	// wait

	EPD_SPI_CS_Low();
	EPD_SPI_DC_Low();		// command write
	EPD_SPI_Xfer(0x24);

	EPD_SPI_DC_High();		// data write

	for (i = 0; i < YSize; i++) {
		for (j = 0; j < XSize; j++) {
			EPD_SPI_Xfer(*Dispbuff);
			Dispbuff++;
		}
	}
	EPD_SPI_CS_High();
}

void EPD_W21_WriteDispRamMono(unsigned char XSize, unsigned int YSize,
		unsigned char dispdata) {
	int i = 0, j = 0;

	if (XSize % 8 != 0) {
		XSize = XSize + (8 - XSize % 8);
	}
	XSize = XSize / 8;
	while (EPD_SPI_Busy_read() == 1)
		;	// wait


	EPD_SPI_CS_Low();
	EPD_SPI_DC_Low();		// command write
	EPD_SPI_Xfer(0x24);

	EPD_SPI_DC_High();		// data write
	for (i = 0; i < YSize; i++) {
		for (j = 0; j < XSize; j++) {
			EPD_SPI_Xfer(dispdata);
		}
	}

	EPD_SPI_CS_High();
}

void EPD_W21_POWERON(void) {
	EPD_W21_POWERPIN_HIGH();
	EPD_W21_WriteCMD_p1(0x22, 0xc0);
	EPD_W21_WriteCMD(0x20);
	EPD_W21_WriteCMD(0xff);
}
void EPD_W21_POWEROFF(void) {
	EPD_W21_WriteCMD_p1(0x22, 0x03);
	EPD_W21_WriteCMD(0x20);
	EPD_W21_WriteCMD(0xff);
	EPD_W21_POWERPIN_LOW();
}

void EPD_W21_POWERPIN_HIGH(void){
	EPD_POWERON_High();
}
void EPD_W21_POWERPIN_LOW(void){
	halCommonDelayMilliseconds(200);
	EPD_POWERON_Low();
}

void part_display(unsigned char RAM_XST, unsigned char RAM_XEND,
		unsigned char RAM_YST, unsigned char RAM_YST1, unsigned char RAM_YEND,
		unsigned char RAM_YEND1) {
	EPD_W21_SetRamArea(RAM_XST, RAM_XEND, RAM_YST, RAM_YST1, RAM_YEND,
			RAM_YEND1); /*set w h*/
	//EPD_W21_SetRamPointer(RAM_XST, RAM_YST, RAM_YST1); /*set orginal*/
	EPD_W21_SetRamPointer_bc(RAM_XST, (RAM_YST<<8 | RAM_YST1)); /*set orginal*/

}
void EPD_W21_UpdataDisplay(unsigned char *imgbuff, unsigned char xram,
		unsigned int yram) {
	EPD_W21_WriteDispRam(xram, yram, imgbuff);
//	EPD_W21_Update();
}
void EPD_W21_SetRamArea(unsigned char Xstart, unsigned char Xend,
		unsigned char Ystart, unsigned char Ystart1, unsigned char Yend,
		unsigned char Yend1) {
	unsigned char RamAreaX[3];	// X start and end
	unsigned char RamAreaY[5]; 	// Y start and end
	RamAreaX[0] = 0x44;	// command
	RamAreaX[1] = Xstart;
	RamAreaX[2] = Xend;
	RamAreaY[0] = 0x45;	// command
	RamAreaY[1] = Ystart;
	RamAreaY[2] = Ystart1;
	RamAreaY[3] = Yend;
	RamAreaY[4] = Yend1;
	EPD_W21_Write(RamAreaX, sizeof(RamAreaX));
	EPD_W21_Write(RamAreaY, sizeof(RamAreaY));
}
void EPD_W21_SetRamPointer(unsigned char addrX, unsigned char addrY,
		unsigned char addrY1) {
	unsigned char RamPointerX[2];	// default (0,0)
	unsigned char RamPointerY[3];
	RamPointerX[0] = 0x4e; // TODO: cannb, FIXME ?
	RamPointerX[1] = addrX;
	RamPointerY[0] = 0x4f;
	RamPointerY[1] = addrY;
	RamPointerY[2] = addrY1;

	EPD_W21_Write(RamPointerX, sizeof(RamPointerX));
	EPD_W21_Write(RamPointerY, sizeof(RamPointerY));
}

/*TODO: cannb, adress y to int*/
void EPD_W21_SetRamArea_bc(unsigned char Xstart, unsigned char Xend,
		unsigned int Ystart, unsigned int Yend)
{
	unsigned char RamAreaX[3];	// X start and end
	unsigned char RamAreaY[5]; 	// Y start and end
	RamAreaX[0] = 0x44;	// command
	RamAreaX[1] = Xstart/8;
	RamAreaX[2] = Xend/8;
	RamAreaY[0] = 0x45;	// command
	RamAreaY[1] = Ystart & 0xFF;
	RamAreaY[2] =  Ystart >> 8;
	RamAreaY[3] = Yend & 0xFF;
	RamAreaY[4] = Yend >> 8 ;
	EPD_W21_Write(RamAreaX, sizeof(RamAreaX));
	EPD_W21_Write(RamAreaY, sizeof(RamAreaY));
}
void EPD_W21_SetRamPointer_bc(unsigned int addrX, unsigned int addrY)
{
	unsigned char RamPointerX[2];	// default (0,0)
	unsigned char RamPointerY[3];
	RamPointerX[0] = 0x4e; // TODO: cannb ?
	RamPointerX[1] = addrX & 0xFF;
	RamPointerY[0] = 0x4f;
	RamPointerY[1] = addrY & 0xFF; // byte low
	RamPointerY[2] = (addrY >> 8); // byte high

	EPD_W21_Write(RamPointerX, sizeof(RamPointerX));
	EPD_W21_Write(RamPointerY, sizeof(RamPointerY));
}

//=========================functions============================

//-------------------------------------------------------
//Func	: void EPD_W21_DispInit(void)
//Desc	: display parameters initinal
//Input	: none
//Output: none
//Return: none
//Author: 
//Date	: 2011/12/24
//-------------------------------------------------------
void EPD_W21_DispInit(void) {
	/* Pannel configuration, Gate selection*/
	EPD_W21_Write(GDOControl, sizeof(GDOControl));
	EPD_W21_Write(softstart, sizeof(softstart));
	EPD_W21_Write(VCOMVol, sizeof(VCOMVol));		// VCOM setting
	EPD_W21_Write(DummyLine, sizeof(DummyLine));	// dummy line per gate
	EPD_W21_Write(Gatetime, sizeof(Gatetime));		// Gage time setting
	EPD_W21_Write(RamDataEntryMode3, sizeof(RamDataEntryMode3));// X increase, Y decrease
	EPD_W21_SetRamArea_bc(0,127,0, 295);
}

//-------------------------------------------------------
//Func	: EPD_W21_EnableChargepump
//Desc	: 
//Input	:
//Output: 
//Return: 
//Author: 
//Date	: 2011/12/24
//-------------------------------------------------------
void EPD_W21_EnableChargepump(void) {
	EPD_W21_WriteCMD_p1(0xf0, 0x8f);
	EPD_W21_WriteCMD_p1(0x22, 0xc0);
	EPD_W21_WriteCMD(0x20);
	EPD_W21_WriteCMD(0xff);
}

//-------------------------------------------------------
//Func	: EPD_W21_DisableChargepump
//Desc	: 
//Input	:
//Output: 
//Return: 
//Author: 
//Date	: 2011/12/24
//-------------------------------------------------------
void EPD_W21_DisableChargepump(void) {
	EPD_W21_WriteCMD_p1(0x22, 0xf0);
	EPD_W21_WriteCMD(0x20);
	EPD_W21_WriteCMD(0xff);
}
//-------------------------------------------------------
//Func	: EPD_W21_Update
//Desc	: 
//Input	:
//Output: 
//Return: 
//Author: 
//Date	: 2011/12/24
//-------------------------------------------------------
void EPD_W21_Update(unsigned char cmdUpdate) {
	EPD_W21_WriteCMD_p1(0x22, cmdUpdate);
	EPD_W21_WriteCMD(0x20);
	EPD_W21_WriteCMD(0xff);
}


void EPD_W21_WriteRAM(void) {
	EPD_W21_WriteCMD(0x24);
}

//-------------------------------------------------------
//Func	: EPD_W21_WriteLUT(unsigned char *LUTvalue)
//Desc	: write the waveform to the dirver's ram 
//Input	: *LUTvalue, the wavefrom tabe address
//Output: none
//Return: none
//Author: 
//Date	: 2011/12/24
//-------------------------------------------------------
void EPD_W21_WriteLUT(unsigned char *LUTvalue) {
	EPD_W21_Write(LUTvalue, 31);
}

/***********************************************************
 end file
 ***********************************************************/

/*
 * GDE029A1_sFunc.c
 *
 *  Created on: Aug 23, 2016
 *      Author:
 */

//unsigned int IC_SWITCH;
//unsigned char vcom;
void lcd_chkstatus(void);
void lut_red(void);
void lut_bw(void);
void Ultrachip_red(void);
void Ultrachip(void);
void master_uc8154_init(void);
void Ultrachip_red1(void);
void Ultrachip1(void);
void pic_display(void);
unsigned char EPD_W21_ReadDATA(void);

void EPD_init(int selectLut) {
	/* 4 wire spi mode selected by hardware */
	EPD_SPI_Reset();
	EPD_SPI_MOSI_Low();
	EPD_SPI_RESET_Low();
	EPD_W21_DispInit(); /* Pannel configure*/

//	if(selectLut)
//	{
	EPD_W21_WriteLUT((unsigned char *) LUTDefault_full);
//	}
//	else
//	{
//		EPD_W21_WriteLUT((unsigned char *) LUTDefault_part);
//	}

	EPD_W21_POWERON();
}

//============================================
// TODO: cannb , port font

void EPD_clean(void){
	EPD_W21_SetRamArea_bc(0, EPD_HEIGHT, 0, EPD_WIDTH);
	EPD_W21_SetRamPointer_bc(0,0);	// set ram
	EPD_W21_WriteDispRamMono(EPD_HEIGHT+1, EPD_WIDTH +1, 0xFF);	// white
	EPD_W21_Update(0xC7);
}


void EPD_update_image( unsigned char *data){
	EPD_W21_SetRamArea_bc(0, EPD_HEIGHT, 0, EPD_WIDTH);
	EPD_W21_SetRamPointer_bc(0x00,0);	// set ram
	EPD_W21_WriteDispRam(EPD_HEIGHT+1, EPD_WIDTH +1, (unsigned char *) data);
	EPD_W21_Update(0xC7);
}



/**
 * @brief  Writes 4 dots.
 * @param  HEX_Code: specifies the Data to write.
 * @retval None
 */
void gde029a1_WritePixel(uint8_t HEX_Code)
{
	/* Prepare the register to write data on the RAM */
	EPD_W21_WriteCMD(36);

	/* Send the data to write */
	EPD_W21_WriteDATA(HEX_Code);
}

/**
 * @brief  Sets a display window.
 * @param  Xpos: specifies the X bottom left position.
 * @param  Ypos: specifies the Y bottom left position.
 * @param  Width: display window width.
 * @param  Height: display window height.
 * @retval None
 */
void gde029a1_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width,
		uint16_t Height) {
	//emberAfGuaranteedPrint("+++ gde029a1_SetDisplayWindow Xpos %d, Ypos %d, Width %d, Height %d\n", Xpos, Ypos, Width, Height);
	/*
	 * +++ gde029a1_SetDisplayWindow Xpos 30, Ypos 65535, Width 36, Height 1
+++ gde029a1_SetDisplayWindow Xpos 23, Ypos 65535, Width 29, Height 1
+++ gde029a1_SetDisplayWindow Xpos 16, Ypos 65535, Width 22, Height 1
+++ gde029a1_SetDisplayWindow Xpos 9, Ypos 65535, Width 15, Height 1
+++ gde029a1_SetDisplayWindow Xpos 2, Ypos 65535, Width 8, Height 1
+++ gde029a1_SetDisplayWindow Xpos 65531, Ypos 65535, Width 1, Height 1
	 * */
	/* Set Y position and the height */
	EPD_W21_WriteCMD(0x45);
	EPD_W21_WriteDATA(Xpos& 0xFF);
	EPD_W21_WriteDATA(Xpos>>8);
	EPD_W21_WriteDATA(Width& 0xFF);
	EPD_W21_WriteDATA(Width>>8);
	/* Set X position and the width */
	EPD_W21_WriteCMD(0x44);
	EPD_W21_WriteDATA(Ypos);
	EPD_W21_WriteDATA(Height);
	/* Set the height counter */
	EPD_W21_WriteCMD(0x4E);
	EPD_W21_WriteDATA(Ypos);
	/* Set the width counter */
	EPD_W21_WriteCMD(0x4F);
	EPD_W21_WriteDATA(Xpos& 0xFF);
	EPD_W21_WriteDATA(Xpos>>8);

}

uint32_t GetXSize(void) {
	return EPD_WIDTH ;
}

uint32_t GetYSize(void) {
	return EPD_HEIGHT ;
}

void SetFont(sFONT *pFonts) {
	pFont = pFonts;
}

sFONT *GetFont(void) {
	return pFont;
}

void Clear(uint16_t Color) {
	uint32_t index = 0;

	gde029a1_SetDisplayWindow(0, 0, GetXSize(), GetYSize());

	for (index = 0; index < 3096; index++) {
		gde029a1_WritePixel(Color);
	}
}

/**
 * @brief  Draws a character on EPD.
 * @param  Xpos: specifies the X position, can be a value from 0 to 171
 * @param  Ypos: specifies the Y position, can be a value from 0 to 17
 * @param  c: pointer to the character data
 * @retval None
 */

uint8_t REV(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}
/*char REV(char x) {
//	return (((x >> 4) & 0x0F) | ((x << 4) & 0xF0));
	return x;
}*/

void DrawChar(uint16_t Xpos, uint16_t Ypos, const uint8_t *c)
{
	int8_t index = 0,count = 0;
	//uint32_t data_length = 0;
	uint16_t height = 0;
	uint16_t width = 0;


	width = pFont->Width;
	height = pFont->Height;

	/* Set the Character display window */
	gde029a1_SetDisplayWindow( 295-(Xpos + width - 1),
			15-(Ypos + height - 1),295-Xpos, 15-Ypos);

	//data_length = (height * width);

	for (index = width-1; index >-1; index--)
	{
		for (count = height-1; count >-1 ; count--)
		{

			//emberAfGuaranteedPrint("+++ DrawChar %d %x %x\n", index * height + count, REV(c[index * height + count]),c[index * height + count]);
			gde029a1_WritePixel(REV(c[index * height + count]));

		}
	}

}

void DisplayChar(uint16_t Xpos, uint16_t Ypos, uint8_t Ascii)
{
	Ascii -= 32;

	DrawChar(Xpos, Ypos,
			&pFont->table[Ascii * ((pFont->Height) * (pFont->Width))]);
}

void DisplayStringAt(uint16_t Xpos, uint16_t Ypos, char *Text,
		Text_AlignModeTypdef Mode)
{
	uint16_t refcolumn = 1, i = 0;
	uint32_t size = 0, xsize = 0;
	uint8_t *ptr = Text;

	/* Get the text size */
	while (*ptr++)
		size++;

	/* Characters number per line */
	xsize = (GetXSize() / pFont->Width);

	switch (Mode)
	{
	case TEXT_CENTER_MODE:
	{
		refcolumn = Xpos + ((xsize - size) * pFont->Width) / 2;
		break;
	}
	case TEXT_LEFT_MODE:
	{
		refcolumn = Xpos;
		break;
	}
	case TEXT_RIGHT_MODE:
	{
		refcolumn = -Xpos + ((xsize - size) * pFont->Width);
		break;
	}
	default:
	{
		refcolumn = Xpos;
		break;
	}
	}

	/* Send the string character by character on EPD */
	while ((*Text != 0)
			& (((GetXSize() - (i * pFont->Width)) & 0xFFFF) >= pFont->Width))
	{
		/* Display one character on EPD */
		DisplayChar(refcolumn, Ypos, *Text);
		/* Decrement the column position by 16 */
		refcolumn += pFont->Width;
		/* Point on the next character */
		Text++;
		i++;
	}
}
void EPD_PrintDefaultInfo(){

	DisplayStringAt(1,13, "                                          ", TEXT_LEFT_MODE); // 37 char per line
	DisplayStringAt(1,10, "                                          ", TEXT_LEFT_MODE); // 37 char per line
	DisplayStringAt(1,7, "                                          ", TEXT_LEFT_MODE); // 37 char per line

	DisplayStringAt(1,13,personInformation.name, TEXT_LEFT_MODE);
	DisplayStringAt(1,10, personInformation.company, TEXT_LEFT_MODE);
	DisplayStringAt(1,7, personInformation.scope, TEXT_LEFT_MODE);

	DisplayStringAt(0,1, "                                          ", TEXT_LEFT_MODE); // 37 char per line
	DisplayStringAt(0,1, (char*)personInformation.msg, TEXT_LEFT_MODE);
}
void EPD_UpdateDisplay(){

		EPD_W21_Update(0x04);
}

void EPD_DisplayScanInfo(uint8_t index, char * info){

		DisplayStringAt(1,13-index*3, "                                          ", TEXT_LEFT_MODE); // 37 char per line

		DisplayStringAt(1,13-index*3,info, TEXT_LEFT_MODE);
}
