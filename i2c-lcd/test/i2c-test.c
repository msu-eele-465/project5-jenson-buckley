#include <msp430fr2355.h>

// Pin Definitions
#define LCD_RS BIT0     // Register Select
#define LCD_RW BIT1     // Read/Write
#define LCD_E  BIT2     // Enable
#define LCD_DATA P2OUT  // Data bus on Port 1

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

// Main Program
int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;     // Unlock GPIO ports

    lcd_init();               // Initialize LCD

    lcd_set_cursor(0x00);     // Set cursor to beginning of first row
    lcd_display_string("Hello World    ");

    lcd_set_cursor(0x40);     // Set cursor to beginning of second row
    lcd_display_string("I like ham      ");

    //delay(1000);
    //lcd_clear();
    //lcd_display_string("Hello Peter"); // Display sample text


    while (1) {
    }
}