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


int sendIridiumString(char * String){
    char IMessage[100];

    printf("\nCommand: AT\n");
    Iridium_puts("AT\r");
    while(StringClassifyGo == 0);
    if(strncmp("OK",IridiumString,2)){
        StringClassifyGo = 0;
    } else {
        printf("Error AT %s\n", IridiumString);
    }


    printf("\nCommand: AT&K0\n");
    Iridium_puts("AT&K0\r");
    strcpy(IridiumString, "NO");
    while(strncmp("OK",IridiumString,2) != 0);


    printf("\nCommand: AT+SBDWT\n");
    Iridium_puts("AT+SBDWT=hello\r");
    strcpy(IridiumString, "NO");
    while(strncmp("OK",IridiumString,2) != 0 && strncmp("ERROR",IridiumString,5) != 0);

    if(!strncmp("ERROR",IridiumString,5))
        printf("error\n");


    printf("\nCommand: AT+SBDIX\n");
    Iridium_puts("AT+SBDIX\r");
    strcpy(IridiumString, "NO");
    while(strncmp("+SBDIX",IridiumString,6) != 0);
    if(strncmp("+SBDIX: 32",IridiumString,10) != 0){
        printf("message fail: %s\n", IridiumString);
        return 0;
    }
    printf("message succeed: %s\n", IridiumString);

    printf("\nCommand: AT+SBDIX\n");
    Iridium_puts("AT+SBDD0\r");
    strcpy(IridiumString, "NO");
    while(strncmp("OK",IridiumString,2) != 0 && strncmp("ERROR",IridiumString,5) != 0);


    if(!strncmp(&IridiumString[14],"1,",2)){

        printf("\nCommand: AT+SBDRT\n");
        Iridium_puts("AT+SBDRT\r");
        strcpy(IridiumString, "NO");
        while(strncmp("+SBDRT",IridiumString,6) != 0);

        while(strncmp("$",IridiumString,1) != 0 );
        printf("Message: %s\n", IridiumString);
        return 2;
    }
    return 1;
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
