#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

extern uint8_t wolf_signon[], spear_signon[];

void VL_MungePic (uint8_t *source, unsigned width, unsigned height)
{
    unsigned x,y,plane,size,pwidth;
    uint8_t *temp, *dest, *srcline;

    size = width*height;

    temp=(uint8_t *) malloc(size);
    memcpy (temp,source,size);

    dest = source;
    pwidth = width/4;

    for (plane=0;plane<4;plane++)
    {
        srcline = temp;
        for (y=0;y<height;y++)
        {
            for (x=0;x<pwidth;x++)
                *dest++ = *(srcline+x*4+plane);
            srcline+=width;
        }
    }

    free(temp);
}

int main(void)
{
    VL_MungePic(wolf_signon, 320, 200);
    VL_MungePic(spear_signon, 320, 200);

    int i;

    printf("#include <wl_def.h>\n\n");

    printf("#ifndef SPEAR\n");
    printf("byte signon[] = {\n");
    i = 0;
    while (i < 320*200)
    {
        printf("    ");
        for (int j = 0; j < 16; j++)
            printf("0x%02X,", wolf_signon[i + j]);

        if (i + 16 < 320*200)
            printf("\n");

        i += 16;
    }
    printf("\b};\n");

    printf("#else\n");
    printf("byte signon[] = {\n");
    i = 0;
    while (i < 320*200)
    {
        printf("    ");
        for (int j = 0; j < 16; j++)
            printf("0x%02X,", spear_signon[i + j]);

        if (i + 16 < 320*200)
            printf("\n");

        i += 16;
    }
    printf("\b};\n");

    printf("#endif\n");

    return 0;
}
