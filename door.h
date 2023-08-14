#ifndef DOOR_H_
#define DOOR_H_

//LCD SCREEN
#define CMD         0
#define DATA        1
#define RS          BIT2
#define EN          BIT3
#define D4          BIT4
#define D5          BIT5
#define D6          BIT6
#define D7          BIT7
#define LCD_OUT     P2OUT
#define LCD_DIR     P2DIR

int CountVal=0;
char str[4];

//KEYPAD 1x3
#define KYPD        BIT1

//DC MOTOR
#define MTR1        BIT4    //01 forward
#define MTR2        BIT5    //10 reverse

//SERVO MOTOR
#define SRV         BIT6

//INFRARED
#define IRP         BIT2    //package
#define IRD         BIT3    //door

//MSP1 and MSP2 communication link
#define TO2         BIT0    //port 2
#define FROM2       BIT0    //port 1

#endif
