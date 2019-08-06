/**
 * STM32F030 HAL
 *
 * author: AlexandrinK <aks@cforge.org>
 **/
#include "stshell.h"

// -------------------------------------------------------------------------------------------------------------
// PINS CONFING
#define HAL_USART_PIN_TX   	GPIO_Pin_9		// PA9
#define HAL_USART_APIN_TX   	GPIO_PinSource9
#define HAL_USART_PIN_RX   	GPIO_Pin_10		// PA10
#define HAL_USART_APIN_RX   	GPIO_PinSource10
#define HAL_LED_PIN        	GPIO_Pin_4 		// PA4


static const char hex_digits[] = "0123456789ABCDEF";
static void usart_put_fmt(const char *fmt, va_list args);

// -------------------------------------------------------------------------------------------------------------
void timers_init(void) {
    SysTick_Config(SystemCoreClock / 1000);

}

void common_pins_init(void) {
    GPIO_InitTypeDef ist;
    GPIO_StructInit(&ist);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);
    //
    ist.GPIO_Mode  = GPIO_Mode_OUT;
    ist.GPIO_OType = GPIO_OType_PP;
    ist.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    ist.GPIO_Speed = GPIO_Speed_10MHz;
    ist.GPIO_Pin   = HAL_LED_PIN;
    //
    GPIO_Init(GPIOA, &ist);
    GPIO_ResetBits(GPIOA, HAL_LED_PIN);
}

// using PINS: 9
void usart_init(void) {
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = USART_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = HAL_USART_PIN_TX | HAL_USART_PIN_RX;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // GPIO_Speed_10MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, HAL_USART_APIN_TX, GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA, HAL_USART_APIN_RX, GPIO_AF_1);

    USART_InitStructure.USART_BaudRate = USART_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1,ENABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------
void led_toggle(bool_t st) {
    if(st) GPIO_SetBits(GPIOA, HAL_LED_PIN);
    else GPIO_ResetBits(GPIOA, HAL_LED_PIN);
}

char usart_get_char(bool_t nowait) {
    if(nowait) {
        if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) {
            return (uint16_t)(USART1->RDR & (uint16_t) 0x01FF);
        }
        return 0;
    }
    //
    while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
    return (uint16_t)(USART1->RDR & (uint16_t) 0x01FF);
}

void usart_put_char(const char c) {
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART1->TDR = (c & (uint16_t) 0x01FF);
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

