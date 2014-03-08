#include "helpers.h"

/*..........................................................................*/
char const *bin2dec3(uint32_t val) {
    static char str[] = "DDD";
    str[2] = '0' + (val % 10);
    val /= 10;
    str[1] = '0' + (val % 10);
    val /= 10;
    str[0] = '0' + (val % 10);
    if (str[0] == '0') {
         str[0] = ' ';
         if (str[1] == '0') {
             str[1] = ' ';
         }
    }
    return str;
}
/*..........................................................................*/
char const *ticks2min_sec(uint32_t val) {
    static char str[] = "MM:SS";
    uint32_t tmp;

    val /= BSP_TICKS_PER_SEC;                     /* convert val to seconds */
    tmp = val / 60;                                    /* tmp holds minutes */
    str[1] = '0' + (tmp % 10);
    str[0] = '0' + (tmp / 10);
    val -= tmp * 60;                                    /* subtract minutes */
    str[4] = '0' + (val % 10);
    str[3] = '0' + (val / 10);
    if (str[0] == '0') {
         str[0] = ' ';
    }
    return str;
}
