#include "pnghandler.h"
#include <png.h>

static const unsigned int pngtileheight = 256;
static const unsigned int pngmindim = 2000;

static BOOL AcceptPNGImage(const unsigned char* buffer, unsigned int size) {
    static unsigned char pngsig[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    if ((size >= 8) && !memcmp(pngsig, buffer, 8))
	return TRUE;
    return FALSE;
}

const char* GetPNGDescription() { return "PNG Images"; }
const char* GetPNGExtension() { return "*.png"; }

static BOOL OpenPNGImage(ImagePtr img, const TCHAR* name) {
    img->numdirs = 1;
    img->supportmt = 1;
    img->currentdir = 0;
    return TRUE;
}

void png_win32_read_data(png_structp png_ptr, png_bytep data, size_t length) {
   DWORD readbytes;
   if (png_ptr == NULL)
      return;
   ReadFile((HANDLE)png_get_io_ptr(png_ptr), data, length, &readbytes, 0);
   if (readbytes != length)
      png_error(png_ptr, "Read Error");
}


static void SetPNGDirectory(ImagePtr img, unsigned int which) {
	HANDLE file = CreateFile(img->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file) {
	int bitdepth, colortype, itype, ctype, ftype;
	png_structp pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop pnginfo = png_create_info_struct(pngptr);
	png_set_read_fn(pngptr, file, png_win32_read_data);
	png_read_info(pngptr, pnginfo);
	png_get_IHDR(pngptr, pnginfo, &img->width, &img->height, &bitdepth, &colortype, &itype, &ctype, &ftype);
	png_destroy_read_struct(&pngptr, &pnginfo, (png_infopp)0);
	if (img->height <= pngmindim) {
	    img->numtilesy = 1;
	    img->theight = img->height;
	    img->istiled = FALSE;
	} else {
	    img->theight = pngtileheight;
	    img->istiled = TRUE;
	    img->numtilesy = img->height / img->theight;
	    if ((img->numtilesy * img->theight) <img->height) 
		++img->numtilesy;
	}
	img->twidth = img->width;
	img->numtilesx = 1;
	CloseHandle(file);
    }
}


static HBITMAP LoadPNGTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    HANDLE file = CreateFile(img->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   HBITMAP hbitmap = 0;
    if (file) {
	int bitdepth, colortype, itype, ctype, ftype, passes, p;
	unsigned int h, cpos;
	unsigned int starty;
	png_uint_32 width;
	png_uint_32 height;
	unsigned int* bits;
	png_bytep temp;
	png_structp pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop pnginfo = png_create_info_struct(pngptr);
	starty = y * img->theight;
	png_set_read_fn(pngptr, file, png_win32_read_data);
	png_read_info(pngptr, pnginfo);
	png_get_IHDR(pngptr, pnginfo, &width, &height, &bitdepth, &colortype, &itype, &ctype, &ftype);
	if (colortype == PNG_COLOR_TYPE_PALETTE) png_set_expand(pngptr);
	if (colortype == PNG_COLOR_TYPE_GRAY && bitdepth < 8) png_set_expand(pngptr);
	if (bitdepth == 16) png_set_strip_16(pngptr);
	if (png_get_valid(pngptr, pnginfo, PNG_INFO_tRNS)) {
	    png_set_swap_alpha(pngptr);
	    png_set_expand(pngptr);
	}
	png_set_filler(pngptr, 0x00, PNG_FILLER_AFTER);
	png_set_bgr(pngptr);
	if (colortype == PNG_COLOR_TYPE_GRAY || colortype == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(pngptr);
	hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 32);
	if (bits) {
	    if (setjmp(png_jmpbuf(pngptr))) {
		png_destroy_read_struct(&pngptr, &pnginfo, (png_infopp)NULL);
		return hbitmap;
	    }
	    passes = png_set_interlace_handling(pngptr);
	    png_read_update_info(pngptr, pnginfo);
	    temp = (png_bytep)MYALLOC(img->twidth * 4);
	    cpos = 0;
	    for (p = 0; p < passes; ++p) {
		cpos = 0;
		// skip to begin of tile
		for (h = 0; h < starty; ++h, ++cpos) 
		    png_read_row(pngptr, temp, 0);
		for (h = 0; (h < img->theight) && ((h + starty) < img->height); ++h, ++cpos)
		    png_read_row(pngptr, (png_bytep)(bits + img->twidth * h), 0);
		if (passes > 1) 
		    for (; cpos < img->height; ++h, ++cpos)
			png_read_row(pngptr, temp, 0);
	    }
	    png_read_end(pngptr, pnginfo);
	    png_destroy_read_struct(&pngptr, &pnginfo, (png_infopp)0);
	    MYFREE(temp);
	}
	CloseHandle(file);
    }
    return hbitmap;
}

static void ClosePNGImage(ImagePtr img) {
}

void RegisterPNGHandler(ImagePtr img) {
    img->helper.Register(AcceptPNGImage,
			 GetPNGDescription,
			 GetPNGExtension,
			 OpenPNGImage,
			 SetPNGDirectory,
			 LoadPNGTile,
			 ClosePNGImage);
}

