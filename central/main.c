#include "gpio.h"
#include <driverlib.h>

void setupKeypad();
char readKeypad();
int checkRows();
char lastKey = 'X';

// STATE
    // 0    Locked
    // 1    First correct digit entered
    // 2    Second
    // 3    Third
    // 4    Unlocked
int state = 0;

int main(void) {

    volatile uint32_t i;
    char key_val;

    // Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);

    // Set P1.0 LED
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    // Set P6.6 LED
    P6DIR |= BIT6;
    P6OUT &= ~BIT6;

    // Setup Keypad
    setupKeypad();

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();

    
    while(1)
    {

        key_val = readKeypad();
        if (key_val != 'X') {
            if (state == 0) {
                if (key_val=='1') {
                    state = 1;
                    P1OUT |= BIT0;
                } else {
                    state = 0;
                    P1OUT &= ~BIT0;
                }

            } else if (state == 1) {
                if (key_val=='1') {
                    state = 2;
                } else {
                    state = 0;
                    P1OUT &= ~BIT0;
                }
                
            } else if (state == 2) {
                if (key_val=='1') {
                    state = 3;
                } else {
                    state = 0;
                    P1OUT &= ~BIT0;
                }

            } else if (state == 3) {
                if (key_val=='1') {
                    state = 4;
                    P6OUT |= BIT6;
                } else {
                    state = 0;
                    P1OUT &= ~BIT0;
                }

            } else if (state == 4) {
                if (key_val=='D') {             // lock
                    state = 0;
                    P1OUT &= ~BIT0;
                    P6OUT &= ~BIT6;
                } else if (key_val=='A') {      // decrease base period by 0.25 s
                    
                } else if (key_val=='B') {      // increase base period by 0.25 s
                    
                } else if (key_val=='0') {      // pattern 0
                    
                } else if (key_val=='1') {      // pattern 0
                    
                } else if (key_val=='2') {      // pattern 2
                    
                } else if (key_val=='3') {      // pattern 0
                    
                } else if (key_val=='4') {      // pattern 0
                    
                } else if (key_val=='5') {      // pattern 0
                    
                } else if (key_val=='6') {      // pattern 0
                    
                } else if (key_val=='7') {      // pattern 0
                    
                }
            } else {
                state = 0;
                P1OUT &= ~BIT0;
                P6OUT &= ~BIT6;
            }
        }
    }
}

// --------------------------------------------- KEYPAD ---------------------------------------------
void setupKeypad() {
    // columns as outputs on P1.4, P5.3, P5.1, P5.0 initialized to 0
    P1DIR |= BIT4;
    P5DIR |= BIT3;
    P5DIR |= BIT1;
    P5DIR |= BIT0;
    P1OUT &= ~BIT4;      
    P5OUT &= ~BIT3;
    P5OUT &= ~BIT1;
    P5OUT &= ~BIT0;
    // rows as inputs pulled down internally on P5.4, P1.1, P3.5, 3.1
    P5DIR &= ~BIT4;     // inputs
    P1DIR &= ~BIT1;
    P3DIR &= ~BIT5;
    P3DIR &= ~BIT1;
    P5REN |= BIT4;      // internal resistors
    P1REN |= BIT1;
    P3REN |= BIT5;
    P3REN |= BIT1;
	P5OUT &=~ BIT4;     // pull-downs
	P1OUT &=~ BIT1;
	P3OUT &=~ BIT5;
	P3OUT &=~ BIT1;
}

char readKeypad() {
    // columns on P1.4, P5.3, P5.1, P5.0
    // rows on P5.4, P1.1, P3.5, 3.1

    char keys[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };

    char pressed = 'X';

    // check col 1
    P1OUT |= BIT4;
    int row = checkRows();
    if (row!=-1) {
        pressed = keys[row][0];
    } 
    P1OUT &= ~BIT4;

    // check col 2
    P5OUT |= BIT3;
    row = checkRows();
    if (row!=-1) {
        pressed =  keys[row][1];
    } 
    P5OUT &= ~BIT3;

    // check col 3
    P5OUT |= BIT1;
    row = checkRows();
    if (row!=-1) {
        pressed =  keys[row][2];
    }
    P5OUT &= ~BIT1;

    // check col 4
    P5OUT |= BIT0;
    row = checkRows();
    if (row!=-1) {
        pressed =  keys[row][3];
    }
    P5OUT &= ~BIT0;

    if (pressed != lastKey) {
        lastKey = pressed;
        return pressed;
    } else {
        return 'X';
    }
}

int checkRows() {
    // check inputs for rows on P5.4, P1.1, P3.5, 3.1 (rows 1-4)
    if (P5IN & BIT4) {
        return 0;
    } else if (P1IN & BIT1) {
        return 1;
    } else if (P3IN & BIT5) {
        return 2;
    } else if (P3IN & BIT1) {
        return 3;
    } else {
        return -1;
    }
}
//---------------------------------------------