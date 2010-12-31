#include "bmplib.h"

uint32_t CreateBitmap(pBMP Bitmap)
{
    uint32_t i, j, offset = 0;
    uint32_t sizeOfImage, XOR_size, AND_size;

    /* check for correct arguments */
    if (Bitmap->BitCount != 1 &&
        Bitmap->BitCount != 4 &&
        Bitmap->BitCount != 8 &&
        Bitmap->BitCount != 24 &&
        Bitmap->BitCount != 32)
        return 1;

    /* calculate size of color count */
    Bitmap->ColorCount = (uint32_t) pow(2.0, (int) Bitmap->BitCount);

    /* calculate size of XOR bitmap */
    XOR_size = (Bitmap->Width * Bitmap->BitCount > 32) ? (uint32_t) ceil((Bitmap->Width * Bitmap->BitCount) / 8.0) : 4;
    XOR_size = (XOR_size % 4 != 0) ? XOR_size += 4 - (XOR_size % 4) : XOR_size;

    /* calculate size of AND bitmap */
    AND_size = (Bitmap->Width > 32) ? (uint32_t) ceil(Bitmap->Width / 8.0) : 4;
    AND_size = (AND_size % 4 != 0) ? AND_size += 4 - (AND_size % 4) : AND_size;

    if (!Bitmap->AndBmp)
        AND_size = 0;

    /* calculate size of out complete bitmap, and allocate memory for it */
    if (Bitmap->BitCount != 24 && Bitmap->BitCount != 32) {
        sizeOfImage = (Bitmap->ColorCount * 4) +
                      (Bitmap->Height * XOR_size) +
                      (Bitmap->Height * AND_size);

        Bitmap->Size = 0x28 + sizeOfImage;
    } else {
        if (Bitmap->BitCount == 24)
            sizeOfImage = Bitmap->Height *
                         (Bitmap->Width * 3 + Bitmap->Width % 4) +
                         (Bitmap->Height * AND_size);

        if (Bitmap->BitCount == 32)
            sizeOfImage = Bitmap->Height *
                         (Bitmap->Width * 4) +
                         (Bitmap->Height * AND_size);

        Bitmap->Size = 0x28 + sizeOfImage;
    }

    Bitmap->Buffer = (uint8_t*) calloc(Bitmap->Size, sizeof(uint8_t));

    /* create header of bitmap */
    *(uint32_t *) &Bitmap->Buffer[offset + 0x00] = 0x28;              // size of InfoHeader structure = 40
    *(uint32_t *) &Bitmap->Buffer[offset + 0x04] = Bitmap->Width;
    *(uint32_t *) &Bitmap->Buffer[offset + 0x08] = (Bitmap->AndBmp) ? Bitmap->Height * 2 : Bitmap->Height;
    *(uint16_t*)  &Bitmap->Buffer[offset + 0x0C] = 1;                 // number of planes = 1
    *(uint16_t*)  &Bitmap->Buffer[offset + 0x0E] = Bitmap->BitCount;  // bits per pixel = 1, 4, 8
    *(uint32_t *) &Bitmap->Buffer[offset + 0x10] = 0;                 // Type of Compression = 0
    *(uint32_t *) &Bitmap->Buffer[offset + 0x14] = sizeOfImage;       // size of Image in uint8_ts = 0 (uncompressed)
    *(uint32_t *) &Bitmap->Buffer[offset + 0x18] = 0;                 // XpixelsPerM (unused = 0)
    *(uint32_t *) &Bitmap->Buffer[offset + 0x1C] = 0;                 // YpixelsPerM (unused = 0)
    *(uint32_t *) &Bitmap->Buffer[offset + 0x20] = 0;                 // ColorsUsed (unused = 0)
    *(uint32_t *) &Bitmap->Buffer[offset + 0x24] = 0;                 // ColorsImportant (unused = 0)
    offset += 0x28;

    /* color Map for XOR-Bitmap */
    if (Bitmap->BitCount != 24 && Bitmap->BitCount != 32)
        for (i = 0; i < Bitmap->ColorCount; i++) {
            Bitmap->Buffer[offset + 0] = 0x00; // blue
            Bitmap->Buffer[offset + 1] = 0x00; // green
            Bitmap->Buffer[offset + 2] = 0x00; // red
            Bitmap->Buffer[offset + 3] = 0x00; // alpha
            offset += 4;
        }

    /* XOR array (Bitmap line's) */
    if (Bitmap->BitCount != 24 && Bitmap->BitCount != 32)
        for (i = 0; i < Bitmap->Height; i++) {
            for (j = 0; j < XOR_size; j++) {
                Bitmap->Buffer[offset] = 0;
                offset++;
            }
        }

    /* XOR array (bitmap lines) */
    if (Bitmap->BitCount == 24 || Bitmap->BitCount == 32)
        for (i = 0; i < Bitmap->Height; i++) {
            for (j = 0; j < Bitmap->Width; j++) {
                Bitmap->Buffer[offset + 0] = 0; // blue
                Bitmap->Buffer[offset + 1] = 0; // green
                Bitmap->Buffer[offset + 2] = 0; // red

                if (Bitmap->BitCount == 32) {
                    Bitmap->Buffer[offset + 3] = 0; // ALPHA CHANNEL
                    offset += 4;
                } else {
                    offset += 3;
                }
            }

            /* alignment (padding uint8_ts) */
            if (Bitmap->BitCount == 24)
                offset += Bitmap->Width % 4;
        }

    /* AND array (mask for icon) */
    if (Bitmap->AndBmp) for (i = 0; i < Bitmap->Height; i++) {
            for (j = 0; j < AND_size; j++) {
                Bitmap->Buffer[offset] = 0;
                offset++;
            }
        }

    assert(offset == Bitmap->Size);
    return 0;
}

void CloseBitmap(pBMP Bitmap)
{
    if (Bitmap != NULL && Bitmap->Buffer != NULL) {
        free(Bitmap->Buffer);
        Bitmap->Buffer = NULL;
    }
}

int SetPalette(pBMP Bitmap, uint32_t Index, uint32_t Color)
{
    if (Bitmap->BitCount != 1 && Bitmap->BitCount != 4 && Bitmap->BitCount != 8)
        return 1;

    if (Index >= Bitmap->ColorCount)
        return 1;

    *(uint32_t*) &Bitmap->Buffer[0x28 + Index * 4] = Color;
    return 0;
}

int SetPixel(pBMP Bitmap, int x, int y, uint32_t ColorIndex)
{
    uint32_t XOR_size;

    if (x <= 0 || y <= 0 || x > Bitmap->Width || y > Bitmap->Height)
        return 1;

    /* calculate size of XOR bitmap */
    XOR_size = (Bitmap->Width * Bitmap->BitCount > 32) ? (uint32_t) ceil((Bitmap->Width * Bitmap->BitCount) / 8.0) : 4;
    XOR_size = (XOR_size % 4 != 0) ? XOR_size += 4 - (XOR_size % 4) : XOR_size;

    if (Bitmap->BitCount == 1) {
        uint32_t Shift = 7 - ((x - 1) % 8);
        uint32_t Left  = (uint32_t)  ceil(x / 8.0);
        uint32_t nX    = Left - 1;
        uint32_t nY    = (Bitmap->Height - y) * XOR_size;

        Bitmap->Buffer[(0x28 + Bitmap->ColorCount * 4) + nX + nY] |= ColorIndex << Shift;
    }

    if (Bitmap->BitCount == 4) {
        uint32_t Shift = (x % 2) ? 4 : 0;
        uint32_t Left  = (uint32_t)  ceil(x / 2.0);
        uint32_t nX    = Left - 1;
        uint32_t nY    = (Bitmap->Height - y) * XOR_size;

        Bitmap->Buffer[(0x28 + Bitmap->ColorCount * 4) + nX + nY] |= ColorIndex << Shift;
    }

    if (Bitmap->BitCount == 8) {
        uint32_t nX    = x - 1;
        uint32_t nY    = (Bitmap->Height - y) * XOR_size;

        Bitmap->Buffer[(0x28 + Bitmap->ColorCount * 4) + nX + nY] = ColorIndex;
    }

    if (Bitmap->BitCount == 24) {
        uint32_t nX    = (x - 1) * 3;
        uint32_t nY    = (Bitmap->Height - y) * (Bitmap->Height * 3 + Bitmap->Height % 4);

        Bitmap->Buffer[0x28 + nX + nY + 0] = 0; // blue
        Bitmap->Buffer[0x28 + nX + nY + 1] = 0; // green
        Bitmap->Buffer[0x28 + nX + nY + 2] = 0; // red
    }

    if (Bitmap->BitCount == 32) {
        uint32_t nX    = (x - 1) * 4;
        uint32_t nY    = (Bitmap->Height - y) * (Bitmap->Height * 4);

        Bitmap->Buffer[0x28 + nX + nY + 0] = 0; // blue
        Bitmap->Buffer[0x28 + nX + nY + 1] = 0; // green
        Bitmap->Buffer[0x28 + nX + nY + 2] = 0; // red
        Bitmap->Buffer[0x28 + nX + nY + 3] = 0; // alpha
    }

    return 0;
}

void MaskBitmap(pBMP Bitmap, int x, int y)
{
    uint32_t AND_Offset, AND_size;

    /* Checking for correct parametrs */
    if (Bitmap->AndBmp == false ||
        x == 0 ||
        y == 0 ||
        x > Bitmap->Width ||
        y > Bitmap->Height)
        return;

    /* Calculate size of AND bitmap */
    AND_size = (Bitmap->Width > 32) ? (uint32_t) ceil(Bitmap->Width / 8.0) : 4;
    AND_size = (AND_size % 4 != 0) ? AND_size += 4 - (AND_size % 4) : AND_size;
    AND_Offset = Bitmap->Size - (Bitmap->Height * AND_size);

    uint32_t Shift = 7 - ((x - 1) % 8);
    uint32_t Left  = (uint32_t)  ceil(x / 8.0);
    uint32_t nX    = Left - 1;
    uint32_t nY    = (Bitmap->Width - y) * AND_size;

    Bitmap->Buffer[AND_Offset + nX + nY] |= 1 << Shift;
}

pBMP OpenBitmap(uint8_t *Buffer, uint32_t size)
{
    pBMP Bitmap = (pBMP) malloc(sizeof(BMP));

    Bitmap->Width      = *(uint32_t*) &Buffer[0x04];
    Bitmap->Height     = *(uint32_t*) &Buffer[0x08];
    Bitmap->BitCount   = *(uint16_t*) &Buffer[0x0E];
    Bitmap->ColorCount = (uint32_t) pow(2.0, (int) Bitmap->BitCount);
    Bitmap->AndBmp     = false;
    Bitmap->Size       = size;
    Bitmap->Buffer     = (uint8_t*) malloc(Bitmap->Size);

    /* copy all data to bitmap buffer */
    memcpy(&Bitmap->Buffer[0], &Buffer[0], Bitmap->Size);
    return Bitmap;
}

int CreateBMPFile(pBMP Bitmap, uint8_t **oBuffer, uint32_t * osize)
{
    uint8_t *Buffer;
    uint32_t size;

    /* create header of BMP file */
    size = 0x0E + Bitmap->Size;
    Buffer = (uint8_t*) malloc(size);

    /* 'BM' Signature */
    *(uint16_t*) &Buffer[0x00] = 0x4D42;

    /* file size */
    *(uint32_t*) &Buffer[0x02] = size;

    /* reserved */
    *(uint32_t*) &Buffer[0x06] = 0;

    /* file offset to Raster Data */
    *(uint32_t*) &Buffer[0x0A] = 0x0E + 0x28 + Bitmap->ColorCount * 4;

    /* copy raster data */
    memcpy(&Buffer[0x0E], &Bitmap->Buffer[0], Bitmap->Size);

    *oBuffer = Buffer;
    *osize = size;
    return 0;
}

pBMP OpenBMPFile(const char *FilePath)
{
    uint8_t *Buffer;
    uint32_t Filesize;

    FILE *FHandle = fopen(FilePath, "rb");

    if (FHandle == NULL)
        return NULL;

    /* get size of file and read file to allocated memory */
    fseek(FHandle, 0, SEEK_END);
    Filesize = ftell(FHandle);
    fseek(FHandle, 0, SEEK_SET);

    Buffer = (uint8_t*) malloc(Filesize);
    fread(Buffer, 1, Filesize, FHandle);

    pBMP Bitmap = OpenBitmap(&Buffer[0x0E], Filesize - 0x0E);

    fclose(FHandle);
    return Bitmap;
}

void DrawLine(pBMP Bitmap, int x1, int y1, int x2, int y2, uint32_t ColorIndex)
{
    const int deltaX = abs(x2 - x1);
    const int deltaY = abs(y2 - y1);
    const int signX = x1 < x2 ? 1 : -1;
    const int signY = y1 < y2 ? 1 : -1;

    int error = deltaX - deltaY;

    SetPixel(Bitmap, x2, y2, ColorIndex);

    while (x1 != x2 || y1 != y2) {
        if (x1 > Bitmap->Width) break;
        if (y1 > Bitmap->Height) break;

        SetPixel(Bitmap, x1, y1, ColorIndex);
        const int error2 = error * 2;

        if (error2 > -deltaY) {
            error -= deltaY;
            x1 += signX;
        }

        if (error2 < deltaX) {
            error += deltaX;
            y1 += signY;
        }
    }
}

void DrawCircle(pBMP Bitmap, int x0, int y0, int radius, uint32_t Color)
{
    int x = 0;
    int y = radius;
    int delta = 2 - 2 * radius;
    int error = 0;

    while (y >= 0) {
        SetPixel(Bitmap, x0 + x, y0 + y, Color);
        SetPixel(Bitmap, x0 + x, y0 - y, Color);
        SetPixel(Bitmap, x0 - x, y0 + y, Color);
        SetPixel(Bitmap, x0 - x, y0 - y, Color);

        error = 2 * (delta + y) - 1;
        if (delta < 0 && error <= 0) {
            ++x;
            delta += 2 * x + 1;
            continue;
        }

        error = 2 * (delta - x) - 1;
        if (delta > 0 && error > 0) {
            --y;
            delta += 1 - 2 * y;
            continue;
        }

        ++x;
        delta += 2 * (x - y);
        --y;
    }
}
