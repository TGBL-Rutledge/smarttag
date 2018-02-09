#ifndef __RUTLEDGE_COMMON_H__
#define __RUTLEDGE_COMMON_H__


#include <stdint.h>
#include "rutledge-common/common.h"
#ifndef   MIN
  #define MIN(a, b)         (((a) < (b)) ? (a) : (b))
#endif

#ifndef   MAX
  #define MAX(a, b)         (((a) > (b)) ? (a) : (b))
#endif

typedef struct {
  uint8_t   longId[8];
  int8_t    rssi;
} RutledgeScanData;
#define MAX_SCANNING_DEVICES 32
#define MAX_TOP_DEVICES 3
#define COMPARE_DEVIATION_PERCENT 20

//compile option for building a smart tag device or smart relay
#define BADGE_DEVICE true        //true  = smart badge
                                 //false = smart relay

//define use e-paper hardware module
#define MODULE_EPD true
//enable Serial Wire Output pin
#define SWO_ENABLED true

extern RutledgeScanData top3Data[MAX_TOP_DEVICES];
extern RutledgeScanData scanData[MAX_SCANNING_DEVICES];
extern uint8_t scanCounter;

//vlad
void initDataBeforeScan();
extern void sendAir(uint8_t status);


#endif
