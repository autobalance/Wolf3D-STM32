/*
    Use USART2 to interface with a PS2 keyboard. Note that this is
    experimental as setup involves auto baud-rate detection to avoid
    using the synchronous clock pin of a PS2 keyboard. This means only
    key reception is supported, but on the plus side, everything is taken
    care of using only the USART2 peripheral and an interrupt to capture
    keys. May not work with all KBs, of course.
*/

#ifndef __PS2_KB_H__
#define __PS2_KB_H__

#include <stm32h7xx.h>

void ps2_kb_setup(void);

void ps2_kb_set_baud_rate(void);

#endif
