#include <vliv.h>
#include <wichandler.h>

#include <wincodec.h>

#include <stdio.h>

#define FULLIMAGE

static BOOL AcceptWICImage(const unsigned char* buffer, unsigned int size) {
    return FALSE;
}

static const char* GetWICDescription() { return "WIC Images"; }
static const char* GetWICExtension() { return "*.wdp;*.hdp;*.crw;*.cr2"; }

struct wic_internal {
    IWICImagingFactory *piFactory;
    IWICBitmapDecoder *piDecoder;
    IWICBitmapFrameDecode *piBitmapFrame;
    IWICFormatConverter* piConverter;
};

static BOOL OpenWICImage(ImagePtr img, const TCHAR* name) {
    struct wic_internal* wic_internal = (struct wic_internal*)MYALLOC(sizeof(struct wic_internal));
    img->handler->internal = (void*)wic_internal;
    wic_internal->piFactory = 0;
    wic_internal->piDecoder = 0;
    wic_internal->piBitmapFrame = 0;
    wic_internal->piConverter = 0;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory,
				  NULL,
				  CLSCTX_INPROC_SERVER,
				  IID_IWICImagingFactory,
				  (LPVOID*)&wic_internal->piFactory);
    if (SUCCEEDED(hr)) {
      WCHAR wfilename[MAX_PATH];
      MultiByteToWideChar(CP_ACP, 0, img->name, -1, wfilename, MAX_PATH - 1);
	hr = wic_internal->piFactory->CreateDecoderFromFilename(wfilename, 
						  NULL, GENERIC_READ, 
						  WICDecodeMetadataCacheOnLoad, 
						  &wic_internal->piDecoder);
	if (SUCCEEDED(hr)) {
	    UINT uiFrameCount = 0;
	    wic_internal->piDecoder->GetFrameCount(&uiFrameCount);
	    img->numdirs = uiFrameCount;
  	} else {
	  wic_internal->piFactory->Release();
	  wic_internal->piFactory = 0;
	  return FALSE;
	}
    }
    img->supportmt = 0;
    img->currentdir = 0;
    return TRUE;
}

static void SetWICDirectory(ImagePtr img, unsigned int which) {
    struct wic_internal* wic_internal = (struct wic_internal*)img->handler->internal;
    IWICImagingFactory *piFactory = wic_internal->piFactory;
    IWICBitmapDecoder *piDecoder = wic_internal->piDecoder;
    if (wic_internal->piBitmapFrame) {
	wic_internal->piBitmapFrame->Release();
	wic_internal->piBitmapFrame = 0;
    }
    if (wic_internal->piConverter) {
	wic_internal->piConverter->Release();
	wic_internal->piConverter = 0;
    }
    HRESULT hr = piDecoder->GetFrame(which, &wic_internal->piBitmapFrame);
    if (SUCCEEDED(hr)) {
	UINT uiWidth = 0, uiHeight = 0;
	hr = wic_internal->piBitmapFrame->GetSize(&uiWidth, &uiHeight);
	wic_internal->piFactory->CreateFormatConverter(&wic_internal->piConverter);
	wic_internal->piConverter->Initialize(wic_internal->piBitmapFrame, 
					      GUID_WICPixelFormat24bppBGR,
					      WICBitmapDitherTypeNone,
					      NULL, 
					      0.0, 
					      WICBitmapPaletteTypeCustom);
  	img->width = uiWidth;
	img->height = uiHeight;
#if defined(FULLIMAGE)
	img->twidth = img->width;
	img->theight = img->height;
#else
	img->twidth = 256;
	img->theight = 256;
#endif
	img->numtilesx = img->width / img->twidth;
	if ((img->numtilesx * img->twidth) <img->width) 
	    ++img->numtilesx;
	img->numtilesy = img->height / img->theight;
	if ((img->numtilesy * img->theight) <img->height) 
	    ++img->numtilesy;
	img->istiled = TRUE;
    }
}

static HBITMAP 
LoadWICTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    struct wic_internal* wic_internal = (struct wic_internal*)img->handler->internal;
    unsigned int* bits = 0;
    HBITMAP hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -((int)img->theight), &bits, 24);
    UINT stride = (((img->twidth * 24) + 31) & ~31) >> 3;
    UINT buffersize = stride * img->theight;
    RECT imgrect;
    RECT tilerect;
    RECT realrect;
    SetRect(&imgrect, 0, 0, img->width, img->height);
    SetRect(&tilerect, x * img->twidth, y * img->theight, (x + 1) * img->twidth, (y + 1) * img->theight);
    IntersectRect(&realrect, &imgrect, &tilerect);
    WICRect rc = { 0 }; 
    rc.X = realrect.left;
    rc.Y = realrect.top;
    rc.Width = realrect.right - realrect.left;
    rc.Height = realrect.bottom - realrect.top;
    wic_internal->piConverter->CopyPixels(&rc, stride, buffersize, (BYTE*)bits);
    return hbitmap;
}

static void CloseWICImage(ImagePtr img) {
    struct wic_internal* wic_internal = (struct wic_internal*)img->handler->internal;
    if (wic_internal->piDecoder) 
	wic_internal->piDecoder->Release();
    if (wic_internal->piFactory) 
	wic_internal->piFactory->Release();
    if (wic_internal->piBitmapFrame)
	wic_internal->piBitmapFrame->Release();
    if (wic_internal->piConverter)
	wic_internal->piConverter->Release();
    MYFREE(wic_internal);
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptWICImage,
			 GetWICDescription,
			 GetWICExtension,
			 OpenWICImage,
			 SetWICDirectory,
			 LoadWICTile,
			 CloseWICImage);
}

