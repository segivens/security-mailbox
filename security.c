/************************************************************
Steven Givens
CDA4630 Intro to Embedded Systems
Bassem Alhalabi
Anti-Theft Mailbox - Security Control
*************************************************************/

#include <msp430.h> 
#include <stdio.h>
#include <inttypes.h>
#include "security.h"

int ADCReading[4];

int xValue = 0;         //x-axis of accelerometer
int IR_package = 0;     //IR value for package
int signal = 0;         //signal from MSP1

int dud = 0;    //dummy variable for ADC

int alert = 0;  //0 if no emergency, 1 if emergency
int flag = 0;   //flag for sending fresh notification

//***********************************************************************************************************
//MISC FUNCTIONS
//***********************************************************************************************************
//Delay function for producing delay in 0.1 ms increments
void delay(uint8_t t)
{
    uint8_t i;
    for(i=t; i > 0; i--) { __delay_cycles(100); }
}

//sets pulse width modulation
void setPWM(int valuePWM)
{
    P1SEL |= BZZ;               //select buzzer pin as our PWM output.
    TA0CCR0 = 1000;             //set the period in the Timer A0 Capture/Compare 0 register to 1000 us.
    TA0CCTL1 = OUTMOD_7;        //CCR1 reset/set
    TA0CCR1 = valuePWM;         //the period in microseconds that the power is ON.
    TA0CTL = TASSEL_2 + MC_1;   //TASSEL_2 selects SMCLK as the clock source, and MC_1 tells it to count up to the value in TA0CCR0.
}

//***********************************************************************************************************
//ANALOG
//***********************************************************************************************************
void ConfigureAdc(void)
{
   ADC10CTL1 = INCH_3 | CONSEQ_1;                //A3 + A2 + A1 + A0, single sequence
   ADC10CTL0 = ADC10SHT_2 | MSC | ADC10ON;
   while (ADC10CTL1 & BUSY);
   ADC10DTC1 = 0x04;                             //4 conversions
   ADC10AE0 |= (IRP | ACCX | FROM1);             //ADC10 option select
}

void getanalogvalues()
{
    int i = 0;
    IR_package = 0;     //set all analog values to zero
    xValue = 0;
    signal = 0;
    dud = 0;

    for(i=1; i<=5 ; i++)                          // read analog values 5 times each and average
    {
        ADC10CTL0 &= ~ENC;
        while (ADC10CTL1 & BUSY);           //wait while ADC is busy
        ADC10SA = (unsigned)&ADCReading[0]; //RAM Address of ADC Data, must be reset every conversion
        ADC10CTL0 |= (ENC | ADC10SC);       //start ADC Conversion
        while (ADC10CTL1 & BUSY);           //wait while ADC is busy

        IR_package += ADCReading[0];     //sum all 5 readings into one variable each
        dud += ADCReading[1];
        xValue += ADCReading[2];
        signal += ADCReading[3];
    }

    IR_package /= 5;    //average the 5 readings for the three variables
    xValue /= 5;
    signal /= 5;
}

//***********************************************************************************************************
//BLUETOOTH
//***********************************************************************************************************
//sets up Bluetooth
void ConfigureBluetooth()
{
    if (CALBC1_1MHZ==0xFF)   //If calibration constant erased
    {
        while(1);          //do not load, trap CPU
    }

     DCOCTL  = 0;             //Select lowest DCOx and MODx settings
     BCSCTL1 = CALBC1_1MHZ;   //Set range
     DCOCTL  = CALDCO_1MHZ;   //Set DCO step + modulation

     P1SEL  |=  BIT2;         //P1.1 UCA0RXD input
     P1SEL2 |=  BIT2;         //P1.2 UCA0TXD output

     UCA0CTL1 |=  UCSSEL_2 + UCSWRST;  //USCI Clock = SMCLK,USCI_A0 disabled
     UCA0BR0   =  104;                 //104 From datasheet table-
     UCA0BR1   =  0;                   //-selects baudrate =9600,clk = SMCLK
     UCA0MCTL  =  UCBRS_1;             //Modulation value = 1 from datasheet
     UCA0STAT |=  UCLISTEN;            //loop back mode enabled
     UCA0CTL1 &= ~UCSWRST;             //Clear UCSWRST to enable USCI_A0
}

//prints a string to terminal
void printBluetooth(char *str)
{
    while(*str != 0)    //loop until end of string
    {
        while (!(IFG2&UCA0TXIFG));
            UCA0TXBUF = *str++;     //prints each letter of string to terminal
    }
}

//***********************************************************************************************************
//ACCEL + INFRARED
//***********************************************************************************************************
//checks if package is remove/mailbox is moving
void check_disturbance()
{
    if (IR_package > 650 && IR_package < 850) {}    //deadzone
    else
    {
        if(IR_package <= 650)   //package removed
        {
            alert = 1;
            return;

        }
        if(IR_package >= 850)   //package still in mailbox
        {
            alert = 0;
        }
    }

    if (xValue > 500 && xValue < 550) {}    //deadzone
    else
    {
        if(xValue >= 550)   //mailbox moved
        {
            alert = 1;
            return;
        }
        if(xValue <= 500)   //mailbox not moving
        {
            alert = 0;
        }
    }

    return;
}

//***********************************************************************************************************
//***********************************************************************************************************
//***********************************************************************************************************
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	//stop watchdog timer
	
    P1OUT = 0;              //set default values to all zeros for port 1
    P1DIR = 0;              //set port direction to all inputs for port 1
    P2OUT = 0;              //set default values to all zeros for port 2
    P2DIR = 0;              //set port direction to all inputs for port 2

    P1REN = 0;              //disable pull up/down resistors for port 1
    P2REN = 0;              //disable pull up/down resistors for port 2

    P1DIR |= (BZZ | YEL | RED);     //set outputs
    P2DIR |= (TO1);                 //signal to MSP1

    ConfigureAdc();         //set up ADC
    delay(3);

    ConfigureBluetooth();   //set up bluetooth
    delay(3);

    P2OUT |= TO1;   //signal default on

    //for testing analog
    /*while(1)
    {
        getanalogvalues();
        delay(5);
    }*/

for(;;)
{
    getanalogvalues();  //check if armed

    if (signal > 600)   //if not armed
    {
        while(signal > 600)    //loop exits when armed
        {
            //P1OUT |= GRE;   //green LED on: disarmed
            P1OUT &= ~YEL;
            P1OUT &= ~RED;
            delay(3);

            P1SEL &= ~BZZ; //turn off buzzer

            getanalogvalues(); //check if armed
        }

        printBluetooth("Package arrived, security armed\n");
    }

    getanalogvalues();
    delay(5);
    check_disturbance();    //check if package stolen/mailbox moved

    if(alert == 0) //no emergency
    {
        P2OUT |= TO1; //resume MSP1

        //P1OUT &= ~GRE;
        P1OUT |= YEL;   //yellow LED on, armed
        P1OUT &= ~RED;
        delay(3);

        P1SEL &= ~BZZ;  //buzzer off

        if(flag == 1)   //if flag set, set it back to 0
        {
            flag = 0;
        }
    }
    else if (alert == 1) //emergency
    {
        P2OUT &= ~TO1;    //pause MSP1

        //P1OUT &= ~GRE;
        P1OUT &= ~YEL;
        P1OUT |= RED;   //red LED on, emergency
        delay(3);

        setPWM(500);    //buzzer on

        if (flag == 0)
        {
            __delay_cycles(1000000);

            //send notification
            printBluetooth("Disturbance at mailbox\n");

            flag = 1;
        }
    }
} //end for
} //end main
