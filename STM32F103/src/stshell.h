/**
 * ST-Shell
 *
 * author: AlexandrinK <aks@cforge.org>
 **/
#ifndef ST_SHELL__H
#define ST_SHELL__H

#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_tim.h>
#include <stm32f10x_usart.h>
#include <misc.h>
#include <stdarg.h>

// ------------------------------------------------------------------------------
#define PIN_LED1	GPIO_Pin_13	// dev board pin
#define PIN_USART_TX	GPIO_Pin_9	// A9
#define PIN_USART_RX	GPIO_Pin_10	// A10

// ------------------------------------------------------------------------------
#define VESION_ID "ST-Shell (ver.1.0.0)"
#define BOARD_ID  "STM32F103"

#define NULL 	0
#define TRUE 	1
#define FALSE	0
#define ARRAY_SIZE(a) ((sizeof(a))/(sizeof((a)[0])))

#define CMD_BUFFER_SIZE			129
#define CMD_NAME_LEN_MAX		16
#define CMD_MAX_ARGS			10
#define USART_BAUDRATE			115200

#define	STATUS_SUCCESS			0x00
#define STATUS_ERROR			0x01
#define STATUS_NOT_IMPLEMENTED		0x02
#define STATUS_INVALID_ARGUMENTS	0x03
#define STATUS_UNKNOWN_COMMAND		0x04
#define STATUS_COMMAND_TOO_LONG		0x05

// ------------------------------------------------------------------------------
typedef uint8_t status_t;
typedef uint8_t bool_t;
typedef status_t (*fnc_void_t)(char *);

typedef struct ucmd_entry_s {
    char        *name;
    uint8_t     params;
    fnc_void_t  fnc;
    char        *descr;
    char        *usage;
} ucmd_entry_t;

typedef struct status_entry_s {
    uint8_t     code;
    char        *text;
} status_entry_t;

// ------------------------------------------------------------------------------
// common functions
uint32_t systick_get();
void systick_sleep(uint32_t delay);
bool_t systick_asleep(uint32_t ststamp, uint32_t delay);

void bzero(void *tov, uint16_t len);
void bcopy(const void *src, void *dest, uint16_t len);
uint16_t strlen(const char *s);
uint8_t strcmp(const char *s1, const char *s2);
char *strchr(const char *s, uint8_t c);

int atoi(const char *str);
int atoi2(const char *str);
uint8_t parse_args(char *p, char **args_ret, uint8_t args_size);


// ------------------------------------------------------------------------------
// stm hal 
void common_pins_init(void);
void usart_init(void);
void timers_init(void);

void led_toggle(bool_t st);

char usart_get_char(bool_t nowait);
void usart_put_char(const char c);
void usart_put_str(const char* str);
void usart_printf(const char *fmt, ...);


#endif
