#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#define WIN32_LEAN_AND_MEAN 
#include <exrhandler.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


// very simple plugin that loads EXR files using tinyexr header only library
static BOOL AcceptEXRImage(const unsigned char* buffer, unsigned int size) { 
    return IsEXRFromMemory(buffer, size) == TINYEXR_SUCCESS ? TRUE : FALSE;
}

const char* GetEXRDescription() { return "EXR Images"; }
const char* GetEXRExtension() { return "*.EXR"; }

struct exr_internal {
	unsigned char* image8;
	const char* err = NULL;
};

static BOOL OpenEXRImage(ImagePtr img, const TCHAR* name) {
    struct exr_internal* exr_internal = (struct exr_internal*)MYALLOC(sizeof(struct exr_internal));
	exr_internal->image8 = 0;
    img->handler->internal = (void*)exr_internal;
    img->numdirs = 1;
    img->supportmt = 0;
    img->currentdir = 0;
    return TRUE;
}

static void SwapBytes(unsigned int* bits, unsigned int num) {
    while (--num) {
		unsigned int pixel = *bits;
		*bits = ((pixel << 16) & 0xff0000) | (pixel & 0xff00ff00) | ((pixel >> 16) & 0xff);
		++bits;
    }
}

static void SetEXRDirectory(ImagePtr img, unsigned int which) {
    struct exr_internal* exr_internal = (struct exr_internal*)img->handler->internal;
	int width, height;
	float* image;

	if (LoadEXR(&image, &width, &height, img->name, &(exr_internal->err)) == TINYEXR_SUCCESS) {
 		// convert image to 8 bits
		unsigned char* image8 = stbi__hdr_to_ldr(image, width, height, 4);
		// leak or crash ? I choose leak for now
		// free(image);
		// now convert to BGRA
		SwapBytes((unsigned int*)image8, width * height);
		exr_internal->image8 = image8;
	}  else {
		width = 500;
        height  = 500;
	}
	img->width = width;
	img->height  = height;
	img->twidth = img->width;
	img->theight = img->height;
	img->numtilesx = 1;
	img->numtilesy = 1;
	img->subfiletype = Normal;
	img->istiled = FALSE;
}
 

static HBITMAP
LoadEXRTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
   struct exr_internal* exr_internal = (struct exr_internal*)img->handler->internal;
   	unsigned int* bits = 0;
    HBITMAP hbitmap = 0;    
   if (exr_internal->image8 == 0) {
	   hbitmap = img->helper.CreateDefaultDIBSection(hdc, img->twidth, img->theight, exr_internal->err, &bits);
	   FreeEXRErrorMessage(exr_internal->err);
   } else {
		hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -((int)img->theight), &bits, 32);
		memcpy(bits, exr_internal->image8, img->twidth* img->theight * 4);
		free(exr_internal->image8);
   }
	return hbitmap;
}

static void CloseEXRImage(ImagePtr img) {
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptEXRImage,
			 GetEXRDescription,
			 GetEXRExtension,
			 OpenEXRImage,
			 SetEXRDirectory,
			 LoadEXRTile,
			 CloseEXRImage);
}
