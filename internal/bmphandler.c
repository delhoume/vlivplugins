#include <bmphandler.h>

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
	HANDLE handle;
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

static unsigned int ReadInt(HANDLE handle) {
    unsigned int buf;
	DWORD readbytes;
	ReadFile(handle, &buf, sizeof(unsigned int), &readbytes, 0);
     return buf;
}

static unsigned short ReadShort(HANDLE handle) {
    unsigned short buf;
	DWORD readbytes;
 	ReadFile(handle, &buf, sizeof(unsigned short), &readbytes, 0);
    return buf;
}

static unsigned char ReadByte(HANDLE handle) {
    unsigned char buf;
	DWORD readbytes;
 	ReadFile(handle, &buf, sizeof(unsigned char), &readbytes, 0);
    return buf;
}

static RGBQUAD ReadQuad(HANDLE handle) {
    RGBQUAD buf;
	DWORD readbytes;
 	ReadFile(handle, &buf, sizeof(RGBQUAD), &readbytes, 0);
    return buf;
}

#define WIN_NEW	    40
#define OS2_NEW	    64
#define WIN_V5      124

static void SetBMPDirectory(ImagePtr img, unsigned int which) {
    struct bmp_internal* bmp_internal = (struct bmp_internal*)img->handler->internal;
    HANDLE handle = CreateFile(img->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			       FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle) {
		unsigned int offset;
		unsigned int headersize;
		unsigned int headerpos;
		ReadByte(handle); ReadByte(handle);
		ReadInt(handle); ReadShort(handle); ReadShort(handle);
		offset = ReadInt(handle);
		headersize = ReadInt(handle);
		headerpos = SetFilePointer(handle, 0, 0, FILE_CURRENT);
		if ((headersize == WIN_NEW || headersize == OS2_NEW || headersize == WIN_V5)) {
			img->width = ReadInt(handle);
			img->height = ReadInt(handle);
			ReadShort(handle); // planes
			bmp_internal->bitcount = ReadShort(handle);
			bmp_internal->compression = ReadInt(handle);
			ReadInt(handle); // size
			ReadInt(handle); // horizontal resolution
			ReadInt(handle); // vertical resolution
			ReadInt(handle); // colors used
			ReadInt(handle); // important colors
//			DWORD rmask = ReadInt(handle);
//			DWORD gmask = ReadInt(handle);
//			DWORD bmask = ReadInt(handle);
			headerpos = SetFilePointer(handle, 0, 0, FILE_CURRENT) - headerpos;
		}
		// skip remaining header
		unsigned int bytesleft = headersize - headerpos;
		while (bytesleft--) {
			ReadByte(handle);
		}
		switch (bmp_internal->bitcount) {
			// skip palette, not used yet
			case 1: case 4: case 8: {
				unsigned int idx;
				unsigned int palsize = 1 << bmp_internal->bitcount;
				bmp_internal->paloffset = SetFilePointer(handle, 0, 0, FILE_CURRENT);
				for (idx = 0; idx < palsize; ++idx) {
					ReadQuad(handle);
				}
			}
		}
		bmp_internal->offset = SetFilePointer(handle, 0, 0, FILE_CURRENT);
		img->istiled = FALSE;
		img->numtilesx = 1;
		img->numtilesy = 1;
		img->twidth = img->width;
		img->theight = img->height;
		img->subfiletype = Normal;
		CloseHandle(handle);
    }
}


static void FillBMPPalette(BITMAPINFO* bmi, unsigned int idx, void* arg) {
    struct bmp_internal* bmp_internal = (struct bmp_internal*)arg;
    HANDLE handle = bmp_internal->handle;
    SetFilePointer(handle, bmp_internal->paloffset, 0, FILE_BEGIN);
    for (unsigned int i = 0; i < idx; ++i) {
		bmi->bmiColors[idx] = ReadQuad(handle);
	}
}

static HBITMAP 
LoadBMPTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
  HANDLE handle = CreateFile(img->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			       FILE_ATTRIBUTE_NORMAL, NULL);
    HBITMAP hbitmap = 0;
    if (handle) {
		unsigned int* bits = 0;
		struct bmp_internal* bmpinternal = (struct bmp_internal*)img->handler->internal;
		bmpinternal->handle = handle;
		unsigned short bitcount = bmpinternal->bitcount;
		unsigned int realwidth = (((img->width * bitcount) + 31) & ~31) >> 3;
		switch (bitcount) {
			case 1: case 4: case 8:
				if (bmpinternal->compression == BI_RGB ) {
					hbitmap = img->helper.CreateIndexedDIBSection(hdc, img->width, img->height, &bits, FillBMPPalette, (void*)bmpinternal);
				} else {
					hbitmap = img->helper.CreateDefaultDIBSection(hdc, img->width, img->height, "Unsupported compression", &bits);
				}
				break;
			case 16: case 24: case 32: 
				if ((bmpinternal->compression == BI_RGB) || (bmpinternal->compression == BI_BITFIELDS)) {
					hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->width, img->height, &bits, bitcount);
				} else {
					hbitmap = img->helper.CreateDefaultDIBSection(hdc, img->width, img->height, "Unsupported compression", &bits);
				}
			break;
		}
		if (bits) {
			DWORD readbytes;
			SetFilePointer(handle, bmpinternal->offset, 0, FILE_BEGIN);
			ReadFile(handle, bits, (DWORD)(img->height * realwidth), &readbytes, 0);
    	}
		CloseHandle(handle);
	} else {
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

