#ifndef __ID_PM__
#define __ID_PM__

//static const uint32_t VSWAP_LEN = 1545400;
static const byte *vswap = (byte *) VSWAP_ADDR;

#define PMPageSize 4096

extern word ChunksInFile;
extern word PMSpriteStart;
extern word PMSoundStart;

extern bool PMSoundInfoPagePadded;

// ChunksInFile+1 pointers to page starts.
// The last pointer points one byte after the last page.

extern longword *pageOffsets;
extern word *pageLengths;

void PM_Startup();
void PM_Shutdown();

static inline uint32_t PM_GetPageSize(int page)
{
    if(page < 0 || page >= ChunksInFile)
        Quit("PM_GetPageSize: Tried to access illegal page: %i", page);

    return pageLengths[page];
}

static inline uint8_t *PM_GetPage(int page)
{
    if(page < 0 || page >= ChunksInFile)
        Quit("PM_GetPage: Tried to access illegal page: %i", page);

    return (uint8_t *) &vswap[pageOffsets[page]];
}

static inline uint8_t *PM_GetEnd()
{
    return (uint8_t *) &vswap[VSWAP_LEN];
}

static inline byte *PM_GetTexture(int wallpic)
{
    return PM_GetPage(wallpic);
}

static inline uint16_t *PM_GetSprite(int shapenum)
{
    // correct alignment is enforced by PM_Startup()
    return (uint16_t *) (void *) PM_GetPage(PMSpriteStart + shapenum);
}

static inline byte *PM_GetSound(int soundpagenum)
{
    return PM_GetPage(PMSoundStart + soundpagenum);
}

#endif
