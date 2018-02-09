/*
 * rutledge-common.h
 *
 *  Created on: Aug 18, 2016
 *      Author: cannb
 */

#ifndef _RUTLEDGE_COMMON_COMMON_H_
#define _RUTLEDGE_COMMON_COMMON_H_

/*Include*/
#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_HAL
#include EMBER_AF_API_NETWORK_MANAGEMENT

/*for display EPD*/
#define EPD_STR_NAME	"NAME : KALUTSKIY VLADISLAV"
#define EPD_STR_COMPANY	"COMPANY : RUTLEDGE OMNI SERVICES PTE LTD"
#define EPD_STR_SCOPE	"TITLE : SW DEVELOPER"


//DEBUG

#define DEBUG_ERR(...)     emberAfPrintln(EMBER_AF_PRINT_CORE, "ERR: "## __VA_ARGS__)
#define DEBUG_INFO(...)    emberAfPrintln(EMBER_AF_PRINT_CORE, "INFO: "## __VA_ARGS__)

/** @brief  Structure to hold information of Tag
 * Display it on EPD
 */
typedef struct personInfor
{
	char name[50];
	char company[50];
	char scope[50];
	char msg[40];
}personInfor_t;


extern personInfor_t  personInformation;
#endif /* RUTLEDGE_COMMON_H_ */
