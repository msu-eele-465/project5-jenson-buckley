#include "gpio.h"
#include "intrinsics.h"
#include "msp430fr2355.h"
#include <driverlib.h>
#include <math.h>

//-- KEYPAD
void setupKeypad();                         // init
char readKeypad();                          // checks for pressed keys on keypad
int checkCols();                            // ussed internally by readKeypad()
char lastKey = 'X';                         // used internally for debouncing

//-- ADC SAMPLING AND AVERAGING
void setupADC();                            // init
void readADC();                             // read value from ADC into adc_val
int adc_val;                                // ADC buffer read into this value by ISR
unsigned int adc_sensor_array[100];         // array to store values used to calculate average
unsigned int adc_filled = 0;                // number of values used in average (when adc_filled == adc_buffer_length, average is accurate)
int adc_buffer_length = 3;                  // number of values to  be used in average: can be [1,100]
unsigned int temp_adc_buffer_length = 0;    // holds number of values to be used in average while user is entering the number
unsigned int adc_tens = 0;                  // used to input multiple-digit number for window size
unsigned int adc_sensor_ave = 0;            // sensor average in ADC code
void setupSampleClock();                    // setup clock on TB3 to sample ADC every 0.5s

//-- ADC CODE TO C/F CONVERSION
#define ADC_SCALER (3.3 / 4095.0)           // 3.3V / 2^12
float adc2c(unsigned int code);             // convert ADC code to celcius
float adc2f(unsigned int code);             // convert ADC code to fahrenheit
int corf_toggle = 0;                        // toggle temperature units between F (1) and C (0)
float average = 0;                          // sensor average in units matching corf_toggle

//-- LCD
char message[32] = {"LOCKED          T=##.#°X    N=3  "};   // 32 characters long; first 16 are the top row, rest are the bottom
char status[16] = {"LOCKED          "}                      // 16 chars that can be written to whole top row (index [0,15])
char temp[4] = {"00.0"};                                    // 4 characters that can written to bottom row (index [18,21])
char window[3] = {"3  "};                                    // 3 characters that can written to bottom row (index [29,31])

// STATE
    // 0    Locked
    // 1    First correct digit entered
    // 2    Second
    // 3    Third
    // 4    Unlocked
    // 5    Pattern input
    // 6    Windown input
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
    
    // TODO: send default message (N=3 and locked)

    while(1)
    {

        key_val = readKeypad();
        if (key_val != 'X') {
            if (state == 0) {
                if (key_val=='1') {
                    state = 1;
                    P1OUT |= BIT0;
                    // TODO: send "UNLOCKING" to top row of LCD
                } else {
                    state = 0;
                    P1OUT &= ~BIT0;
                    // TODO: send "LOCKED" to top row of LCD
                }

            } else if (state == 1) {
                if (key_val=='1') {
                    state = 2;
                } else {
                    state = 0;
                    P1OUT &= ~BIT0;
                    // TODO: send "LOCKED" to top row of LCD
                }
                
            } else if (state == 2) {
                if (key_val=='1') {
                    state = 3;
                } else {
                    state = 0;
                    P1OUT &= ~BIT0;
                    // TODO: send "LOCKED" to top row of LCD
                }

            } else if (state == 3) {
                if (key_val=='1') {
                    state = 4;
                    P6OUT |= BIT6;
                    // TODO: send "UNLOCKED" to top row of LCD
                } else {
                    state = 0;
                    P1OUT &= ~BIT0;
                    // TODO: send "LOCKED" to top row of LCD
                }

            } else if (state == 4) {
                if (key_val=='D') {             // lock
                    state = 0;
                    P1OUT &= ~BIT0;
                    // TODO: send "LOCKED" to top row of LCD
                    P6OUT &= ~BIT6;

                } else if (key_val=='A') {      // enter pattern
                    // TODO: send "set pattern" to top row of LCD screen
                    state = 5;

                } else if (key_val=='B') {      // enter window
                    // TODO: send "set window size" to top row of LCD screen
                    adc_tens = 0;
                    temp_adc_buffer_length = 0;
                    state = 6;

                } else if (key_val=='C') {      // toggle degF/degC
                    corf_toggle ^= 1;
                }

            } else if (state == 5) {
                if (key_val=='D') {             // lock
                    state = 0;
                    P1OUT &= ~BIT0;
                    // TODO: send "LOCKED" to top row of LCD
                    P6OUT &= ~BIT6;
                } else if (key_val=='A') {      // decrease base period by 0.25 s
                    // TODO: I2C - send base period dec
                } else if (key_val=='B') {      // increase base period by 0.25 s
                    // TODO: I2C - send base period inc
                } else if (key_val=='0') {      // pattern 0
                    // TODO: I2C - pattern 0
                } else if (key_val=='1') {      // pattern 1
                    // TODO: I2C - pattern 1
                } else if (key_val=='2') {      // pattern 2
                    // TODO: I2C - pattern 2
                } else if (key_val=='3') {      // pattern 3
                    // TODO: I2C - pattern 3
                } else if (key_val=='4') {      // pattern 4
                    // TODO: I2C - pattern 4
                } else if (key_val=='5') {      // pattern 5
                    // TODO: I2C - pattern 5
                } else if (key_val=='6') {      // pattern 6
                    // TODO: I2C - pattern 6
                } else if (key_val=='7') {      // pattern 7
                    // TODO: I2C - pattern 7
                } else if (key_val=='*') {      // exit
                    state = 4;
                }

            } else if (state == 6) {
                if (key_val=='D') {             // lock
                    state = 0;
                    P1OUT &= ~BIT0;
                    // TODO: send "LOCKED" to top row of LCD
                    P6OUT &= ~BIT6;
                    
                } else if ((key_val >= '0') & (key_val <= '9')) {
                    temp_adc_buffer_length = temp_adc_buffer_length*pow(10, adc_tens)+(key_val-'0');
                
                } else if (key_val=='*') {      // exit

                    if ((temp_adc_buffer_length > 0) & (temp_adc_buffer_length < 101)) {                         // update length of rolling average
                        adc_buffer_length = temp_adc_buffer_length;
                    } else {
                        adc_buffer_length = 3;
                    }
                    memset(adc_sensor_array, 0, sizeof(adc_sensor_array));  // clear collected values used for average
                    adc_sensor_ave = 0;                                     // clear average
                    adc_filled = 0;                                         // reset counter for values used in average
                    // TODO: send "N = {adc_buffer_length}" to the bottom right corner of the LCD (longest will be "N = 100")
                    state = 4;
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

    // check row 1
    P1OUT |= BIT4;
    int col = checkCols();
    if (col!=-1) {
        pressed = keys[0][col];
    } 
    P1OUT &= ~BIT4;

    // check row 2
    P5OUT |= BIT3;
    col = checkCols();
    if (col!=-1) {
        pressed =  keys[1][col];
    } 
    P5OUT &= ~BIT3;

    // check row 3
    P5OUT |= BIT1;
    col = checkCols();
    if (col!=-1) {
        pressed =  keys[2][col];
    }
    P5OUT &= ~BIT1;

    // check row 4
    P5OUT |= BIT0;
    col = checkCols();
    if (col!=-1) {
        pressed =  keys[3][col];
    }
    P5OUT &= ~BIT0;

    if (pressed != lastKey) {
        lastKey = pressed;
        return pressed;
    } else {
        return 'X';
    }
}

int checkCols() {
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
    int popped = adc_sensor_array[adc_buffer_length-1];
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
    } else {
        if (corf_toggle == 0) {
            average = adc2c(adc_sensor_ave);
            // TODO: send C average to bottom left of LCD
        } else {
            average = adc2f(adc_sensor_ave);
            // TODO: send F average to bottom left of LCD
        }
    }

    // read value
    readADC();

    // clear CCR0 IFG
    TB3CCTL0 &= ~CCIFG;
}

//---------------------------------------------ADC CONVERSIONS---------------------------------------------
float adc2c(unsigned int code) {
    // 12-bit adc conversion with 3.3V reference
    float voltage = ((float) code) * ADC_SCALER;
    // Vo to C conversion from LM19 datasheet
    float celcius = -1481.96 + sqrt(2.1962e6 + (1.8639 - voltage) / (3.88e-6));
    return celcius;
}

float adc2f(unsigned int code) {
    float fahrenheit = 1.8*adc2c(code)+32.0;
    return fahrenheit;
}