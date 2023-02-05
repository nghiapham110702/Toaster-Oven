/* 
 * File:   Lab07_main.c
 * Author: Nghia Pham (ngmpham@ucsc.edu)
 *
 * Created on November 13, 2022, 5:00 PM
 */
// **** Include libraries here ****
// Standard libraries
#include <stdio.h>
#include <string.h>
//CSE13E Support Library
#include "BOARD.h"
// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>

//User Libraries
#include "Oled.h"
#include "OledDriver.h"
#include "Adc.h"
#include "Ascii.h"
#include "Leds.h"
#include "Buttons.h"



// **** Set any macros or preprocessor directives here ****
// Set a macro for resetting the timer, makes the code a little clearer.
#define TIMER_2HZ_RESET() (TMR1 = 0)


// **** Set any local typedefs here ****
typedef enum {
    SETUP, SELECTOR_CHANGE_PENDING, COOKING, RESET_PENDING, EXTRA_CREDIT
} OvenState;

typedef struct {
    OvenState state; // used to add more state for the oven
    uint16_t initialCookTime; // Stored the inital cooking time
    uint16_t cookingTimeLeft; // check the time change
    uint16_t temperature; // change temperature and setting
    uint16_t buttonPressTime; // check how long the button is pressed
    uint8_t mode; // Check what mode of the toaster is in
} OvenData;
// cooking mode
typedef enum {
    BAKE, TOAST, BROIL
} CookingStates;
// variable declared and flag for the event
static OvenData ovenData;
//flag  time tick to  false
static uint16_t TIMER_TICK = 0;
// time that press
static uint16_t freeRunningTime = 0;
//flag adc event to false
static uint8_t adcAdjust = 0;
// set the button event to none
static uint8_t buttonState = 0x00;
// variable to store adc value
static uint16_t adcResult;
//flag event change temperature to False
static uint8_t ChangeTemp = 0;
// created variable to store durationTime
static uint16_t storeTime = 0;
// variable to track tick
static uint16_t Ticktracker = 0;
//number of Led 1- 8 used for tracking which Led need to be off
static uint16_t LEDSRange;
// this variable use to store the remainder after division
static uint16_t divisionRemainder;
// variable to calculate elapsed time
static uint16_t elaspedTime;
// store the temperature variable
static uint16_t storetemp;
// for extra credit flag alert mode to false
static uint8_t alertFinish = 0;
// **** Define any module-level, global, or external variables here ****
// constants defined below
#define LONG_PRESS 5
#define TEMPDEFAULTING 350
// **** Put any helper functions here ****


/*This function will update your OLED to reflect the state .*/
void updateOvenOLED(OvenData ovenData){
    //update OLED here
    // draw the oven
    char Result[100] = "";
    char topOn[6];
    sprintf(topOn, "%s%s%s%s%s", OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON);
    char topOff[6];
    sprintf(topOff, "%s%s%s%s%s", OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, 
            OVEN_TOP_OFF);
    char bottomOn[6];
    sprintf(bottomOn, "%s%s%s%s%s", OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, 
            OVEN_BOTTOM_ON);
    char bottomOff[6];
    sprintf(bottomOff, "%s%s%s%s%s", OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, 
            OVEN_BOTTOM_OFF);
    //base on the return value of switch statement we can check what to print 
    //based on the cooking mode of
    // the toaster. it check by, if the state is not in cooking or reset, print
    // the same message, which is editing time and temperature screen
    //, else we print what the toaster look like when it is 
    // cooking. we divide by 60 to get minute and divide by another 60 to get second 
    switch (ovenData.mode) {
        case BAKE:
            if ((ovenData.state == COOKING || ovenData.state == RESET_PENDING) == 0) {
                if (ChangeTemp == 0) {
                    sprintf(Result, "|%s| Mode: Bake\n|     | >Time: %d:%02d\n|-----|  Temp: %d%sF"
                            "\n|%s|", topOff, ovenData.initialCookTime/60, ovenData.initialCookTime%60,
                            ovenData.temperature, DEGREE_SYMBOL, bottomOff);
                } else {
                    sprintf(Result, "|%s| Mode: Bake\n|     |  Time: %d:%02d\n|-----| >Temp: %d%sF"
                            "\n|%s|", topOff, ovenData.initialCookTime/60, ovenData.initialCookTime%60,
                            ovenData.temperature, DEGREE_SYMBOL, bottomOff);
                }
            } else {
                sprintf(Result, "|%s| Mode: Bake\n|     |  Time: %d:%02d\n|-----|  Temp: %d%sF"
                            "\n|%s|", topOn, ovenData.cookingTimeLeft/60, ovenData.cookingTimeLeft%60,
                            ovenData.temperature, DEGREE_SYMBOL, bottomOn);
            }
            break;
        case TOAST:
            if ((ovenData.state == COOKING || ovenData.state == RESET_PENDING)== 0) {
                sprintf(Result, "|%s| Mode: Toast\n|     |  Time: %d:%02d\n|-----|"
                        "\n|%s|", topOff, ovenData.initialCookTime/60, ovenData.initialCookTime%60,
                         bottomOff);
            } else {
                sprintf(Result, "|%s| Mode: Toast\n|     |  Time: %d:%02d\n|-----|"
                        "\n|%s|", topOff, ovenData.cookingTimeLeft/60, ovenData.cookingTimeLeft%60,
                         bottomOn);
            }
            break;
        case BROIL:
            if ((ovenData.state == COOKING || ovenData.state == RESET_PENDING) == 0) {
                sprintf(Result, "|%s| Mode: Broil\n|     |  Time: %d:%02d\n|-----|  Temp: 500%sF"
                        "\n|%s|", topOff, ovenData.initialCookTime/60, ovenData.initialCookTime%60,
                        DEGREE_SYMBOL, bottomOff);
            } else {
                sprintf(Result, "|%s| Mode: Broil\n|     |  Time: %d:%02d\n|-----|  Temp: 500%sF"
                        "\n|%s|", topOn, ovenData.cookingTimeLeft/60, ovenData.cookingTimeLeft%60,
                        DEGREE_SYMBOL, bottomOff);
            }
            break;
    }
    // Then we clear oled and reset it
    // Draw string in the oled
    // then update oled
    OledClear(OLED_COLOR_BLACK);
    OledDrawString(Result);
    // at this point, when the cooking is finish, it will enter EXTRA_CREDIT state
    // so the screen will display inverted and normal at the same time to create
    // flashing effect which is alert the user that the cooking is finish
    if (ovenData.state == EXTRA_CREDIT){
        if(alertFinish == 0){
            OledSetDisplayInverted();
        }else{
            OledSetDisplayNormal();
        }
    }
    OledUpdate();
}

/*This function will execute your state machine.  
 * It should ONLY run if an event flag has been set.*/
void runOvenSM(void)
{
    switch (ovenData.state) {
        case SETUP:
            if (adcAdjust == 1) {
                adcResult = AdcRead() >> 2;
                // if the cooking mode is Bake and we want to change temperature
                // then we add adcResult by 300 to get desired range we want
                // if not we add 1 to adcResult so we can get desired cookingtime
                if (ovenData.mode == BAKE && ChangeTemp) {
                    ovenData.temperature = adcResult + 300;
                } else {
                    ovenData.initialCookTime = adcResult + 1;
                    ovenData.cookingTimeLeft = ovenData.initialCookTime;
                }
                updateOvenOLED(ovenData);
            }
            
            // if button 3 is pressed, there are 2 scenerio we want to check
            // if the user want to change mode or they want to change Time and
            // temperature, so we go to SELECTOR_CHANGE_PENDING to decide what
            // they want
            if (buttonState & BUTTON_EVENT_3DOWN) {
                ovenData.buttonPressTime = freeRunningTime ;
                ovenData.state = SELECTOR_CHANGE_PENDING;
            }
            // if the button 4 is pressed, we want to start the cooking,
            // turn on all the led and then take initial cook time * 5
            // divide by 8(since there are 8 led) which is the number of tick
            // it will take to cook. Also if there is a case it cant 
            // divisible by 8 then we want to store the remainder into
            // variable divisionRemainder, which will 
            //determine the time for each led to turn off 
            if (buttonState & BUTTON_EVENT_4DOWN) {
                storeTime = freeRunningTime ;
                ovenData.state = COOKING;
                updateOvenOLED(ovenData);
                LEDS_SET(0xFF);
                LEDSRange = (ovenData.initialCookTime * 5) / 8;
                divisionRemainder = (ovenData.initialCookTime * 5) % 8;
                Ticktracker = 0;
            }
            break;
        case SELECTOR_CHANGE_PENDING:
            //if the button 3 is pressed, we want to check how long is it pressed
            // if it is greater than long press, we want to stay in the mode bake
            // and edit the time and temperature
            // Then, update the oled and go back to SETUP
            if (buttonState & BUTTON_EVENT_3UP) {
                elaspedTime = freeRunningTime  - ovenData.buttonPressTime;
                if (elaspedTime > LONG_PRESS) {
                    if (ovenData.mode == BAKE) {
                        if (ChangeTemp == 0) {
                            ChangeTemp = 1;
                        } else {
                            ChangeTemp = 0;
                        }
                    }
                    updateOvenOLED(ovenData);
                    ovenData.state = SETUP;
                } else if (elaspedTime < LONG_PRESS){
                    // if it take less than a second we then change mode
                    // if the cook mode is in Broil then we want to set fixed
                    // desire temperature
                    // then update oled and set it back to SETUP
                    if (ovenData.mode == BROIL) {
                        ovenData.mode = BAKE;
                    } else {
                        ovenData.mode++;
                    }
                    if (ovenData.mode == BROIL) {
                        storetemp = ovenData.temperature;
                        ovenData.temperature = 500;
                    } else if (ovenData.mode == BAKE) {
                        ovenData.temperature = storetemp;
                    }
                    updateOvenOLED(ovenData);
                    ovenData.state = SETUP;
                    
                }
            }
            break;
        case COOKING:
            // if there is a tick, we want to check if a led need to turn off
            if (TIMER_TICK == 1) {
                // TickTracker track how many tick happened already and 
                // if the amount of tracker is == to timeticker, then we turn
                // off one LED by shifting left it by 1, then add 1 to the number
                // of ticks it takes to turn off a LED, As I said in the 
                // case SETUP, not all Time can be divisible by 8, so we have 
                // the remainder are distribute evenly among all the LED, which
                // is the the time to turn off for each LED.
                Ticktracker++;
                if ((divisionRemainder > 0 && Ticktracker == LEDSRange + 1)) {
                    Ticktracker = 0;
                    divisionRemainder--;
                    LEDS_SET( LEDS_GET()<< 1);
                }else if(divisionRemainder == 0 && Ticktracker == LEDSRange){
                    Ticktracker = 0;
                    divisionRemainder--;
                    LEDS_SET( LEDS_GET()<< 1);
                }
                
                // When cooking is finish, we want to reset time  and update the
                //oled and then we want to clear all the led.
                // Then we want to flash like we alert the user that the 
                // cooking is finish
                if (ovenData.cookingTimeLeft == 0) {
                    ovenData.cookingTimeLeft = ovenData.initialCookTime;
                    ovenData.state = EXTRA_CREDIT;
                    updateOvenOLED(ovenData);
                    LEDS_SET(0x00);
                    break;
                }
                // if the subtraction between duration time and storeTime are 
                // divisible by 5, which mean one second has passed so
                //we want to update the oled so we can see the time is 
                // counting down
                if ((freeRunningTime  - storeTime) % 5 == 0) {
                    ovenData.cookingTimeLeft--;
                    updateOvenOLED(ovenData);
                }
            }
            // while the cooking process still going and if the user pressed
            // button 4 down, then we assume the user want to 
            // reset the cooking time and then we go to RESET_PENDING state
            if (buttonState & BUTTON_EVENT_4DOWN) {
                ovenData.state = RESET_PENDING;
                ovenData.buttonPressTime = freeRunningTime;
            }
            break;
        case RESET_PENDING:
            // while we are cooking mode, if the time that
            //user press button 4 greater than or equal to 
            //1 second then we want to reset time and clear all the LED
            if (TIMER_TICK == 1) {
                if (freeRunningTime  - ovenData.buttonPressTime >= LONG_PRESS) {
                    ovenData.cookingTimeLeft = ovenData.initialCookTime;
                    ovenData.state = SETUP;
                    updateOvenOLED(ovenData);
                    LEDS_SET(0x00);
                    break;
                }
            }
            // if button 4 is take less than 1 second then we want to start the
            // cooking mode.
            if (buttonState & BUTTON_EVENT_4UP && 
                    (freeRunningTime  - ovenData.buttonPressTime < LONG_PRESS)) {
                    ovenData.state = COOKING;
            }
            break;
        case EXTRA_CREDIT:
            // For this case, when the cooking is finish,if the screen is normal,
            // then it will become inverted, and if the screen is normal
            // it will be  inverted, so the screen will go back and forth 
            // between inverted and normal inorder to create flashing effect.
            if (TIMER_TICK == 1){
                if (alertFinish == 0){
                    alertFinish = 1;
                }else{
                    alertFinish = 0;
                }
                updateOvenOLED(ovenData);
            }
            // when it is in alertmode, if the button 4 is press , then
            // it will stop the flashing effect and return back to the SETUP
            // state and then we update oled again
            if (buttonState & BUTTON_EVENT_4DOWN){
                alertFinish = 1;
                updateOvenOLED(ovenData);
                ovenData.state = SETUP;
                updateOvenOLED(ovenData);
            }
            break;
        
    }
}


int main()
{
    BOARD_Init();

     //initalize timers and timer ISRs:
    // <editor-fold defaultstate="collapsed" desc="TIMER SETUP">
    
    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a .01s timer.

    T2CON = 0; // everything should be off
    T2CONbits.TCKPS = 0b100; // 1:16 prescaler
    PR2 = BOARD_GetPBClock() / 16 / 100; // interrupt at .5s intervals
    T2CONbits.ON = 1; // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T2IF = 0; //clear the interrupt flag before configuring
    IPC2bits.T2IP = 4; // priority of  4
    IPC2bits.T2IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T2IE = 1; // turn the interrupt on

    // Configure Timer 3 using PBCLK as input. We configure it using a 1:256 prescaler, so each timer
    // tick is actually at F_PB / 256 Hz, so setting PR3 to F_PB / 256 / 5 yields a .2s timer.

    T3CON = 0; // everything should be off
    T3CONbits.TCKPS = 0b111; // 1:256 prescaler
    PR3 = BOARD_GetPBClock() / 256 / 5; // interrupt at .5s intervals
    T3CONbits.ON = 1; // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T3IF = 0; //clear the interrupt flag before configuring
    IPC3bits.T3IP = 4; // priority of  4
    IPC3bits.T3IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T3IE = 1; // turn the interrupt on;

    // </editor-fold>
   
    printf("Welcome to ngmpham's Lab07 (Toaster Oven).  Compiled on %s %s.", __TIME__, __DATE__);

    //initialize state machine (and anything else you need to init) here
    ovenData.initialCookTime = 1;
    ovenData.cookingTimeLeft = 1;
    ovenData.buttonPressTime = 0;
    ovenData.temperature = TEMPDEFAULTING;
    ovenData.state = SETUP;
    ovenData.mode = BAKE;
    
    OledInit();
    ButtonsInit();
    AdcInit();
    LEDS_INIT();
    updateOvenOLED(ovenData);
    while (1){
        // Add main loop code here:
        // check for events
        // on event, run runOvenSM()
        // clear event flags
        if (buttonState != BUTTON_EVENT_NONE || adcAdjust || TIMER_TICK) {
            runOvenSM();
            buttonState = BUTTON_EVENT_NONE;
            adcAdjust = 0;
            TIMER_TICK = 0;
        }
    }
}


/*The 5hz timer is used to update the free-running timer and to generate TIMER_TICK events*/
void __ISR(_TIMER_3_VECTOR, ipl4auto) TimerInterrupt5Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 12;

    //add event-checking code here
    // set Timertick to True  to generate TIMER_TICK events
    TIMER_TICK = 1;
    freeRunningTime ++;
}


/*The 100hz timer is used to check for button and ADC events*/
void __ISR(_TIMER_2_VECTOR, ipl4auto) TimerInterrupt100Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;

    
    //add event-checking code here
    adcAdjust = AdcChanged();
    buttonState = ButtonsCheckEvents();
}