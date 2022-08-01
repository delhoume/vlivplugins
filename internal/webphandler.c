#include <webp/decode.h>
#include "webphandler.h"

static BOOL AcceptWEBPImage(const unsigned char* buffer, unsigned int size) { 
if (size >= 12) { 
    if ((buffer[0] == 'R') && (buffer[1] == 'I') && (buffer[2] == 'F') && (buffer[3] == 'F') &&
	(buffer[8] == 'W') && (buffer[9] == 'E') && (buffer[10] == 'B') && (buffer[11] == 'P'))
      return TRUE;
  }
  return FALSE; 
}

const char* GetWEBPDescription() { return "WebP Images"; }
const char* GetWEBPExtension() { return "*.webp"; }


struct webp_internal {
	char* data;
	DWORD data_size;
};

static BOOL OpenWEBPImage(ImagePtr img, const TCHAR* name) {
    struct webp_internal* internal = (struct webp_internal*)MYALLOC(sizeof(struct webp_internal));
	img->handler->internal = (void*)internal;
	img->numdirs = 1;
	img->supportmt = 0;
	img->currentdir = 0;
	return TRUE;
}

// only one directory
static void SetWEBPDirectory(ImagePtr img, unsigned int which) {
    struct webp_internal* internal = (struct webp_internal*)img->handler->internal;
	HANDLE fileHandle = CreateFile(img->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	int width = 1000;
	int height = 1000;
	if (fileHandle != INVALID_HANDLE_VALUE) {
		DWORD filesize = GetFileSize(fileHandle, NULL);
		DWORD readbytes;
		internal->data_size = filesize;
		internal->data = (char*)MYALLOC(filesize);
		ReadFile(fileHandle, internal->data, internal->data_size, &readbytes, NULL);
		WebPGetInfo(internal->data, internal->data_size, &width, &height);
		img->numtilesx = 1;
		img->numtilesy = 1;
		img->twidth = width;
		img->theight = height;
		img->width = width;
		img->height = height;
		img->istiled = FALSE;
		img->subfiletype = Normal;
		CloseHandle(fileHandle);
	} else {
	}
}

static HBITMAP
LoadWEBPTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
  unsigned int* bits = 0;
  HBITMAP hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -img->theight, &bits, 24);
  if (bits) {
      struct webp_internal* internal = (struct webp_internal*)img->handler->internal;
	  int stride = ((img->twidth * 24 + 31) & ~31) >> 3;
	  WebPDecodeBGRInto((const uint8_t*)internal->data, internal->data_size, (uint8_t*)bits, stride * img->theight, stride);
  }
  return hbitmap;
}

static void CloseWEBPImage(ImagePtr img) {
    if (img->handler && img->handler->internal) {
		struct webp_internal* internal = (struct webp_internal*)img->handler->internal;
		MYFREE(internal->data);
		MYFREE(internal);
    }
}

void RegisterWEBPHandler(ImagePtr img) {
    img->helper.Register(AcceptWEBPImage,
			 GetWEBPDescription,
			 GetWEBPExtension,
			 OpenWEBPImage,
			 SetWEBPDirectory,
			 LoadWEBPTile,
			 CloseWEBPImage);
}

