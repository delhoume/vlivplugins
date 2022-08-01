#include "jpghandler.h"

#include <stdio.h>

#include <setjmp.h>
#include <jpeglib.h>

struct error_mgr
{
    struct jpeg_error_mgr mgr;
    jmp_buf set_jmp_buffer;
};
typedef struct error_mgr* error_ptr;

static void error_exit(j_common_ptr cinfo) {
    error_ptr err_ptr = (error_ptr)cinfo->err;
    longjmp(err_ptr->set_jmp_buffer, 1);
}

static BOOL AcceptJPEGImage(const unsigned char* buffer, unsigned int size) {
    if (size >= 3) {
	if ((buffer[0] == 0xff) && (buffer[1] == 0xd8) && (buffer[2] == 0xff))
	    return TRUE;
    }
    return FALSE;
}

static const char* GetJPEGDescription() { return "JPEG Images"; }
static const char* GetJPEGExtension() { return "*.jpg;*.jpeg"; }

static BOOL
OpenJPEGImage(ImagePtr img, const TCHAR* name) {
    img->numdirs = 4;
    img->supportmt = 1;
    img->currentdir = 3;
    return TRUE;
}

static const unsigned int jpegtileheight = 256;
static const unsigned int jpegmindim = 2000;

static void
SetJPEGDirectory(ImagePtr img, unsigned int which) {
    FILE* file = fopen(img->name, "rb");
    if (file) {
	struct jpeg_decompress_struct cinfo;
	struct error_mgr jerr;
	jpeg_create_decompress(&cinfo);
	cinfo.err = jpeg_std_error(&jerr.mgr);
	jerr.mgr.error_exit = error_exit;
	if (setjmp(jerr.set_jmp_buffer)) {
	    jpeg_destroy_decompress(&cinfo);
	    fclose(file);
	    return;
	}
	jpeg_stdio_src(&cinfo, file);
	jpeg_read_header(&cinfo, TRUE);
	// Virtual pyramid for JPEG !!!!
	switch (which) {
	case 0: cinfo.scale_denom = 8; img->subfiletype = Virtual; break;
	case 1: cinfo.scale_denom = 4; img->subfiletype = Virtual; break;
	case 2: cinfo.scale_denom = 2; img->subfiletype = Virtual; break;
	case 3: default: cinfo.scale_denom = 1; img->subfiletype = Normal; break;
	}
	jpeg_calc_output_dimensions(&cinfo);
	img->width = cinfo.output_width;
	img->height = cinfo.output_height;
	if (img->height <= jpegmindim) {
	    img->numtilesy = 1;
	    img->theight = img->height;
	    img->istiled = FALSE;
	} else {
	    img->numtilesy = (img->height / jpegtileheight);
	    if ((img->numtilesy * jpegtileheight) < img->height)
		++img->numtilesy;
	    img->theight = jpegtileheight;
	    img->istiled = TRUE;
	}
	img->numtilesx = 1;
	img->twidth = img->width;
	jpeg_destroy_decompress(&cinfo);
	fclose(file);
    }
}

static HBITMAP
LoadJPEGTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    FILE* file = fopen(img->name, "rb");
    HBITMAP hbitmap = 0;
    if (file) {
	unsigned int* bits = 0;
	struct jpeg_decompress_struct cinfo;
	struct error_mgr jerr;
	jpeg_create_decompress(&cinfo);
	cinfo.err = jpeg_std_error(&jerr.mgr);
	jerr.mgr.error_exit = error_exit;
	jpeg_stdio_src(&cinfo, file);
	if (setjmp(jerr.set_jmp_buffer)) {
	    jpeg_destroy_decompress(&cinfo);
	    fclose(file);
	    return hbitmap;
	}
	jpeg_read_header(&cinfo, TRUE);
	switch (img->currentdir) {
	case 0: cinfo.scale_denom = 8; break;
	case 1: cinfo.scale_denom = 4; break;
	case 2: cinfo.scale_denom = 2; break;
	case 3: default: cinfo.scale_denom = 1; break;
	}
	jpeg_calc_output_dimensions(&cinfo);
	hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 24);
	if (bits) {
	    char* dstptr = (char*)bits;
	    unsigned int idx, idx2;
	    unsigned int starty = y * img->theight;
	    int scanlinesize = img->twidth * 24 / 8;
	    cinfo.out_color_space = JCS_RGB;
	    jpeg_start_decompress(&cinfo);
	    while ((scanlinesize % 4) != 0)
		++scanlinesize;
	    for (idx = 0; idx < starty; ++idx) {
		jpeg_read_scanlines(&cinfo, &dstptr, 1);
	    }
	    for (idx = 0; (idx < img->theight) && (cinfo.output_scanline < cinfo.output_height); ++idx) {
		char* dstptr = (char*)bits + scanlinesize * idx;
		jpeg_read_scanlines(&cinfo, &dstptr, 1);
	    }
	    jpeg_abort_decompress(&cinfo);
	    // now convert to BGR
	    for (idx2 = 0; idx2 < img->theight; ++idx2) {
		dstptr = (char*)bits + scanlinesize * idx2;
		for (idx = 0; idx < img->twidth; ++idx, dstptr += 3) {
		    char b = dstptr[2];
		    dstptr[2] = dstptr[0];
		    dstptr[0] = b;
		}
	    }
	}
	jpeg_destroy_decompress(&cinfo);
	fclose(file);
    }
    return hbitmap;
}

static void CloseJPEGImage(ImagePtr img) {}

void RegisterJPGHandler(ImagePtr img) {
    img->helper.Register(AcceptJPEGImage,
			 GetJPEGDescription,
			 GetJPEGExtension,
			 OpenJPEGImage,
			 SetJPEGDirectory,
			 LoadJPEGTile,
			 CloseJPEGImage);
}

