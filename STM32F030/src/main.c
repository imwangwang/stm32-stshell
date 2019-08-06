/**
 * ST-Shell
 *
 * author: AlexandrinK <aks@cforge.org>
 **/
#include "stshell.h"
#include "consts.h"

static struct {
    uint8_t		cmd_buffer[CMD_BUFFER_SIZE];
    uint8_t 	cmd_buffer_pos;
    uint16_t	led_delay;
    bool_t		fl_usart_buffer_ready;
    bool_t		fl_usart_ignore_input;
    bool_t		fl_led_flash;
    bool_t		fl_cpu_halt;
    volatile uint32_t	systick_usec;
} globals;

//
status_t ucmd_help();
status_t ucmd_show_version();
status_t ucmd_core_halt();
status_t ucmd_reboot();
status_t ucmd_echo(char *args);
status_t ucmd_set_led_delay(char *args);

// ----------------------------------------------------------------------------------------------------------------------------------------
static ucmd_entry_t UCMD_MAP[] = {
    {"?",       0, (fnc_void_t) ucmd_help, "this help", NULL},
    {"ver",     0, (fnc_void_t) ucmd_show_version, "show version", NULL},
    {"halt",    0, (fnc_void_t) ucmd_core_halt, "halt CPU", NULL},
    {"reboot",  0, (fnc_void_t) ucmd_reboot, "reboot the device", NULL},
    {"flash",   1, (fnc_void_t) ucmd_set_led_delay, "led flash","flash [NNN]"},
    {"echo",    1, (fnc_void_t) ucmd_echo, "echo","echo [a1, a2, ...]"},
};
#define UCMD_MAP_SIZE ARRAY_SIZE(UCMD_MAP)

static status_entry_t STATUS_MAP[] = {
    {STATUS_SUCCESS, ""},
    {STATUS_ERROR, "generic error"},
    {STATUS_NOT_IMPLEMENTED, "not implemented"},
    {STATUS_INVALID_ARGUMENTS, "invalid arguments"},
    {STATUS_UNKNOWN_COMMAND, "unknown command"},
    {STATUS_COMMAND_TOO_LONG,"command name is too long"},
};
#define SMAP_SIZE ARRAY_SIZE(STATUS_MAP)

// ----------------------------------------------------------------------------------------------------------------------------------------
// common functions
// ----------------------------------------------------------------------------------------------------------------------------------------
uint32_t systick_get() {
  return globals.systick_usec;
}

void systick_sleep(uint32_t delay) {
    uint32_t t = 0;
    if(delay > 0) {
        t = systick_get() + delay;
        while(globals.systick_usec < t) __NOP();
    }
}

bool_t systick_asleep(uint32_t ststamp, uint32_t delay) {
    if(ststamp <= 0 || delay <= 0) return TRUE;
    return (globals.systick_usec >= (ststamp + delay));
}

ucmd_entry_t *ucmd_lookup(char *name) {
    ucmd_entry_t *cmd = NULL;
    uint8_t i;
    //
    for(i = 0; i < UCMD_MAP_SIZE; i++) {
        if(strcmp(name, UCMD_MAP[i].name) == 0) {
            cmd = &UCMD_MAP[i];
            break;
        }
    }
    return cmd;
}


char *status_get_description(status_t code) {
    if(code <= 0 || code > SMAP_SIZE) return "";
    return STATUS_MAP[code].text;

}

// ----------------------------------------------------------------------------------------------------------------------------------------
// cmd functions
// ----------------------------------------------------------------------------------------------------------------------------------------
status_t ucmd_help() {
    uint8_t i;
    usart_put_str("\n\rAvailable commands:\n\r");
    for(i = 0; i < UCMD_MAP_SIZE; i++) {
        if(UCMD_MAP[i].usage) {
            usart_printf("%s \t- %s\n\r \t  usage: %s\n\r", UCMD_MAP[i].name, UCMD_MAP[i].descr, UCMD_MAP[i].usage);
        } else {
            usart_printf("%s \t- %s\n\r", UCMD_MAP[i].name, UCMD_MAP[i].descr);
        }
    }
    return STATUS_SUCCESS;
}

status_t ucmd_show_version() {
    usart_printf("\n\r\n\r%s\n\rBoard: %s / %iHz\n\r\n\r", VESION_ID, BOARD_ID, (SystemCoreClock / 1000));
    return STATUS_SUCCESS;
}

status_t ucmd_core_halt() {
    usart_put_str("\n\r*** CPU halted ***\r\n");
    // disable irq
    __ASM volatile ("cpsid i" : : : "memory");
    globals.fl_cpu_halt = TRUE;
    while(1) __NOP();
}

status_t ucmd_reboot() {
    usart_put_str(CRLF);
    NVIC_SystemReset();
    return STATUS_SUCCESS;
}

status_t ucmd_set_led_delay(char *args) {
    uint16_t o = 0;
    if(!args) {
    return STATUS_INVALID_ARGUMENTS;
    }
    //
    o = globals.led_delay;
    globals.led_delay = atoi(args);
    usart_printf("flash: old=%i, new=%i\n\r", o, globals.led_delay);
    //
    return STATUS_SUCCESS;
}

status_t ucmd_echo(char *params) {
    char *args[CMD_MAX_ARGS];
    uint8_t argc, i;
    //
    argc = parse_args(params, (char **) &args, CMD_MAX_ARGS);
    if(argc > 0) {
    for(i=0; i < argc; i++) {
        usart_printf("echo: %u -> %s\n\r", i, args[i]);
    }
    }
    return STATUS_SUCCESS;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
// Handledrs
// ----------------------------------------------------------------------------------------------------------------------------------------
void SysTick_Handler(void) {
    globals.systick_usec++;
}

void USART1_IRQHandler(void) {
    char rdch = usart_get_char(TRUE);
    //
    if(globals.fl_usart_ignore_input) {
        return;
    }
    // backspace
    if(rdch == 0x8) {
	if(globals.cmd_buffer_pos > 0) {
	    usart_put_char(rdch);
	    usart_put_char(0x20);
	    usart_put_char(rdch);
            globals.cmd_buffer[globals.cmd_buffer_pos--] = NULL;
        }
        return;
    }
    // enter
    if(rdch == 0xD) {
        globals.fl_usart_buffer_ready = TRUE;
        return;
    }
    // echo and collect
    if(globals.cmd_buffer_pos + 1 >= CMD_BUFFER_SIZE) {
        globals.cmd_buffer_pos = 0;
        bzero((char *) globals.cmd_buffer, sizeof(globals.cmd_buffer));
    }
    globals.cmd_buffer[globals.cmd_buffer_pos++] = rdch;
    globals.cmd_buffer[globals.cmd_buffer_pos] = NULL;
    usart_put_char(rdch);
    //
    return;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------------------------------------------------------------------------------
int main() {
    ucmd_entry_t *ucmd;
    uint32_t led_timer = 0;
    status_t fn_err;
    //
    bzero(&globals, sizeof(globals));
    globals.led_delay = 100;
    globals.fl_usart_buffer_ready = FALSE;
    globals.fl_usart_ignore_input = FALSE;

    // init hal
    timers_init();
    common_pins_init();
    usart_init();

    //  show version
    ucmd_show_version();
    usart_put_str(CMD_PROMPT);

    while (1) {
        if(systick_asleep(led_timer, globals.led_delay)) {
            led_timer = systick_get();
            led_toggle(globals.fl_led_flash);
            globals.fl_led_flash = (globals.fl_led_flash == FALSE ? TRUE : FALSE);
        }
        if(globals.fl_usart_buffer_ready) {
            globals.fl_usart_ignore_input = TRUE;
            usart_put_str(CRLF);
            if(globals.cmd_buffer_pos < 1) {
                goto endnrm2;
            }
            ucmd = ucmd_lookup((char *) globals.cmd_buffer);
            if(ucmd) {
                if(!ucmd->fnc) {
                    usart_printf("%s: #%x (%s)\n\r", MSG_ERR, STATUS_NOT_IMPLEMENTED, status_get_description(STATUS_NOT_IMPLEMENTED));
                    goto endnrm;
                }
                if(ucmd->params) {
                    if(ucmd->usage) usart_printf("%s: %s\n\r", MSG_USAGE, ucmd->usage);
                    else usart_printf("%s: #%x (%s)\n\r", MSG_ERR, STATUS_INVALID_ARGUMENTS, status_get_description(STATUS_INVALID_ARGUMENTS));
                } else {
                    if((fn_err = ucmd->fnc(NULL)) != STATUS_SUCCESS) {
                        usart_printf("%s: #%x (%s)\n\r", MSG_ERR, fn_err, status_get_description(fn_err));
                    }
                }
            } else {
                char lcmd_name[CMD_NAME_LEN_MAX];
                char *bp = (char *) globals.cmd_buffer;
                uint16_t pos = 0;
                //
                for(pos = 0; pos <= globals.cmd_buffer_pos; pos++) {
                    if(globals.cmd_buffer[pos] == 0x20) break;
                }
                if(pos >= globals.cmd_buffer_pos) {
                    usart_printf("%s: #%x (%s)\n\r", MSG_ERR, STATUS_UNKNOWN_COMMAND, status_get_description(STATUS_UNKNOWN_COMMAND));
                    goto endnrm;
                } else  if(pos >= CMD_NAME_LEN_MAX) {
                    usart_printf("%s: #%x (%s)\n\r", MSG_ERR, STATUS_COMMAND_TOO_LONG, status_get_description(STATUS_COMMAND_TOO_LONG));
                    goto endnrm;
                }
                // copy cmd name
                bzero((char  *)lcmd_name, sizeof(lcmd_name));
                bcopy(bp, (char *)lcmd_name, pos);
                bp += (pos + 1);
                ucmd = ucmd_lookup((char *) lcmd_name);
                if(!ucmd) {
                    usart_printf("%s: #%x (%s)\n\r", MSG_ERR, STATUS_UNKNOWN_COMMAND, status_get_description(STATUS_UNKNOWN_COMMAND));
                    goto endnrm;
                }
                if(!ucmd->fnc) {
                    usart_printf("%s: #%x (%s)\n\r", MSG_ERR, STATUS_NOT_IMPLEMENTED, status_get_description(STATUS_NOT_IMPLEMENTED));
                    goto endnrm;
                }
                if((fn_err = ucmd->fnc(bp)) != STATUS_SUCCESS) {
                    usart_printf("%s: #%x (%s)\n\r", MSG_ERR, fn_err, status_get_description(fn_err));
                }
            }
            // clear buffer
            endnrm:
            globals.cmd_buffer_pos = 0;
            bzero((char *) globals.cmd_buffer, sizeof(globals.cmd_buffer));
            endnrm2:
            usart_put_str(CMD_PROMPT);
            globals.fl_usart_ignore_input = FALSE;
            globals.fl_usart_buffer_ready = FALSE;
        }
    }
}
