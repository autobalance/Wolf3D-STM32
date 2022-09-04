#include <stdio.h>
#include <stdint.h>

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} pixcolor;

#define RGB(r, g, b) {(r)*255/63, (g)*255/63, (b)*255/63, 0}

pixcolor gamepal_spear[]={
    #include "../sodpal.inc"
};
pixcolor gamepal_wolf[]={
    #include "../wolfpal.inc"
};

#define NUMREDSHIFTS    6
#define REDSTEPS        8

#define NUMWHITESHIFTS  3
#define WHITESTEPS      20
#define WHITETICS       6

pixcolor redshifts[NUMREDSHIFTS][256];
pixcolor whiteshifts[NUMWHITESHIFTS][256];

void InitRedShifts (pixcolor gamepal[])
{
    pixcolor *workptr, *baseptr;
    int i, j, delta;

    for (i = 1; i <= NUMREDSHIFTS; i++)
    {
        workptr = redshifts[i - 1];
        baseptr = gamepal;

        for (j = 0; j <= 255; j++)
        {
            delta = 256 - baseptr->r;
            workptr->r = baseptr->r + delta * i / REDSTEPS;
            delta = -baseptr->g;
            workptr->g = baseptr->g + delta * i / REDSTEPS;
            delta = -baseptr->b;
            workptr->b = baseptr->b + delta * i / REDSTEPS;
            baseptr++;
            workptr++;
        }
    }

    for (i = 1; i <= NUMWHITESHIFTS; i++)
    {
        workptr = whiteshifts[i - 1];
        baseptr = gamepal;

        for (j = 0; j <= 255; j++)
        {
            delta = 256 - baseptr->r;
            workptr->r = baseptr->r + delta * i / WHITESTEPS;
            delta = 248 - baseptr->g;
            workptr->g = baseptr->g + delta * i / WHITESTEPS;
            delta = 0-baseptr->b;
            workptr->b = baseptr->b + delta * i / WHITESTEPS;
            baseptr++;
            workptr++;
        }
    }
}

void gen_shiftpal(pixcolor gamepal[], char *redfname, char *whitefname)
{
    char buf[256];

    InitRedShifts(gamepal);

    FILE *rs_fd = fopen(redfname, "w");

    for (int i = 0; i < NUMREDSHIFTS; i++)
    {
        fputs("{\n", rs_fd);
        for (int j = 0; j < 256; j+=5)
        {
            fputs("    ", rs_fd);
            for (int k = j; k < (j + 5 < 256 ? j + 5 : 256); k++)
            {
                int n_write;
                if (k == 255)
                    n_write = snprintf(buf, 256, "{%3d,%3d,%3d,  0}", redshifts[i][k].r,
                                                                      redshifts[i][k].g,
                                                                      redshifts[i][k].b);
                else
                    n_write = snprintf(buf, 256, "{%3d,%3d,%3d,  0},", redshifts[i][k].r,
                                                                       redshifts[i][k].g,
                                                                       redshifts[i][k].b);

                fwrite(buf, 1, n_write, rs_fd);
            }
            fputs("\n", rs_fd);
        }
        if (i == NUMREDSHIFTS - 1)
            fputs("}\n", rs_fd);
        else
            fputs("},\n", rs_fd);
    }

    fclose(rs_fd);


    FILE *ws_fd = fopen(whitefname, "w");

    for (int i = 0; i < NUMWHITESHIFTS; i++)
    {
        fputs("{\n", ws_fd);
        for (int j = 0; j < 256; j+=5)
        {
            fputs("    ", ws_fd);
            for (int k = j; k < (j + 5 < 256 ? j + 5 : 256); k++)
            {
                int n_write;
                if (k == 255)
                    n_write = snprintf(buf, 256, "{%3d,%3d,%3d,  0}", whiteshifts[i][k].r,
                                                                      whiteshifts[i][k].g,
                                                                      whiteshifts[i][k].b);
                else
                    n_write = snprintf(buf, 256, "{%3d,%3d,%3d,  0},", whiteshifts[i][k].r,
                                                                       whiteshifts[i][k].g,
                                                                       whiteshifts[i][k].b);

                fwrite(buf, 1, n_write, ws_fd);
            }
            fputs("\n", ws_fd);
        }
        if (i == NUMWHITESHIFTS - 1)
            fputs("}\n", ws_fd);
        else
            fputs("},\n", ws_fd);
    }

    fclose(ws_fd);
}

int main(void)
{
    gen_shiftpal(gamepal_spear, "sodredpal.inc", "sodwhitepal.inc");
    gen_shiftpal(gamepal_wolf, "wolfredpal.inc", "wolfwhitepal.inc");

    return 0;
}
