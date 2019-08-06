/**
 * part of stdlib
 *
 **/
#include "stshell.h"
#include "ctype.h"

/**
 * bzero - clear memory regions of arbitary length
 **/
void bzero(void *tov, uint16_t len) {
    uint8_t *to = tov;
    while (len-- > 0) {
        *to++ = NULL;
    }
}

/**
 * bcopy - copy memory regions of arbitary length
 **/
void bcopy(const void *src, void *dest, uint16_t len) {
    if (dest < src) {
        const uint8_t *firsts = (const uint8_t *) src;
        uint8_t *firstd = (uint8_t *) dest;
        while (len--) {
            *firstd++ = *firsts++;
        }
    } else {
        const uint8_t *lasts = (const uint8_t *)src + (len-1);
        uint8_t *lastd = (uint8_t *)dest + (len-1);
        while (len--) {
            *lastd-- = *lasts--;
        }
    }
}

// -------------------------------------------------------------------------------------------------------------------------------
// strings
// -------------------------------------------------------------------------------------------------------------------------------
uint16_t strlen(const char *s) {
    const char *ret = s;
    while (*s++);

    return(s - 1 - ret);
}

uint8_t strcmp(const char *s1, const char *s2) {
    while (*s1 == *s2) {
        if (*s1 == NULL) {
            return 0;
        }
        s1++;
        s2++;
    }
    return *(unsigned const char *) s1 - *(unsigned const char *) (s2);
}

char *strchr(const char *s, uint8_t c) {
    while (1) {
        if (*s == c) return (char*)s;
        if (*s == 0) return 0;
        s++;
    }
}

// -------------------------------------------------------------------------------------------------------------------------------
// other
// -------------------------------------------------------------------------------------------------------------------------------
int atoi(const char *str) {
        int n = 0;
        while (isdigit(*str)) {
            n = (n * 10) + *str - '0';
            str++;
        }
        return n;
}

int atoi2(const char *str){
        int i,n,sign;
	// skip wspaces and tabs
        for (i=0; (str[i] == 0x20) | (str[i] == 0x09) | (str[i] == 0x0A); ++i);
	//
        sign = 1;
        switch(str[i]){
    	    case '-': sign = -1;
    	    case '+': ++i; 
	    break;
        }
        for(n = 0; isdigit(str[i]); ++i) {
	    n = (10 * n) + str[i] - '0';
	}
        return(sign * n);
}

uint8_t parse_args(char *p, char **args_ret, uint8_t args_size) {
    char **args = args_ret;
    uint8_t argc = 0;
    //
    if(*p == NULL) return 0;
    //
    args[argc] = p;  argc++;
    while (*p++) {
        if(*p == 0x20) {
            *p = 0x0; p++;
            if(*p == NULL) break;
            args[argc]=p; argc++;
            if(argc >= args_size) break;
        }
    }
    return argc;
}
