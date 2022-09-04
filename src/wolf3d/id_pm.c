#include "wl_def.h"

int ChunksInFile;
int PMSpriteStart;
int PMSoundStart;

bool PMSoundInfoPagePadded = false;

// holds the whole VSWAP
//uint32_t *PMPageData;
//size_t PMPageDataSize;

// ChunksInFile+1 pointers to page starts.
// The last pointer points one byte after the last page.
//uint8_t **PMPages;

//uint8_t *PMPage;
uint32_t* pageOffsets;
word *pageLengths;

PMPage PMPages[MAX_PAGES_IN_MEM];
PMPage *mru, *lru;
PMPage **page_lut;

FILE *vswap_file;

void PM_Startup()
{
    char fname[13] = "vswap.";
    strcat(fname,extension);

    vswap_file = fopen(fname, "rb");
    if(!vswap_file)
        CA_CannotOpen(fname);

    ChunksInFile = 0;
    fread(&ChunksInFile, sizeof(word), 1, vswap_file);
    PMSpriteStart = 0;
    fread(&PMSpriteStart, sizeof(word), 1, vswap_file);
    PMSoundStart = 0;
    fread(&PMSoundStart, sizeof(word), 1, vswap_file);

    pageOffsets = (uint32_t *) malloc((ChunksInFile + 1) * sizeof(int32_t));
    CHECKMALLOCRESULT(pageOffsets);
    fread(pageOffsets, sizeof(uint32_t), ChunksInFile, vswap_file);

    pageLengths = (word *) malloc(ChunksInFile * sizeof(word));
    CHECKMALLOCRESULT(pageLengths);
    fread(pageLengths, sizeof(word), ChunksInFile, vswap_file);

    fseek(vswap_file, 0, SEEK_END);
    long fileSize = ftell(vswap_file);
    long pageDataSize = fileSize - pageOffsets[0];
    if(pageDataSize > (size_t) -1)
        Quit("The page file \"%s\" is too large!", fname);

    pageOffsets[ChunksInFile] = fileSize;

    uint32_t dataStart = pageOffsets[0];
    int i;

    // Check that all pageOffsets are valid
    for(i = 0; i < ChunksInFile; i++)
    {
        if(!pageOffsets[i]) continue;   // sparse page
        if(pageOffsets[i] < dataStart || pageOffsets[i] >= (size_t) fileSize)
            Quit("Illegal page offset for page %i: %u (filesize: %u)",
                    i, pageOffsets[i], fileSize);
    }

    // Calculate total amount of padding needed for sprites and sound info page
    int alignPadding = 0;
    for(i = PMSpriteStart; i < PMSoundStart; i++)
    {
        if(!pageOffsets[i]) continue;   // sparse page
        uint32_t offs = pageOffsets[i] - dataStart + alignPadding;
        if(offs & 1)
            alignPadding++;
    }

    if((pageOffsets[ChunksInFile - 1] - dataStart + alignPadding) & 1)
        alignPadding++;

    for (int i = 0; i < MAX_PAGES_IN_MEM; i++)
    {
        PMPages[i].page = -1;
        PMPages[i].p_page = NULL;
        PMPages[i].p_prev = &PMPages[(i + MAX_PAGES_IN_MEM - 1) % MAX_PAGES_IN_MEM];
        PMPages[i].p_next = &PMPages[(i + 1) % MAX_PAGES_IN_MEM];
    }

    mru = &PMPages[0];
    lru = &PMPages[MAX_PAGES_IN_MEM-1];

    page_lut = (PMPage **) malloc(ChunksInFile * sizeof(PMPage *));
    CHECKMALLOCRESULT(page_lut);

    for (int i = 0; i < ChunksInFile; i++)
        page_lut[i] = NULL;

    /*PMPageDataSize = (size_t) pageDataSize + alignPadding;
    PMPageData = (uint32_t *) malloc(PMPageDataSize);
    CHECKMALLOCRESULT(PMPageData);

    PMPages = (uint8_t **) malloc((ChunksInFile + 1) * sizeof(uint8_t *));
    CHECKMALLOCRESULT(PMPages);

    // Load pages and initialize PMPages pointers
    uint8_t *ptr = (uint8_t *) PMPageData;
    for(i = 0; i < ChunksInFile; i++)
    {
        if(i >= PMSpriteStart && i < PMSoundStart || i == ChunksInFile - 1)
        {
            size_t offs = ptr - (uint8_t *) PMPageData;

            // pad with zeros to make it 2-byte aligned
            if(offs & 1)
            {
                *ptr++ = 0;
                if(i == ChunksInFile - 1) PMSoundInfoPagePadded = true;
            }
        }

        PMPages[i] = ptr;

        if(!pageOffsets[i])
            continue;               // sparse page

        // Use specified page length, when next page is sparse page.
        // Otherwise, calculate size from the offset difference between this and the next page.
        uint32_t size;
        if(!pageOffsets[i + 1]) size = pageLengths[i];
        else size = pageOffsets[i + 1] - pageOffsets[i];

        //fseek(file,pageOffsets[i],SEEK_SET);
        //fread(ptr, 1, size, file);
        ptr += size;
    }

    // last page points after page buffer
    PMPages[ChunksInFile] = (uint8_t *) 0xFFFFFFFF;//ptr;

    free(pageLengths);
    free(pageOffsets);
    fclose(vswap_file);*/
}

void PM_Shutdown()
{
    free(pageLengths);
    free(pageOffsets);
    fclose(vswap_file);
    //free(PMPages);
    //free(PMPageData);
}
