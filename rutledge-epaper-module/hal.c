/*
 * */

#include "hal.h"

void driver_delay_us(unsigned int xus)
{
    for(;xus>1;xus--);
}

void driver_delay_xms(unsigned long xms)
{
    unsigned long i = 0 , j=0;

    for(j=0;j<xms;j++)
    {
        for(i=0; i<256; i++);
    }
}
