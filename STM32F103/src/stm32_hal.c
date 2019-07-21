/**
 * STM32F103 HAL
 *
 * author: AlexandrinK <aks@cforge.org>
 **/
#include "stshell.h"

// -------------------------------------------------------------------------------------------------------------
static const char hex_digits[] = "0123456789ABCDEF";
static void usart_put_fmt(const char *fmt, va_list args);


// -------------------------------------------------------------------------------------------------------------
void timers_init(void) {
    // systick
    //SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
    SysTick_Config(SystemCoreClock / 1000); // 1 msec tick
}

void common_pins_init(void) {
    GPIO_InitTypeDef ist;
    GPIO_StructInit(&ist);

    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    //
    ist.GPIO_Mode  = GPIO_Mode_Out_PP;
    ist.GPIO_Speed = GPIO_Speed_10MHz;
    ist.GPIO_Pin   = PIN_LED1;
    //
    GPIO_Init(GPIOC, &ist);
    GPIO_ResetBits(GPIOC, PIN_LED1);
}


void usart_init(void) {
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    /* USART1 Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = PIN_USART_TX;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
 
    /* USART1 Rx (PA.10) as input floating */
    GPIO_InitStructure.GPIO_Pin = PIN_USART_RX;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = USART_BAUDRATE; 
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1,ENABLE);
}

//void i2c_init(bool_t master) {
//
//
//}

// -----------------------------------------------------------------------------------------------------------------------------------------------------
void led_toggle(bool_t st) {
    if(st) GPIO_SetBits(GPIOC, PIN_LED1);
    else GPIO_ResetBits(GPIOC, PIN_LED1);
}

char usart_get_char(bool_t nowait) {
    if(nowait) {
	if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) {
	    return (uint16_t)(USART1->DR & (uint16_t) 0x01FF);
	}
	return 0;
    }
    //
    while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
    return (uint16_t)(USART1->DR & (uint16_t) 0x01FF);
}

void usart_put_char(const char c) {
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART1->DR = (c & (uint16_t)0x01FF);
}

void usart_put_str(const char* str) {
    while(*str) {
        usart_put_char(*str++);
    }
}

void usart_put_hex_i8(uint8_t value) {
    usart_put_char(hex_digits[(value >> 0x4)]);
    usart_put_char(hex_digits[(value & 0x0f)]);
}

void usart_put_hex_i16(uint16_t value) {
    usart_put_hex_i8((value & 0xFF00) >> 8);
    usart_put_hex_i8((value & 0x00FF));
}

void usart_put_hex_i32(uint32_t value) {
    usart_put_hex_i8((value & 0xFF000000) >> 24);
    usart_put_hex_i8((value & 0x00FF0000) >> 16);
    usart_put_hex_i8((value & 0x0000FF00) >> 8);
    usart_put_hex_i8((value & 0x000000FF));
}

void usart_put_ui(uint32_t value) {
    uint16_t i = 0;
    unsigned char b1;
    signed int nb;
    int buf[16];
    while (1) {
        b1 = value % 10;
        buf[i] = b1;
        nb = value / 10;
        if (nb <= 0) break;
        i++;
        value = nb;
    }
    for (nb = i + 1; nb > 0; nb--) {
        usart_put_char(hex_digits[buf[nb - 1]]);
    }
}

void usart_put_dec(int number, uint8_t radix) {
    int i;
    if (number < 0 && radix == 10){
	usart_put_char('-');
	number = -number;
    }
    if ((i = number / radix) != 0) {
	usart_put_dec(i, radix);
    }
    usart_put_char(hex_digits[number % radix]);
}


void usart_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    usart_put_fmt(fmt, args);
    va_end(args);
}

static void usart_put_fmt(const char *fmt, va_list args) {
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's':
                    usart_put_str(va_arg(args, char *));
                    break;
                case 'c':
                    usart_put_char(va_arg(args, uint32_t));
                    break;
                case '%':
                    usart_put_char('%');
                    break;
                case 'u':
                    usart_put_ui(va_arg(args, uint32_t));
                    break;
                case 'i':
                    usart_put_dec(va_arg(args, int), 10);
                    break;
                case 'x':
                    usart_put_hex_i8(va_arg(args, uint32_t));
                    break;
                case 'X':
                    usart_put_hex_i32(va_arg(args, uint32_t));
                    break;
            }
        } else {
            usart_put_char(*fmt);
        }
        fmt++;
    }
}
