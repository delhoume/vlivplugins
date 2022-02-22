#define WIN32_LEAN_AND_MEAN 
#include <stbhandler.h>

#define STBI_ONLY_PSD
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// very simple plugin that loads PSD files using stb_image header only library

static BOOL AcceptSTBImage(const unsigned char* buffer, unsigned int size) { 
    if (size >= 4) {
		if ((buffer[0] == '8') && (buffer[1] == 'B')&& (buffer[2] == 'P') && (buffer[1] == 'S')) {
			return TRUE;
		}
    }
    return FALSE;
}

const char* GetSTBDescription() { return "PSD Images"; }
const char* GetSTBExtension() { return "*.PSD"; }

static BOOL OpenSTBImage(ImagePtr img, const TCHAR* name) {
    img->numdirs = 1;
    img->supportmt = 0;
    img->currentdir = 0;
    return TRUE;
}

static void SetSTBDirectory(ImagePtr img, unsigned int which) {
	int w, h, c;	
	if (stbi_info(img->name, &w, &h, &c)) {
        img->width = w;
        img->height  = h;
        img->numtilesx = 1;
		img->numtilesy = 1;
		img->twidth = img->width;
		img->theight = img->height;
		img->subfiletype = Normal;
        img->istiled = FALSE;
    }
 }

static HBITMAP
LoadSTBTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    HBITMAP hbitmap = 0;    
    unsigned int* bits = 0;
	int w, h, c;
	unsigned char* data = stbi_load(img->name, &w, &h, &c, 4);
	if (data) {
		hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 32);
		if (bits) {
			memcpy(bits, data, w * h * 4);
		}
		STBI_FREE(data);
	} else {
		hbitmap = img->helper.CreateDefaultDIBSection(hdc, img->twidth, img->theight, "Error", &bits);
	}
	return hbitmap;
}

static void CloseSTBImage(ImagePtr img) {
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptSTBImage,
			 GetSTBDescription,
			 GetSTBExtension,
			 OpenSTBImage,
			 SetSTBDirectory,
			 LoadSTBTile,
			 CloseSTBImage);
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                   DWORD  ul_reason_for_call, 
                   LPVOID lpReserved
                 ) {
    return TRUE;
}
