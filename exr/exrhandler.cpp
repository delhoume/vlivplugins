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
	float* image;
};

static BOOL OpenEXRImage(ImagePtr img, const TCHAR* name) {
    struct exr_internal* exr_internal = (struct exr_internal*)MYALLOC(sizeof(struct exr_internal));
	exr_internal->image = 0;
    img->handler->internal = (void*)exr_internal;
    img->numdirs = 1;
    img->supportmt = 0;
    img->currentdir = 0;
    return TRUE;
}

static void SetEXRDirectory(ImagePtr img, unsigned int which) {
    struct exr_internal* exr_internal = (struct exr_internal*)img->handler->internal;
	int width, height;
	const char* err = NULL;
	if (LoadEXR(&(exr_internal->image), &width, &height, img->name, &err) == TINYEXR_SUCCESS) {
        img->width = width;
        img->height  = height;
        img->numtilesx = 1;
		img->numtilesy = 1;
		img->twidth = img->width;
		img->theight = img->height;
		img->subfiletype = Normal;
        img->istiled = FALSE;
	} else 
		FreeEXRErrorMessage(err);
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

static HBITMAP
LoadEXRTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    struct exr_internal* exr_internal = (struct exr_internal*)img->handler->internal;
	float* image = exr_internal->image;
    HBITMAP hbitmap = 0;    
    unsigned int* bits = 0;
	hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -((int)img->theight), &bits, 32);
#if 1
	// convert image to 8 bits
	unsigned char* image8 = stbi__hdr_to_ldr(image, img->twidth, img->theight, 4);
	// now convert to BGRA
	SwapBytes((unsigned int*)image8, img->twidth * img->theight);
	memcpy(bits, image8, img->twidth* img->theight * 4);
	free(image8);
#else
	// convert image to BGRA
	for (unsigned int y = 0; y < img->theight; ++y) {
		for (unsigned int x = 0; x < img->twidth; ++x) {
			float* imagep = image + (y * img->twidth * 4) + x * 4;
			float r = imagep[0];
			float g = imagep[1];
			float b = imagep[2];
			
			if (r >= 1.0) r = 1.0;
			if (g >= 1.0) g = 1.0;
			if (b >= 1.0) b = 1.0;
			
			unsigned char bb = (unsigned char)(b * 255.0);
			unsigned char gg = (unsigned char)(g * 255.0);
			unsigned char rr = (unsigned char)(r * 255.0);
			
			unsigned char* destp = (unsigned char*)((unsigned char*)bits + (y * img->twidth * 4) + x * 4);
			destp[0] = bb;
			destp[1] = gg;
			destp[2] = rr;
			destp[3] = 255;
		}
	}
#endif
	return hbitmap;
}

static void CloseEXRImage(ImagePtr img) {
    struct exr_internal* exr_internal = (struct exr_internal*)img->handler->internal;
//	free(exr_internal->image);
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

BOOL APIENTRY DllMain( HANDLE hModule, 
                   DWORD  ul_reason_for_call, 
                   LPVOID lpReserved
                 ) {
    return TRUE;
}
