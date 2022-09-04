// ID_VL.H

// wolf compatability

//#ifndef ID_VL.H 
//#define ID_VL.H
#ifndef ID_VL_H 
#define ID_VL_H

#include "wl_def.h"

void Quit (const char *error,...);

//===========================================================================

#define CHARWIDTH       2
#define TILEWIDTH       4

//===========================================================================

extern scrsurf *screen, *screenBuffer;

extern  boolean  fullscreen, usedoublebuffering;
extern  unsigned screenWidth, screenHeight, screenBits, screenPitch, bufferPitch;
extern  unsigned scaleFactor;

extern  boolean  screenfaded;
extern  unsigned bordercolor;

extern const pixcolor gamepal[256];
extern uint32_t curpal_lut[256];

//===========================================================================

//
// VGA hardware routines
//

#define VL_WaitVBL(a) delay_ms((a)*8)

void VL_SetVGAPlaneMode (void);
void VL_SetTextMode (void);
void VL_Shutdown (void);

void VL_ConvertPalette(byte *srcpal, pixcolor *destpal, int numColors);
void VL_FillPalette (int red, int green, int blue);
void VL_SetColor    (int color, int red, int green, int blue);
void VL_GetColor    (int color, int *red, int *green, int *blue);
void VL_SetPalette  (const pixcolor *palette, bool forceupdate);
void VL_GetPalette  (pixcolor *palette);
void VL_FadeOut     (int start, int end, int red, int green, int blue, int steps);
void VL_FadeIn      (int start, int end, const pixcolor *palette, int steps);

byte VL_GetPixel        (int x, int y);
void VL_Plot            (int x, int y, int color);
void VL_Hlin            (unsigned x, unsigned y, unsigned width, int color);
void VL_Vlin            (int x, int y, int height, int color);
void VL_BarScaledCoord  (int scx, int scy, int scwidth, int scheight, int color);
void VL_Bar      (int x, int y, int width, int height, int color);
void VL_ClearScreen(int color);

void VL_MungePic                (byte *source, unsigned width, unsigned height);
void VL_DrawPicBare             (int x, int y, byte *pic, int width, int height);
void VL_MemToLatch              (byte *source, int width, int height,
                                    scrsurf *destSurface, int x, int y);
void VL_ScreenToScreen          (scrsurf *source, scrsurf *dest);
void VL_MemToScreenScaledCoordOrig  (byte *source, int width, int height, int scx, int scy);
void VL_MemToScreenScaledCoord  (byte *source, int origwidth, int origheight, int srcx, int srcy,
                                    int destx, int desty, int width, int height);

void VL_MemToScreen (byte *source, int width, int height, int x, int y);

void VL_MaskedToScreen (byte *source, int width, int height, int x, int y);

void VL_LatchToScreenScaledCoord (scrsurf *source, int xsrc, int ysrc,
    int width, int height, int scxdest, int scydest);

void VL_LatchToScreen (scrsurf *source, int xsrc, int ysrc,
    int width, int height, int xdest, int ydest);
void VL_LatchToScreenScaledCoordOrig (scrsurf *source, int scx, int scy);
void VL_LatchToScreenOrig (scrsurf *source, int x, int y);

#endif
