#ifndef __ID_PM__
#define __ID_PM__


#define PMPageSize 4096

extern int ChunksInFile;
extern int PMSpriteStart;
extern int PMSoundStart;

extern bool PMSoundInfoPagePadded;

// ChunksInFile+1 pointers to page starts.
// The last pointer points one byte after the last page.

extern uint32_t* pageOffsets;
extern word *pageLengths;

extern FILE *vswap_file;

void PM_Startup();
void PM_Shutdown();

static inline uint32_t PM_GetPageSize(int page)
{
    if(page < 0 || page >= ChunksInFile)
        Quit("PM_GetPageSize: Tried to access illegal page: %i", page);
    //return (uint32_t) (PMPages[page + 1] - PMPages[page]);

    return pageLengths[page];
}

typedef struct PMPage
{
    int page;
    uint8_t *p_page;
    struct PMPage *p_prev;
    struct PMPage *p_next;
} PMPage;

#define MAX_PAGES_IN_MEM 32
extern PMPage PMPages[MAX_PAGES_IN_MEM];
extern PMPage *mru, *lru;
extern PMPage **page_lut;

static PMPage* PM_PageInMem(int page)
{
    return page_lut[page];
}

static void PM_SetMRU(PMPage *p_page)
{
    if (p_page == mru)
        return;

    p_page->p_prev->p_next = p_page->p_next;
    p_page->p_prev = lru;
    p_page->p_next = mru;
    mru->p_prev = p_page;

    mru = p_page;
}

static void PM_SwapPageInMem(int page)
{
    if (lru->p_page != NULL)
    {
        page_lut[lru->page] = NULL;
        free(lru->p_page);
    }

    uint32_t size = PM_GetPageSize(page);

    lru->page = page;
    lru->p_page = (uint8_t *) malloc(size);

    PMPage *p_tmp = lru;
    lru = lru->p_prev;
    mru = p_tmp;

    page_lut[page] = mru;

    fseek(vswap_file, pageOffsets[page], SEEK_SET);
    fread(mru->p_page, 1, size, vswap_file);
}

static inline uint8_t *PM_GetPage(int page)
{
    if(page < 0 || page >= ChunksInFile)
        Quit("PM_GetPage: Tried to access illegal page: %i", page);

    //return PMPages[page];

    PMPage *p_page;
    if ((p_page = PM_PageInMem(page)))
    {
        PM_SetMRU(p_page);
    }
    else
    {
        PM_SwapPageInMem(page);
    }

    return mru->p_page;
}

static inline uint8_t *PM_GetEnd()
{
    return 0xffffffff;//PMPages[ChunksInFile];
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

static uint8_t PM_SndPage[4096];

static inline byte *PM_GetSound(int soundpagenum)
{
    //return PM_GetPage(PMSoundStart + soundpagenum);

    uint32_t size = PM_GetPageSize(PMSoundStart + soundpagenum);

    fseek(vswap_file, pageOffsets[PMSoundStart + soundpagenum], SEEK_SET);
    fread(PM_SndPage, 1, size, vswap_file);

    return PM_SndPage;
}

#endif
