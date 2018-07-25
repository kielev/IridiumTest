/* DriverLib Includes */
#include "driverlib.h"
#include "msp.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Required Includes */
#include "PinSetup.h"

volatile _Bool StringClassifyGo = 0;

//UART ISR Globals
volatile uint8_t RXData = 0; //These are where the characters obtained on the UART buffer for each channel are stored.
volatile int index = 0;

volatile int step = 1;

char IridiumString[100];
char ParameterString[100];

//How a second delay is created without having to use ACLK or another source besides what I know. Delay1ms is included in this
void parrotdelay(unsigned long ulCount)
{
    __asm ( "pdloop:  subs    r0, #1\n"
            "    bne    pdloop\n");
}

void Delay1ms(uint32_t n) //just burns CPU as it decrements this counter. It was scaled to 3MHz from what we had in another project.
{
    while (n)
    {
        parrotdelay(369);    // 1 msec, tuned at 48 MHz is 5901, 3MHz set to 369
        n--;
    }
}

void initClocks(void)
{
    //Halting WDT
    MAP_WDT_A_holdTimer();

    CS_setExternalClockSourceFrequency(32768, 48000000);
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);
    CS_startHFXT(false);
    //Starting HFXT
    MAP_CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_16);
    //Starting LFXT
    MAP_CS_startLFXT(CS_LFXT_DRIVE3);
    MAP_CS_initClockSignal(CS_BCLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // Configuring WDT to timeout after 128M iterations of SMCLK, at 3MHz,
    // this will roughly equal 42 seconds
    MAP_SysCtl_setWDTTimeoutResetType(SYSCTL_SOFT_RESET);
    MAP_WDT_A_initWatchdogTimer(WDT_A_CLOCKSOURCE_SMCLK,
    WDT_A_CLOCKITERATIONS_128M);
}


int queryLock(){
    printf("\nCommand: AT+CULK\n");
    Iridium_puts("AT+CULK?\r");
    while(StringClassifyGo == 0);
    if(!strncmp("AT",IridiumString,2))
        StringClassifyGo = 0;
    else{
        printf("AT echo error %s\n", IridiumString);
        StringClassifyGo = 0;
    }
    while(StringClassifyGo == 0);
    printf("CULK? - %s\n", IridiumString);
}

int sendIridiumString(char * String){
    char SBDIX[30] = {'\0'};
    char *tokString;
    char IMessage[340] = {'\0'};
    int ret = 0;

    printf("\nCommand: AT\n");
    Iridium_puts("AT\r");
    while(StringClassifyGo == 0);
    if(!strncmp("AT",IridiumString,2))
        StringClassifyGo = 0;
    else{
        printf("AT echo error %s\n", IridiumString);
        StringClassifyGo = 0;
    }

    while(StringClassifyGo == 0);
    if(!strncmp("OK",IridiumString,2)){
        StringClassifyGo = 0;
    } else {
        StringClassifyGo = 0;
        printf("Error AT %s\n", IridiumString);
        return 0;
    }

    printf("\nCommand: AT&K0\n");
    Iridium_puts("AT&K0\r");
    while(StringClassifyGo == 0);
    if(!strncmp("AT&K0",IridiumString,5))
        StringClassifyGo = 0;
    else{
        printf("AT&K0 echo error %s\n", IridiumString);
        StringClassifyGo = 0;
    }

    while(StringClassifyGo == 0);
    if(!strncmp("OK",IridiumString,2)){
        StringClassifyGo = 0;
    } else {
        StringClassifyGo = 0;
        printf("Error AT&K0 %s\n", IridiumString);
        return 0;
    }


    printf("\nCommand: AT+SBDWT\n");
    Iridium_puts("AT+SBDWT\r");
    while(StringClassifyGo == 0);
    if(!strncmp("AT+SBDWT",IridiumString,8))
        StringClassifyGo = 0;
    else{
        printf("AT+SBDWT echo error %s\n", IridiumString);
        StringClassifyGo = 0;
    }

    while(StringClassifyGo == 0);
    if(!strncmp("READY",IridiumString,5)){
        StringClassifyGo = 0;
    } else {
        StringClassifyGo = 0;
        printf("Error AT+SBDWT %s\n", IridiumString);
        return 0;
    }

    printf("\nCommand: message\n");
    strcpy(IMessage, "$1234567891011072118,124531,0841.8477,N,06652.1948,W,6.33\n"
                     "072118,124500,3286.3906,N,06550.5332,W,7.33\r");
    Iridium_puts(IMessage);
    while(StringClassifyGo == 0);
    if(!strncmp("OK",IridiumString,2) || !strncmp("0",IridiumString,1)){
        StringClassifyGo = 0;
    } else {
        StringClassifyGo = 0;
        printf("Error message %s\n", IridiumString);
        return 0;
    }


    printf("\nCommand: AT+SBDIX\n");
    Iridium_puts("AT+SBDIX\r");
    while(StringClassifyGo == 0);
    if(!strncmp("AT",IridiumString,2) || !strncmp("\rAT",IridiumString,3))
        StringClassifyGo = 0;
    else{
        printf("AT+SBDIX echo error %s\n", IridiumString);
        StringClassifyGo = 0;
    }

    while(StringClassifyGo == 0);
    if(!strncmp("+SBDIX",IridiumString,6)){
        strcpy(SBDIX, IridiumString);
        StringClassifyGo = 0;
    } else {
        printf("Error +SBDIX %s\n", IridiumString);
        StringClassifyGo = 0;
    }

    while(StringClassifyGo == 0);
    if(!strncmp("\r",IridiumString,1)){
        StringClassifyGo = 0;
    } else {
        StringClassifyGo = 0;
        printf("Error return %s\n", IridiumString);
    }

    while(StringClassifyGo == 0);
    if(!strncmp("OK",IridiumString,2)){
        StringClassifyGo = 0;
    } else {
        StringClassifyGo = 0;
        printf("Error message %s\n", IridiumString);
    }

    printf("\nCommand: AT+SBDD0\n");
    Iridium_puts("AT+SBDD0\r");
    while(StringClassifyGo == 0);
    if(!strncmp("AT",IridiumString,2))
        StringClassifyGo = 0;
    else{
        printf("AT+SBDD0 echo error %s\n", IridiumString);
        StringClassifyGo = 0;
    }

    while(StringClassifyGo == 0);
    if(!strncmp("0",IridiumString,1)){
        StringClassifyGo = 0;
    } else {
        printf("Error AT+SBDD0 %s\n", IridiumString);
        StringClassifyGo = 0;
    }

    printf("message: %s --> ", SBDIX);
    tokString = strtok(SBDIX, ",");
    if(atoi(&tokString[8]) > 2 || strlen(SBDIX) < 10){
        printf("send failure - %d\n", atoi(&tokString[8]));
        ret = 0;
    } else{
        printf("send success\n");
        ret = 1;
    }

    strtok(NULL, ",");
    tokString = strtok(NULL, ",");
    if(atoi(tokString) == 1){
        printf("message received\n");

        Iridium_puts("AT+SBDRT\r");
        while(StringClassifyGo == 0);
        if(!strncmp("AT",IridiumString,2))
            StringClassifyGo = 0;
        else{
            printf("AT+SBRT echo error %s\n", IridiumString);
            StringClassifyGo = 0;
        }

        while(StringClassifyGo == 0);
        if(!strncmp("+SBDRT",IridiumString,6))
            StringClassifyGo = 0;
        else{
            printf("AT+SBRT error %s\n", IridiumString);
            StringClassifyGo = 0;
        }

        while(StringClassifyGo == 0);
        if(!strncmp("\r",IridiumString,1)){
            StringClassifyGo = 0;
        } else {
            StringClassifyGo = 0;
            printf("Error return %s\n", IridiumString);
        }

        while(StringClassifyGo == 0);
        if(!strncmp("$",IridiumString,1)){
            strcpy(ParameterString, IridiumString);
            StringClassifyGo = 0;
        } else {
            printf("message error %s\n", IridiumString);
            StringClassifyGo = 0;
        }
        printf("Received Message - %s\n", ParameterString);
        return (ret+2);

    } else {
        printf("no message received - %d\n", atoi(tokString));
        return ret;
    }
}


int main(void){
    int ret = -1;
    //Halting WDT
    MAP_WDT_A_holdTimer();

    //Initializing IO on the MSP, leaving nothing floating
    IOSetup();

    initClocks(); //Initializing all of the timing devices on the MSP

    initIridiumUART();

    //Iridium On
    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);

    MAP_Interrupt_enableMaster();

    //queryLock();

    ret = sendIridiumString("Hello");

    printf("\nCondition: %d\n", ret);

    return ret;
}


/* EUSCI A0 UART ISR - Stores Iridium response data */
void EUSCIA1_IRQHandler(void)
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A1_BASE);

    MAP_UART_clearInterruptFlag(EUSCI_A1_BASE, status);

    if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        RXData = MAP_UART_receiveData(EUSCI_A1_BASE);
        //printf("%c", RXData);
        switch (RXData)
        {
        case '\n': //looks for a new line character which is the end of the AT string
            IridiumString[index] = '\0'; //puts a NULL at the end of the string again because strncpy doesn't
            index = 0;
            StringClassifyGo = 1;
            break;

        default:
            IridiumString[index] = RXData; //puts what was in the buffer into an index in dataString
            index++; //increments the index so the next character received in the buffer can be stored
            //into dataString
            break;
        }
    }
}
