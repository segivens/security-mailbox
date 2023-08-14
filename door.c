/************************************************************
Steven Givens
CDA4630 Intro to Embedded Systems
Bassem Alhalabi
Anti-Theft Mailbox - Door Control
*************************************************************/

#include <msp430.h> 
#include <stdio.h>
#include <inttypes.h>
#include "door.h"

int keypad = 0;     //keypad analog value
int IR_package = 0; //IR value for package
int signal = 0;     //signal from MSP2
int IR_door = 0;    //IR value for door

int ADCReading[4];  //analog values summed here

int flag = 0;       //flag set if package removed

//***********************************************************************************************************
//MISC FUNCTIONS
//***********************************************************************************************************
//Delay function for producing delay in 0.1ms increments
void delay(uint8_t t)
{
    uint8_t i;
    for(i=t; i > 0; i--) { __delay_cycles(100); }
}

//Sets pulse width modulation
void setPWM(int period, int duty)
{
    P1SEL |= SRV;               //select servo pin as the PWM output
    TA0CCR0 = period;           //set period
    TA0CCTL1 = OUTMOD_7;        //CCR1 reset/set
    TA0CCR1 = duty;             //set duty
    TA0CTL = TASSEL_2 + MC_1;   //TASSEL_2 selects SMCLK as the clock source, and MC_1 tells it to count up to the value in TA0CCR0
}

//***********************************************************************************************************
//ANALOG
//***********************************************************************************************************
//Sets up ADC
void ConfigureAdc(void)
{
   ADC10CTL1 = INCH_3 | CONSEQ_1;                //A3 + A2 + A1 + A0, single sequence
   ADC10CTL0 = ADC10SHT_2 | MSC | ADC10ON;
   while (ADC10CTL1 & BUSY);
   ADC10DTC1 = 0x04;                             //4 conversions
   ADC10AE0 |= (IRP | KYPD | FROM2 | IRD);       //ADC10 option select
}

//Retrieves analog values and finds average
void getanalogvalues()
{
    int i = 0;
    IR_door = 0;        //set analog values to zero
    IR_package = 0;
    keypad = 0;
    signal = 0;

    for(i=1; i<=5 ; i++)    //read analog values 5 times each and average
    {
        ADC10CTL0 &= ~ENC;
        while (ADC10CTL1 & BUSY);           //wait while ADC is busy
        ADC10SA = (unsigned)&ADCReading[0]; //RAM Address of ADC Data, must be reset every conversion
        ADC10CTL0 |= (ENC | ADC10SC);       //start ADC Conversion
        while (ADC10CTL1 & BUSY);           //wait while ADC is busy

        IR_door += ADCReading[0];           //sum all 5 readings into one variable each
        IR_package += ADCReading[1];
        keypad += ADCReading[2];
        signal += ADCReading[3];
    }

    IR_door /= 5;       //average the 5 readings for the four variables
    IR_package /= 5;
    keypad /= 5;
    signal /= 5;
}

//***********************************************************************************************************
//LCD SCREEN
//***********************************************************************************************************
//Function to pulse EN pin after data is written
void pulseEN(void)
{
    LCD_OUT |= EN;   delay(1); LCD_OUT &= ~EN;  delay(1);
}

//Function to write data/command to LCD
void lcd_write(uint8_t value, uint8_t mode)
{
    if(mode == CMD)  { LCD_OUT &= ~RS; }    //Set RS -> LOW for Command mode
    else             { LCD_OUT |= RS;  }    //Set RS -> HIGH for Data mode

    LCD_OUT = ((LCD_OUT & 0x0F) | (value & 0xF0));        pulseEN(); delay(1);  //Write high nibble first
    LCD_OUT = ((LCD_OUT & 0x0F) | ((value << 4) & 0xF0)); pulseEN(); delay(1);  //Write low nibble next
 }

//Function to print a string on LCD
void lcd_print(char *s)
{
    while(*s)
    { lcd_write(*s, DATA); s++;  }
}

//Function to move cursor to desired position on LCD
void lcd_setCursor(uint8_t row, uint8_t col)
{
    const uint8_t row_offsets[] = { 0x00, 0x40};
    lcd_write(0x80 | (col + row_offsets[row]), CMD);
    delay(1);
}

//Initialize LCD
void lcd_init()
{
    P2SEL &= ~(BIT6+BIT7);
    LCD_DIR |= (D4+D5+D6+D7+RS+EN);
    LCD_OUT &= ~(D4+D5+D6+D7+RS+EN);

    delay(150);                         //Wait for power up ( 15ms )
    lcd_write(0x33, CMD);  delay(50);   //Initialization Sequence 1
    lcd_write(0x32, CMD);  delay(1);    //Initialization Sequence 2

    //All subsequent commands take 40 us to execute, except clear & cursor return (1.64 ms)
    lcd_write(0x28, CMD); delay(1);     //4 bit mode, 2 line
    lcd_write(0x0F, CMD); delay(1);     //Display ON, Cursor ON, Blink ON
    lcd_write(0x01, CMD); delay(20);    //Clear screen
    lcd_write(0x06, CMD);  delay(1);    //Auto Increment Cursor
    lcd_setCursor(0,0);                 //Goto Row 1 Column 1
}

//Clears screen
void clear_screen()
{
    //All 16 spaces on each row become blank
    lcd_setCursor(0,0);
    lcd_print("                ");
    lcd_setCursor(1,0);
    lcd_print("                ");
}

//***********************************************************************************************************
//KEYPAD
//***********************************************************************************************************
//Returns keypad value
char keypad_value()
{
    //other keys can be configured, but I only did 1, 2, and 3
    if(keypad > 141 && keypad < 141)
        return '0';
    else if (keypad > 900 && keypad < 1100)
        return '1';
    else if (keypad > 600 && keypad < 750)
        return '2';
    else if (keypad > 350 && keypad < 550)
        return '3';
    /*else if (keypad >= 160 && keypad < 163)
        return '4';
    else if (keypad > 157 && keypad < 159)
        return '5';
    else if (keypad >= 151 && keypad < 156)
        return '6';
    else if (keypad > 148 && keypad < 151)
        return '7';
    else if (keypad >= 144 && keypad < 147)
        return '8';
    else if (keypad >= 142 && keypad < 144 )
        return '9';
    else if (keypad > 107 && keypad < 113)
        return 'A';
    else if (keypad > 100 && keypad < 106)
        return 'B';
    else if (keypad > 95 && keypad < 99)
        return 'C';
    else if (keypad > 90 && keypad < 94)
        return 'D';
    else if (keypad > 87 && keypad < 89)
        return '*';
    else if (keypad > 82 && keypad < 85)
        return '#';*/
    else
        return '?'; //if key cannot be determined
}

//Checks if keypad code is correct
int check_code(char user_input[], char code[], int code_length)
{
    int k = 0;  //index

    while(k < code_length)  //loop through each digit
    {
        if (user_input[k] != code[k])   //if at least one digit doesn't match...
        {
            clear_screen();
            lcd_setCursor(0,0);
            lcd_print("Invalid password");
            __delay_cycles(1000000);

            clear_screen();
            lcd_setCursor(0,0);
            lcd_print("Security armed");
            lcd_setCursor(1,0);
            lcd_print("password:   ");

            return 0;   //code is incorrect
        }

        k++;    //go to next digit
    }

    return 1;   //code is correct if loop exited
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

	P1DIR |= (MTR1 | MTR2 | SRV);     //set motor pins as outputs
	P2DIR |= (TO2);                   //signal to MSP2

	P1OUT &= ~MTR1; //door motor starts off
	P1OUT &= ~MTR2;

	ConfigureAdc(); //set up ADC
	lcd_init();     //set up LCD
	delay(3);

	char digit = '?';   //digits of code stored here

	int code_length = 3;                //length of correct code
	char code[3] = {'1', '2', '3'};     //correct code of mailbox

    char user_input[3] = {'_', '_', '_'};   //user inputed code stored here
    int i = 0;                              //index for user_input

    int correct_code;   //1 if correct, 0 if not

    /*//for testing analog
    while(1)
    {
        getanalogvalues();
        delay(5);
    }*/

for(;;)
{
    correct_code = 0;   //both reset to 0 each loop
    flag = 0;

    P2OUT |= TO2; //disarm MSP2

    setPWM(20000, 2000);        //servo: unlock door
    __delay_cycles(1000000);

    setPWM(100, 100);           //DC motor: on
    __delay_cycles(1000000);

    P1OUT |= MTR1;              //DC motor: open door
    P1OUT &= ~MTR2;
    __delay_cycles(2000000);

    P1OUT &= ~MTR1;             //DC motor: off
    P1OUT &= ~MTR2;
    __delay_cycles(250);

    P1SEL &= ~SRV;               //PWM: off

    getanalogvalues();
    delay(5);

    if (IR_package > 800)   //if package inside
    {
        clear_screen();
        lcd_setCursor(0,0);
        lcd_print("Please remove");
        lcd_setCursor(1,0);
        lcd_print("package");

        while(IR_package > 800) //wait for package to be removed
        {
            getanalogvalues();
            delay(5);
        }
    }

    //update LCD
    clear_screen();
    lcd_setCursor(0,0);
    lcd_print("Please place");
    lcd_setCursor(1,0);
    lcd_print("package inside");

    //check if package inside
    while(IR_package < 800)
    {
        getanalogvalues();
        delay(5);
    }

    //update LCD
    clear_screen();
    lcd_setCursor(0,0);
    lcd_print("Closing door...");

    //close door
    setPWM(100, 100);
    __delay_cycles(1000000);

    while(signal < 500)
    {
        getanalogvalues();
        delay(5);

        if(IR_package < 800)    //if package removed
        {
            flag = 1;   //set flag to skip upcoming parts
            break;
        }

        if(IR_door > 600)       //if obstruction
        {
            P1OUT &= ~MTR1;     //motors off
            P1OUT &= ~MTR2;
            __delay_cycles(100000);
        }
        else
        {
            P1OUT &= ~MTR1;     //motors on
            P1OUT |= MTR2;
            __delay_cycles(100000);
        }
    }

    P1OUT &= ~MTR1;     //motors off
    P1OUT &= ~MTR2;
    __delay_cycles(250);

    if (flag == 0)
    {
        setPWM(20000, 1000);        //lock door
        __delay_cycles(1000000);
        P1SEL &= ~SRV;              //turn off PWM

        //package delivered notification to phone (do in security code?)

        P2OUT &= ~TO2; //arm MSP2
    }

    clear_screen();

    while (correct_code == 0 && flag == 0)
    {
        getanalogvalues();      //get keypad + signal value
        delay(5);

        if (signal < 500)   //if emergency signal from MSP2
        {
            lcd_setCursor(0,0);
            lcd_print("    INTRUDER    ");
            lcd_setCursor(1,0);
            lcd_print("     ALERT!     ");
            __delay_cycles(1000000);

            while(signal < 500) //keep looping if emergency
            {
                getanalogvalues();
                delay(5);
            }

            clear_screen();
            __delay_cycles(1000000);
        }

        lcd_setCursor(0,0);
        lcd_print("Security armed");
        lcd_setCursor(1,0);
        lcd_print("password:");

        digit = keypad_value();     //store value in digit

        if(digit != '?')            //if digit of code is inputed...
        {
            __delay_cycles(100000);
            user_input[i] = digit;  //store digit in user_input

            lcd_setCursor(1,10);    //update LCD
            lcd_print(user_input);
            __delay_cycles(100000);

            i++;                    //go to next spot in user_input

            if(i == code_length)    //if final digit was inputed...
            {
                correct_code = check_code(user_input, code, code_length);   //check if code is correct

                for (i = 0; i < code_length; i++) { user_input[i] = '_'; }  //reset code

                i = 0;  //reset i
            }
        }
    }
} //end for
} //end main

//interrupt
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
    __bic_SR_register_on_exit(CPUOFF);
}
