#define MAX7219_NOOP         0x00
#define MAX7219_DIGIT0       0x01
#define MAX7219_DIGIT1       0x02
#define MAX7219_DIGIT2       0x03
#define MAX7219_DIGIT3       0x04
#define MAX7219_DIGIT4       0x05
#define MAX7219_DIGIT5       0x06
#define MAX7219_DIGIT6       0x07
#define MAX7219_DIGIT7       0x08
#define MAX7219_DECODE_MODE  0x09
#define MAX7219_INTENSITY    0x0a
#define MAX7219_SCAN_LIMIT   0x0b
#define MAX7219_SHUTDOWN     0x0c
#define MAX7219_DISPLAY_TEST 0x0f

#define MINUS 0x0A
#define BLANK 0x0F
#define POINT 0xF0
#define ERROR 0x0B


void display_init();
void display_write(uint8_t reg, uint8_t value);
