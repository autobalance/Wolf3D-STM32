/*
    LTDC peripheral driver to generate a timing-compatible VGA signal. Use
    an external DAC (e.g. R2R dac) to create the VGA signal. Currently uses
    18 pins (16 for RGB565 colour and 2 for VSYNC and HSYNC), see mapping below.
*/

#ifndef __LTDC_H__
#define __LTDC_H__

#include <stm32h7xx.h>

#define FRAMEBUF_WIDTH 320
#define FRAMEBUF_HEIGHT 200

// The line counts/pixels-per-line for a signal output of 640x400@70Hz (like the original Wolf3D used).
// Note that the framebuffer is half the size in both dimensions, so this effectively
// creates a boxed output in the upper left corner based on the setup in 'ltdc_setup'.
// Unfortunately, 320x200@70Hz doesn't exist in VGA; halving these line counts/pixels-per-line
// (and cutting by a quarter the LTDC clock in 'rcc.h'!) kind-of works on a monitor I used,
// but this won't be guaranteed. These values were retrieved from:
//      http://tinyvga.com/vga-timing/640x400@70Hz

// NOTE: current line output is for 320x240@60Hz, a non-VGA signal
// pixel counts
#define HORI_SYNC_PULSE   (96/2)
#define HORI_BACK_PORCH   (48/2)
#define HORI_VISIBLE_AREA (640/2)
#define HORI_FRONT_PORCH  (16/2)

// line counts
#define VERT_SYNC_PULSE   (2/2)
#define VERT_BACK_PORCH   (33/2)
#define VERT_VISIBLE_AREA (480/2)
#define VERT_FRONT_PORCH  (10/2)


// constants for the LTDC configuration registers, noting the successive registers
// are cumulative in counts

// sync-size length
#define SS_HORI (HORI_SYNC_PULSE)
#define SS_VERT (VERT_SYNC_PULSE)

// back-porch length
#define BP_HORI (SS_HORI + HORI_BACK_PORCH)
#define BP_VERT (SS_VERT + VERT_BACK_PORCH)

// active-width length
#define AW_HORI (BP_HORI + HORI_VISIBLE_AREA)
#define AW_VERT (BP_VERT + VERT_VISIBLE_AREA)

// total-width length
#define TW_HORI (AW_HORI + HORI_FRONT_PORCH)
#define TW_VERT (AW_VERT + VERT_FRONT_PORCH)


// constants for the start and end point of drawing lines/pixels, which is essentially
// the sum of the sync pulse and back porch lengths for the start, and the end of the
// framebuffer height/width in line/pixel count for the end count

// window horizontal position
#define WHP_START (HORI_SYNC_PULSE + HORI_BACK_PORCH)
#define WHP_STOP  (WHP_START + FRAMEBUF_WIDTH)

// window vertical position
#define WVP_START (VERT_SYNC_PULSE + VERT_BACK_PORCH)
#define WVP_STOP  (WVP_START + FRAMEBUF_HEIGHT)

/*
    RGB565 output:

    VSYNC: PA4, AF14
    HSYNC: PC6, AF14

    R7: PE15, AF14
    R6: PA8,  AF14
    R5: PA12, AF14
    R4: PA11, AF14
    R3: PB0,  AF9

    G7: PD3,  AF14
    G6: PC7,  AF14
    G5: PB11, AF14
    G4: PB10, AF14
    G3: PE11, AF14
    G2: PA6,  AF14

    B7: PB9,  AF14
    B6: PB8,  AF14
    B5: PA3,  AF14
    B4: PE12, AF14
    B3: PD10, AF14

    AF9 : PB{0}
    AF14: PA{3,4,6,8,11,12}, PB{8,9,10,11}, PC{6,7}, PD{3,10}, PE{11,12,15}
*/
void ltdc_setup(void);

void ltdc_wait_for_vsync(void);

void ltdc_set_clut(uint8_t *rgba);

// draw an intro screen telling user to press the space key
// (to aid in generating the keyboard USART peripheral baud rate)
void ltdc_draw_intro(void);

#endif
