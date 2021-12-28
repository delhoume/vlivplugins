#include <tifhandler.h>

#include <tiffio.h>

static unsigned char tifsig1[] = { 0x49, 0x49, 0x2a, 0x00 };
static unsigned char tifsig2[] = { 0x4d, 0x4d, 0x00, 0x2a };
static unsigned char tifsig3[] = { 0x49, 0x49, 0x2b, 0x00 };

static BOOL AcceptTIFFImage(const unsigned char* buffer, unsigned int size) {
    if (size >= 4) {
	if (((buffer[0] == tifsig1[0]) && (buffer[1] == tifsig1[1]) && (buffer[2] == tifsig1[2]) && (buffer[3] == tifsig1[3])) ||
	    ((buffer[0] == tifsig2[0]) && (buffer[1] == tifsig2[1]) && (buffer[2] == tifsig2[2]) && (buffer[3] == tifsig2[3])) ||
	    ((buffer[0] == tifsig3[0]) && (buffer[1] == tifsig3[1]) && (buffer[2] == tifsig3[2]) && (buffer[3] == tifsig3[3]))) {
	    return TRUE;
	}
    }
    return FALSE;
}

static const char* GetTIFFDescription() { return "TIFF Images"; }
static const char* GetTIFFExtension() { return "*.tif;*.tiff;*.btf"; }

static BOOL OpenTIFFImage(ImagePtr img, const TCHAR* name) {
    TIFF* tif = TIFFOpen(name, "rM");
    if (tif) {
	char* make = 0;
	char* model = 0;
	TIFFGetField(tif, TIFFTAG_MAKE, &make);
	TIFFGetField(tif, TIFFTAG_MODEL, &model);
	if (make && !strcmp(make, "Canon")) {
	    img->numdirs = 1;
	} else {
	    img->numdirs = TIFFNumberOfDirectories(tif);
	}
	img->supportmt = 1;
	img->currentdir = 0;
	TIFFClose(tif);
	return TRUE;
    }
    return FALSE;
}

static void SetTIFFDirectory(ImagePtr img, unsigned int which) {
    uint16_t subfiletype;
    TIFF* tif = TIFFOpen(img->name, "rM");
    if (tif) {
	TIFFSetDirectory(tif, which);
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &img->width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &img->height);
	TIFFGetFieldDefaulted(tif, TIFFTAG_SUBFILETYPE, &subfiletype);
	switch (subfiletype) {
	case FILETYPE_REDUCEDIMAGE: img->subfiletype = ReducedImage; break;
	case FILETYPE_PAGE: img->subfiletype = Page; break;
	default: img->subfiletype = Normal;
	}
	if (TIFFIsTiled(tif)) {
	    img->istiled = 1;
	    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &img->twidth);
	    TIFFGetField(tif, TIFFTAG_TILELENGTH, &img->theight);
	    img->numtilesx = img->width / img->twidth;
	    if (img->width % img->twidth)
		++img->numtilesx;
	    img->numtilesy = img->height / img->theight;
	    if (img->height % img->theight)
		++img->numtilesy;
	} else {
	    img->istiled = 0;
#if defined(READ_ALL_IMAGE)
	    img->twidth = img->width;
	    img->theight = img->height;
	    img->numtilesx = 1;
	    img->numtilesy = 1;
#else
	    img->twidth = img->width;
	    img->numtilesx = 1;
	    TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &img->theight);
	    if (img->theight < 1) {
		img->numtilesy = 1;
		img->theight = img->height;
	    } else {
		img->numtilesy = img->height / img->theight;
		if ((img->numtilesy * img->theight) < img->height)
		    ++img->numtilesy;
	    }
#endif
	}
	TIFFClose(tif);
    }
}

struct TIFFCmap {
    uint16_t* rmap;
    uint16_t* gmap;
    uint16_t* bmap;
};

#define CVT(x) (unsigned char)((((x) * 255) / ((1L << 16) - 1)))

static void FillTIFF(BITMAPINFO* bmi, unsigned int idx, void* arg) {
    struct TIFFCmap* tiffcmap = (struct TIFFCmap*)arg;
    bmi->bmiColors[idx].rgbBlue = CVT(tiffcmap->bmap[idx]);
    bmi->bmiColors[idx].rgbGreen = CVT(tiffcmap->gmap[idx]);
    bmi->bmiColors[idx].rgbRed = CVT(tiffcmap->rmap[idx]);
}

// asm code by Pascal Massimino
static void SwapBytes(unsigned int* bits, unsigned int num) {
#if 1
    if (num & 1) {
	unsigned int pixel = *bits;
	*bits = ((pixel << 16) & 0xff0000) | (pixel & 0xff00ff00) | ((pixel >> 16) & 0xff);
	++bits;
    }
    num >>= 1;
    _asm {
	push ebx;
	mov edx, bits;
	mov ecx, num;
	lea edx,[edx+8*ecx];
	neg ecx;
    LoopS:
	mov eax, [edx+8*ecx];
	mov ebx, [edx+8*ecx+4];
	bswap eax;
	bswap ebx;
	ror eax, 8;
	ror ebx, 8;
	mov [edx+8*ecx],eax;
	mov [edx+8*ecx+4],ebx;
	inc ecx;
	jl LoopS;
	pop ebx;
    }
#else
    while (--num) {
	unsigned int pixel = *bits;
	*bits = ((pixel << 16) & 0xff0000) | (pixel & 0xff00ff00) | ((pixel >> 16) & 0xff);
	++bits;
    }
#endif
}

static HBITMAP LoadTIFFTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    TIFF* tif = TIFFOpen(img->name, "rM");
    HBITMAP hbitmap = 0;
    if (tif) {
	TIFFSetDirectory(tif, img->currentdir);
	if (img->istiled == 1) {
	    unsigned int* bits = 0;
	    hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, img->theight, &bits, 32);
	    TIFFReadRGBATile(tif, x * img->twidth, y * img->theight, bits);
	    SwapBytes(bits, img->twidth * img->theight);
	} else {
	    uint16_t photometric;
	    uint16_t depth;
	    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
	    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &depth);
	    if ((depth != 8) || (photometric != PHOTOMETRIC_PALETTE)) {
		unsigned int* bits = 0;
		hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, img->theight, &bits, 32);
		if (hbitmap) {
#if defined(READ_ALL_IMAGE)
		    TIFFReadRGBAImage(tif, img->twidth, img->theight, bits, 0);
#else
		    TIFFReadRGBAStrip(tif, y * img->theight, bits);
		    if (y == (img->numtilesy - 1)) {
			int offset = img->numtilesy * img->theight - img->height;
			if (offset) {
			    MoveMemory(bits + offset * img->twidth, bits, (img->theight - offset) * img->twidth * 4);
			    ZeroMemory(bits, offset * img->twidth * 4);
			}
		    }
#endif
		    SwapBytes(bits, img->twidth * img->theight);
		}
	    } else {
		struct TIFFCmap tiffcmap;
		unsigned int* bits;
		TIFFGetField(tif, TIFFTAG_COLORMAP, &tiffcmap.rmap, &tiffcmap.gmap, &tiffcmap.bmap);
		hbitmap = img->helper.CreateIndexedDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 
							      FillTIFF, (void*)&tiffcmap);
		if (hbitmap) {
//		    int size = TIFFStripSize(tif);
		    unsigned int realwidth = (((img->twidth * 8) + 31) & ~31) >> 3;
#if defined(READ_ALL_IMAGE)
		    unsigned int idx;
		    for (idx = 0; idx < img->theight; ++idx) 
			TIFFReadScanline(tif, (char*)bits + (img->theight - idx - 1) * realwidth, idx, 0);
#else
		    TIFFReadEncodedStrip(tif, y, (char*)bits, img->theight * img->twidth);
		    if (y == (img->numtilesy - 1)) {
			int offset = img->numtilesy * img->theight - img->height;
			if (offset) {
			    MoveMemory((char*)bits + offset * img->twidth, (char*)bits, 
				       (img->theight - offset) * realwidth);
			    ZeroMemory((char*)bits, offset * realwidth);
			}
		    }
#endif
		}
	    }
	}
	TIFFClose(tif);
    }
    return hbitmap;
}

static void CloseTIFFImage(ImagePtr img) {
}

void RegisterTIFHandler(ImagePtr img) {
    img->helper.Register(AcceptTIFFImage,
			 GetTIFFDescription,
			 GetTIFFExtension,
			 OpenTIFFImage,
			 SetTIFFDirectory,
			 LoadTIFFTile,
			 CloseTIFFImage);
}

