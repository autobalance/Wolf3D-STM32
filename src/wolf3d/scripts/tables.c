#include <stdio.h>
#include <stdint.h>
#include <math.h>

typedef int32_t fixed;

#define PI 3.14159265358979323846
#define GLOBAL1         (1l<<16)

#define FINEANGLES      3600
#define ANGLES          360
#define ANGLEQUAD       (ANGLES/4)

int32_t finetangent[FINEANGLES/4];
fixed sintable[ANGLES+ANGLES/4];

const float radtoint = (float)(FINEANGLES/2/PI);

void BuildTables (void)
{
    int i;
    for(i=0;i<FINEANGLES/8;i++)
    {
        double tang=tan((i+0.5)/radtoint);
        finetangent[i]=(int32_t)(tang*GLOBAL1);
        finetangent[FINEANGLES/4-1-i]=(int32_t)((1/tang)*GLOBAL1);
    }

    float angle=0;
    float anglestep=(float)(PI/2/ANGLEQUAD);
    for(i=0; i<ANGLEQUAD; i++)
    {
        fixed value=(int32_t)(GLOBAL1*sin(angle));
        sintable[i]=sintable[i+ANGLES]=sintable[ANGLES/2-i]=value;
        sintable[ANGLES-i]=sintable[ANGLES/2+i]=-value;
        angle+=anglestep;
    }
    sintable[ANGLEQUAD] = 65536;
    sintable[3*ANGLEQUAD] = -65536;

}

int main(void)
{
    BuildTables();

    FILE *drawh_fd = fopen("wl_draw.h", "w");

    char buf[256];

    fputs("const int32_t finetangent[FINEANGLES/4] =\n", drawh_fd);
    fputs("{\n", drawh_fd);
    for (int i = 0; i < FINEANGLES/4; i+=8)
    {
        fputs("    ", drawh_fd);
        for (int j = i; j < (i+8 < FINEANGLES/4 ? i+8 : FINEANGLES/4); j++)
        {
            int n_write;
            if (j == FINEANGLES/4 - 1)
                n_write = snprintf(buf, 256, "0x%08X", finetangent[j]);
            else
                n_write = snprintf(buf, 256, "0x%08X,", finetangent[j]);

            fwrite(buf, 1, n_write, drawh_fd);
        }
        fputs("\n", drawh_fd);
    }
    fputs("};\n\n", drawh_fd);

    fputs("const fixed sintable[ANGLES+ANGLES/4] =\n", drawh_fd);
    fputs("{\n", drawh_fd);
    for (int i = 0; i < ANGLES+ANGLES/4; i+=8)
    {
        fputs("    ", drawh_fd);
        for (int j = i; j < (i+8 < (ANGLES+ANGLES/4) ? i+8 : (ANGLES+ANGLES/4)); j++)
        {
            int n_write;
            if (j == ANGLES+ANGLES/4 - 1)
                n_write = snprintf(buf, 256, "0x%08X", sintable[j]);
            else
                n_write = snprintf(buf, 256, "0x%08X,", sintable[j]);

            fwrite(buf, 1, n_write, drawh_fd);
        }
        fputs("\n", drawh_fd);
    }
    fputs("};\n\n", drawh_fd);

    fputs("const fixed *costable = sintable+(ANGLES/4);\n", drawh_fd);

    fclose(drawh_fd);

    return 0;
}
