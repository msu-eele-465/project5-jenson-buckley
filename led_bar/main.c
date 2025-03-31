#include "intrinsics.h"
#include <msp430fr2310.h>
#include <stdbool.h>

//-- I2C
int received_data;
void i2c_slave_setup();

//-- I2C HEARTBEAT INDICATOR
int delay = 30000;      // delay for heartbeat
int count = 0;          // how long heartbeat has been fast

//-- LED BAR
int stepIndex = 0;                                  // Current step index
int stepOldIndex[] = {0, 0, 0, 0, 0, 0, 0, 0};      // Last index for each pattern
int prev_pattern = 0;                               // Previously displayed pattern
int stepStart = 0;                                  // Start of the selected pattern
int seqLength = 1;                                  // Length of selected sequence
int basePeriod = 128;                               // Default base period
int patternMultiplier = 1;                          // Multiplier to change base period
unsigned char stepSequence[] = {
                // Pattern 0
                0b10101010,
                0b10101010,
                // Pattern 1
                0b10101010,
                0b01010101,
                // Pattern 3
                0b00011000,
                0b00100100,
                0b01000010,
                0b10000001,
                0b01000010,
                0b00100100,
                // Pattern 5
                0b00000001,
                0b00000010,
                0b00000100,
                0b00001000,
                0b00010000,
                0b00100000,
                0b01000000,
                0b10000000,
                // Pattern 6
                0b01111111,
                0b10111111,
                0b11011111,
                0b11101111,
                0b11110111,
                0b11111011,
                0b11111101,
                0b11111110,
                // Pattern 7
                0b1,
                0b11,
                0b111,
                0b1111,
                0b11111,
                0b111111,
                0b1111111,
                0b11111111,
                // Pattern NULL
                0b0,
                0b0
            };
void setupLeds();
void setPattern(int);


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;            // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                // Enable GPIOs

    __delay_cycles(50000);               // Power-on delay

    i2c_slave_setup();                  // Setup I2C master

    // heartbeat on P2.0
    P2OUT &= ~BIT0;
    P2DIR |= BIT0;

    //-- Setup patterns
    setupLeds(); // NOTE: this doesn't seem to work if the LED bar is hooked up, so just pull it out, let it setup the pins, the drop it back in
    setPattern(9);

     // enable interrupts
    __enable_interrupt();

    int n = 0;
    while (true)
    {   
        P2OUT ^= BIT0;
        while(n < delay) {
            n++;
        }
        n = 0;
        if (count < 50) {
            count ++;
        } else {
            count = 0;
            delay = 30000;
        }
    }
}

//---------------------------------------------I2C SLAVE---------------------------------------------
void i2c_slave_setup() {
    UCB0CTLW0 |= UCSWRST;       // Software reset

    // Configure pins P1.2 (SDA) and P1.3 (SCL)
    P1SEL1 &= ~(BIT2 | BIT3);   // Clear bits in SEL1
    P1SEL0 |= (BIT2 | BIT3);    // Set bits in SEL0

    __delay_cycles(10000);

    UCB0CTLW0 = UCSWRST | UCMODE_3 | UCSYNC;   // I2C mode, sync, hold in reset
    UCB0CTLW0 &= ~UCMST;        // Slave mode
    UCB0I2COA0 = 0x45 | UCOAEN; // Own address + enable
    UCB0CTLW1 = 0;              // No auto STOP
    UCB0CTLW0 &= ~UCTR;         // Receiver mode

    __delay_cycles(10000);      // Wait before releasing reset

    UCB0CTLW0 &= ~UCSWRST;      // Exit reset

    UCB0IE |= UCRXIE0;          // Enable receive interrupt
}

// I2C Interrupt Service Routine
#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    received_data = UCB0RXBUF; // Read received byte

    // if a pattern is received...
    if (received_data < 8) {
        // save previous state
        stepOldIndex[prev_pattern] = stepIndex;
        if (prev_pattern != received_data) {
            stepIndex = stepOldIndex[received_data];
        } else {
            stepIndex = 0;
        }
        // update prev pattern
        prev_pattern = received_data;
    }
    setPattern(received_data);
    delay = 10000;
    count = 0;
}

//---------------------------------------------LED BAR PATTERNS---------------------------------------------

void setupLeds() {
    // Configure Leds (P1.1, P1.0, P2.7, P2.6, P1.4, P1.5, P1.6, P1.7)
    P1DIR |= BIT1 | BIT0 | BIT4 | BIT5 |BIT6 | BIT7;
    P2DIR |= BIT7 | BIT6;
    P1OUT &= ~(BIT1 | BIT0 | BIT4 | BIT5 |BIT6 | BIT7);
    P2OUT &= ~(BIT7 | BIT6);

    // Setup Timer 0 B3
    TB1CTL = TBSSEL__ACLK | MC__UP | TBCLR | ID__8; // SMCLK (1Mhz), Stop mode, clear timer, divide by 8
    TB1EX0 = TBIDEX__8 ;   // Extra division by 4
    TB1CCR0 = basePeriod;  // Set initial speed
    TB1CCTL0 |= CCIE;      // Enable compare interrupt
}

void setPattern(int a) {
    switch (a) {
        case 10:    // dec base period
            if (basePeriod > 32) {
                basePeriod -= 32;
            }
            break;

        case 11:    // inc base period
            if (basePeriod < 608) {
                basePeriod += 32;
            }
            break;

        case 0:
            stepStart = 0;
            seqLength = 2;
            patternMultiplier = 4;
        break;
        
        case 1:
            stepStart = 2;
            seqLength = 2;
            patternMultiplier = 4;
        break;
        
        case 3:
            stepStart = 4;
            seqLength = 6;
            patternMultiplier = 2;
        break;
        
        case 5:
            stepStart = 10;
            seqLength = 8;
            patternMultiplier = 6;
        
        case 6:
            stepStart = 18;
            seqLength = 8;
            patternMultiplier = 2;
        break;

        case 7:
            stepStart = 26;
            seqLength = 8;
            patternMultiplier = 4;
        break;

        default:
            stepStart = 34;
            seqLength = 2;
            patternMultiplier = 4;
        break;
    }

    TB1CCR0 = basePeriod * patternMultiplier;
    
}

#pragma vector = TIMER1_B0_VECTOR
__interrupt void ISR_TB3_CCR0(void)
{
    // Update Leds (P1.1, P1.0, P2.7, P2.6, P1.4, P1.5, P1.6, P1.7)
    P1OUT = ((stepSequence[stepIndex + stepStart] <<1 ) & 0b10) |
            ((stepSequence[stepIndex + stepStart] >>1 ) & 0b1) |
            ((stepSequence[stepIndex + stepStart]) & 0b10000) |
            ((stepSequence[stepIndex + stepStart]) & 0b100000) |
            ((stepSequence[stepIndex + stepStart]) & 0b1000000) |
            ((stepSequence[stepIndex + stepStart]) & 0b10000000); // LSB (0) to P1.1 | 1 to P1.0 | 4 to P1.4 | 5 to P1.5 | 6 to P1.6 | 7 to P1.7
    P2OUT = ((stepSequence[stepIndex + stepStart] <<5 ) & 0b10000000) |
            ((stepSequence[stepIndex + stepStart] <<3 ) & 0b1000000);  // 2 to P2.7 | 3 to P2.6

    stepIndex = (stepIndex + 1) % seqLength; // Update step index
    TB1CCTL0 &= ~CCIFG;  // clear CCR0 IFG
}