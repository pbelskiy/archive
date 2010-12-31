#if !defined(__BMP_LIB_H__)
#define __BMP_LIB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define RGB(red, green, blue)   ((blue << 0) | (green << 8) | (red << 16) ) & 0x00FFFFFF

typedef struct _BITMAP {
    uint8_t *Buffer;
    uint32_t Size;
    uint32_t Width;
    uint32_t Height;
    uint32_t ColorCount;
    uint32_t BitCount;
    bool AndBmp;
} BMP, *pBMP;

uint32_t CreateBitmap(pBMP Bitmap);
void CloseBitmap(pBMP Bitmap);

int SetPalette(pBMP Bitmap, uint32_t Index, uint32_t Color);

int SetPixel(pBMP Bitmap, int x, int y, uint32_t ColorIndex);
void MaskBitmap(pBMP Bitmap, int x, int y);

void DrawCircle(pBMP Bitmap, int x0, int y0, int radius, uint32_t Color);
void DrawLine(pBMP Bitmap, int x1, int y1, int x2, int y2, uint32_t ColorIndex);

int CreateBMPFile(pBMP Bitmap, uint8_t **Buffer, uint32_t * size);
int CreateICO(pBMP Bitmap, uint8_t **Buffer, uint32_t * size);

pBMP ConvertFileToBitmap(uint8_t *Buffer, uint32_t size);
#endif
