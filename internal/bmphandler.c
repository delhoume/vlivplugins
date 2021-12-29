#include <bmphandler.h>

#include <stdio.h>

// TODO: replace by win32 API

static BOOL AcceptBMPImage(const unsigned char* buffer, unsigned int size) {
    if (size >= 2) {
	if ((buffer[0] == 'B') && (buffer[1] == 'M')) {
	    return TRUE;
	}
    }
    return FALSE;
}

static const char* GetBMPDescription() { return "Windows BMP Images"; }
static const char* GetBMPExtension() { return "*.bmp;*.dib"; }

struct bmp_internal {
    unsigned int offset;
    unsigned short bitcount;
    unsigned int compression;
    unsigned int paloffset;
	FILE* file;
};

static BOOL OpenBMPImage(ImagePtr img, const TCHAR* name) {
    struct bmp_internal* bmp_internal = (struct bmp_internal*)MYALLOC(sizeof(struct bmp_internal));
    img->handler->internal = (void*)bmp_internal;
    bmp_internal->offset = 0;
    bmp_internal->paloffset = 0;
    img->numdirs = 1;
    img->supportmt = 0;
    img->currentdir = 0;
    return TRUE;
}

static unsigned int ReadInt(FILE* file) {
    unsigned int buf;
    fread((char*)&buf, 1, sizeof(unsigned int), file);
    return buf;
}

static unsigned short ReadShort(FILE* file) {
    unsigned short buf;
    fread((char*)&buf, 1, sizeof(unsigned short), file);
    return buf;
}

#define WIN_NEW	    40
#define OS2_NEW	    64
#define WIN_OS2_OLD 12
#define WIN_V5      124

static void SetBMPDirectory(ImagePtr img, unsigned int which) {
    struct bmp_internal* bmp_internal = (struct bmp_internal*)img->handler->internal;
    FILE* file = fopen(img->name, "rb");
    if (file) {
	unsigned int offset;
	unsigned int headersize;
	fgetc(file);
	fgetc(file);
	ReadInt(file); ReadShort(file); ReadShort(file);
	offset = ReadInt(file);
	headersize = ReadInt(file);
	if ((headersize == WIN_NEW || headersize == OS2_NEW || headersize == WIN_V5)) {
	    img->width = ReadInt(file);
	    img->height = ReadInt(file);
	    ReadShort(file); // planes
	    bmp_internal->bitcount = ReadShort(file);
	    bmp_internal->compression = ReadInt(file);
	    ReadInt(file); // size
	    ReadInt(file); // horizontal resolution
	    ReadInt(file); // vertical resolution
	    ReadInt(file); // colors used
	    ReadInt(file); // important colors
	}
	
	if (headersize != WIN_OS2_OLD) {
	    unsigned int bytesleft = headersize - 40;
	    while (bytesleft--) 
			fgetc(file);
	}
	switch (bmp_internal->bitcount) {
		case 1: case 4: case 8: {
			RGBQUAD rgb;
			unsigned int idx;
			unsigned int palsize = 1 << bmp_internal->bitcount;
			bmp_internal->paloffset = ftell(file);
			for (idx = 0; idx < palsize; ++idx) {
				fread((char*)&rgb, 1, sizeof(RGBQUAD), file);
			}
		}
	}
	bmp_internal->offset = ftell(file);
	img->istiled = FALSE;
	img->numtilesx = 1;
	img->numtilesy = 1;
	img->twidth = img->width;
	img->theight = img->height;
	img->subfiletype = Normal;
	fclose(file);
    }
}

static void FillBMPPalette(BITMAPINFO* bmi, unsigned int idx, void* arg) {
    struct bmp_internal* bmp_internal = (struct bmp_internal*)arg;
    FILE* file = (FILE*)bmp_internal->file;
    fseek(file, bmp_internal->paloffset, SEEK_SET);
    for (unsigned int i = 0; i < idx; ++i) {
		fread((char*)&(bmi->bmiColors[idx]), 1, sizeof(RGBQUAD), file);
	}
}

static HBITMAP 
LoadBMPTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
   FILE* file = fopen(img->name, "rb");
    HBITMAP hbitmap = 0;
    if (file) {
		unsigned int* bits = 0;
		struct bmp_internal* bmpinternal = (struct bmp_internal*)img->handler->internal;
		bmpinternal->file = file;
		unsigned short bitcount = bmpinternal->bitcount;
		unsigned int realwidth = (((img->width * bitcount) + 31) & ~31) >> 3;
		switch (bitcount) {
			case 1: case 4: case 8:
				if (bmpinternal->compression == BI_RGB) {
					hbitmap = img->helper.CreateIndexedDIBSection(hdc, img->width, img->height, &bits, FillBMPPalette, (void*)bmpinternal);
				} else {
					hbitmap = img->helper.CreateDefaultDIBSection(hdc, img->width, img->height, "Unsupported compression", &bits);
					return hbitmap;
				}
				break;
			case 16: case 24: case 32: 
			if (bmpinternal->compression == BI_RGB) {
				hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->width, img->height, &bits, bitcount);
			} else {
				hbitmap = img->helper.CreateDefaultDIBSection(hdc, img->width, img->height, "Unsupported compression", &bits);
			}
			break;
		}
		if (bits) {
			fseek(file, bmpinternal->offset, SEEK_SET); // goto start of data
			fread((char*)bits, 1, img->height * realwidth, file); 
		}
		fclose(file);
    }
    return hbitmap;
}

static void CloseBMPImage(ImagePtr img) {
    if (img->handler && img->handler->internal) {
	struct bmp_internal* bmp_internal = (struct bmp_internal*)img->handler->internal;
	MYFREE(bmp_internal);
    }
}

void RegisterBMPHandler(ImagePtr img) {
    img->helper.Register(AcceptBMPImage,
			 GetBMPDescription,
			 GetBMPExtension,
			 OpenBMPImage,
			 SetBMPDirectory,
			 LoadBMPTile,
			 CloseBMPImage);
}

