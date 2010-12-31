#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "bmplib.h"

void SaveToFile(const char *FileName, uint8_t *buffer, uint32_t size)
{
    FILE *FHandle = fopen(FileName, "wb+");
    fwrite(buffer, 1, size, FHandle);
    fclose(FHandle);
}

void GetBitmap(uint8_t **outBuffer, uint32_t * outsize)
{
    BMP Bitmap;

    Bitmap.Width = 128;
    Bitmap.Height = 128;
    Bitmap.BitCount = 8;
    Bitmap.ColorCount = (uint32_t) pow(2.0, (int) Bitmap.BitCount);
    Bitmap.AndBmp = false;

    if (CreateBitmap(&Bitmap)) {
        printf("CreateBitmap returned error\n");
        return;
    }

    if (Bitmap.BitCount <= 8)
        for (int p = 0; p < Bitmap.ColorCount; p++)
            SetPalette(&Bitmap, p, RGB(rand() % 0x100,
                                       rand() % 0x100,
                                       rand() % 0x100));

    for (int i = 1; i <= Bitmap.Width ; i++ )
        for (int j = 1 ; j <= Bitmap.Height ; j++) {
            SetPixel(&Bitmap, i, j, rand());
            MaskBitmap(&Bitmap, i, j);
        }

    CreateBMPFile(&Bitmap, outBuffer, outsize);
    CloseBitmap(&Bitmap);
}

int main(int argc, char *argv[])
{
    uint8_t *buffer;
    uint32_t size;

    srand(time(NULL));

    GetBitmap(&buffer, &size);

    SaveToFile("out.bmp", buffer, size);
    return 0;
}
