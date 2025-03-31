#include <msp430fr2355.h>

// Pin Definitions
#define LCD_RS BIT0     // Register Select
#define LCD_RW BIT1     // Read/Write
#define LCD_E  BIT2     // Enable
#define LCD_DATA P2OUT  // Data bus on Port 1
#define MAX_MSG_LEN 32

volatile char message_buffer[MAX_MSG_LEN];
volatile unsigned char msg_index = 0;
volatile unsigned char received_data = 0;


// Delay Function
void delay(unsigned int count) {
    while(count--) __delay_cycles(1000);
}

// Enable Pulse
void lcd_enable_pulse() {
    P3OUT |= LCD_E;
    delay(1);
    P3OUT &= ~LCD_E;
}

// Command Write Function
void lcd_write_command(unsigned char cmd) {
    P3OUT &= ~LCD_RS;  // RS = 0 for command
    P3OUT &= ~LCD_RW;  // RW = 0 for write
    LCD_DATA = cmd;    // Write command to data bus
    lcd_enable_pulse();
    delay(1);         // Command execution delay
}

// Data Write Function
void lcd_write_data(unsigned char data) {
    P3OUT |= LCD_RS;   // RS = 1 for data
    P3OUT &= ~LCD_RW;  // RW = 0 for write
    LCD_DATA = data;   // Write data to data bus
    lcd_enable_pulse();
    delay(1);         // Data write delay
}

// LCD Initialization
void lcd_init() {
    P3DIR |= LCD_RS | LCD_RW | LCD_E;
    P2DIR |= 0xFF;   // Set Port 1 as output for data bus

    delay(50);    // Power-on delay

    lcd_write_command(0x38); // Function set: 8-bit, 2 lines, 5x8 dots
    lcd_write_command(0x0C); // Display ON, Cursor OFF
    lcd_clear();
    lcd_write_command(0x06); // Entry mode set: Increment cursor
}

// Set LCD Cursor Position
void lcd_set_cursor(unsigned char address) {
    lcd_write_command(0x80 | address);
    delay(1);
}

// Display String on LCD
void lcd_display_string(char *str) {
    while(*str) {
        lcd_write_data(*str++);
    }
}

// Clear LCD
void lcd_clear(){
    lcd_write_command(0x01); // Clear display
    delay(1);             // Delay for clear command
}

void i2c_slave_setup() {
    UCB0CTLW0 |= UCSWRST;       // Software reset

    // Configure pins P1.2 (SDA) and P1.3 (SCL)
    P1SEL1 &= ~(BIT2 | BIT3);   // Clear bits in SEL1
    P1SEL0 |= (BIT2 | BIT3);    // Set bits in SEL0

    __delay_cycles(10000);

    UCB0CTLW0 = UCSWRST | UCMODE_3 | UCSYNC;   // I2C mode, sync, hold in reset
    UCB0CTLW0 &= ~UCMST;        // Slave mode
    UCB0I2COA0 = 0x40 | UCOAEN; // Own address + enable
    UCB0CTLW1 = 0;              // No auto STOP
    UCB0CTLW0 &= ~UCTR;         // Receiver mode

    __delay_cycles(10000);      // Wait before releasing reset

    UCB0CTLW0 &= ~UCSWRST;      // Exit reset

    UCB0IE |= UCRXIE0;          // Enable receive interrupt
}

// Main Program
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;       // Turn on GPIO

    __delay_cycles(50000);      // Wait ~50ms after power-up
    i2c_slave_setup();          // Setup I2C
    lcd_init();                 // Initialize LCD
    __enable_interrupt();       // Enable global iunterupts

    lcd_display_string("Ready");

    while (1) {
    }
}

#pragma vector=EUSCI_B0_VECTOR
__interrupt void EUSCI_B0_I2C_ISR(void) {
    char c = UCB0RXBUF;

    if (c == '\n') {
        // End of message, null-terminate
        message_buffer[msg_index] = '\0';

        lcd_clear();

        // Print first 16 characters on top row
        lcd_set_cursor(0x00);
        for (unsigned char i = 0; i < 16 && message_buffer[i] != '\0'; i++) {
            lcd_write_data(message_buffer[i]);
        }

        // Print next 16 characters on bottom row
        lcd_set_cursor(0x40);
        for (unsigned char i = 16; i < 32 && message_buffer[i] != '\0'; i++) {
            lcd_write_data(message_buffer[i]);
        }

        msg_index = 0;  // Reset for next message
    } else {
        if (msg_index < MAX_MSG_LEN - 1) {
            message_buffer[msg_index++] = c;
        } else {
            msg_index = 0; // reset if too long
        }
    }
}

