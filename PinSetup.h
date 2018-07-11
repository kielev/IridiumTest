/*
 * PinSetup.h
 *
 *  Created on: Jun 25, 2018
 *      Author: kielev
 */

#include "driverlib.h"

/* Standard Includes */
#include <stdio.h>
#include <string.h>


#ifndef PINSETUP_H_
#define PINSETUP_H_


void IOSetup(void);

void initGPSUART(void);

void disableGPSUART(void);

void initIridiumUART(void);

void disableIridiumUART(void);

void Iridium_puts(char *outString);

void GPS_puts(char *outString);



#endif /* PINSETUP_H_ */
