#include "gpio.h"
#include "intrinsics.h"
#include "msp430fr2355.h"
#include <driverlib.h>

void setupKeypad();
char readKeypad();
int checkRows();
char lastKey = 'X';

void setupADC();
void readADC();
unsigned int adc_val;
unsigned int adc_sensor_array[100];
unsigned int adc_filled = 0;
unsigned int adc_buffer_length = 3;
unsigned int adc_sensor_ave = 0;

void setupSampleClock();

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

    // Setup Keypad on P1.4-3.1 (one rail of MSP breakout board)
    setupKeypad();

    // Setup ADC conversion
    setupADC();
    setupSampleClock();

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();

    // enable interrupts
    __enable_interrupt();
    
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
                if (adc_val > 0x7FF) {
                    P6OUT ^= BIT6;
                }

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

//---------------------------------------------ADC---------------------------------------------
void setupADC() {
    // Configure ADC A10 pin on P5.2
    P5SEL0 |= BIT2;
    P5SEL1 |= BIT2;

    // Configure ADC12
    ADCCTL0 |= ADCSHT_2 | ADCON;                             // ADCON, S&H=16 ADC clks
    ADCCTL1 |= ADCSHP;                                       // ADCCLK = MODOSC; sampling timer
    ADCCTL2 &= ~ADCRES;                                      // clear ADCRES in ADCCTL
    ADCCTL2 |= ADCRES_2;                                     // 12-bit conversion results
    ADCMCTL0 |= ADCINCH_10;                                   // A10 ADC input select; Vref=AVCC
    ADCIE |= ADCIE0;                                         // Enable ADC conv complete interrupt
}

void readADC() {
    // start sampling and conversion
    ADCCTL0 |= ADCENC | ADCSC;
}

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
{
    switch(ADCIV)
    {
        case ADCIV_ADCIFG:
            adc_val = ADCMEM0;
            break;
        default:
            break;
    }
}

//---------------------------------------------Sample Clock---------------------------------------------
void setupSampleClock() {
    TB3CTL |= TBCLR;    // reset settings
    TB3CTL |= TBSSEL__ACLK | MC__UP | ID__8;    // 32.768 kHz / 8 = 4096 / 2 - 1 = 2047
    TB3CCR0 = 2047;                             // period of .5s
    TB3CCTL0 |= CCIE;                           // Enable capture compare
    TB3CCTL0 &= ~CCIFG;                         // Clear IFG

    // start ADC read for first sample
    readADC();                                  
}

#pragma vector = TIMER3_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void)
{
    // update array       
    unsigned int popped = adc_sensor_array[adc_buffer_length-1];
    int i;
    for (i=adc_buffer_length-1; i>0; i--) {
        adc_sensor_array[i] = adc_sensor_array[i-1];
    }
    adc_sensor_array[0] = adc_val;

    // update average with cool move
    adc_sensor_ave += (adc_val-popped)/adc_buffer_length;

    // keep track of how many values have been read for average
    if (adc_filled < adc_buffer_length) {
        adc_filled++;
    }

    // read value
    readADC();

    // clear CCR0 IFG
    TB3CCTL0 &= ~CCIFG;
}