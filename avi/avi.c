#include <avi.h>

#include <vfw.h>

static BOOL AcceptAVIImage(const unsigned char* buffer, unsigned int size) { 
  return FALSE; 
}

const char* GetAVIDescription() { return "AVI Movies"; }
const char* GetAVIExtension() { return "*.avi"; }

const DWORD columns = 100;

struct avi_internal {
    PAVISTREAM pStream;
    PGETFRAME pFrame;
    DWORD width;
    DWORD height;
    DWORD frames;
};

static BOOL OpenAVIImage(ImagePtr img, const TCHAR* name) {
    struct avi_internal* internal = (struct avi_internal*)MYALLOC(sizeof(struct avi_internal));
    PAVIFILE avi;
    AVIFileInit();
    if (AVIFileOpen(&avi, img->name, OF_PARSE, NULL) == AVIERR_OK) {
	img->handler->internal = (void*)internal;
	img->numdirs = 4;
	img->supportmt = 0;
	img->currentdir = 3;
	internal->pStream = 0;
	internal->pFrame = 0;
	AVIFileRelease(avi);
	return TRUE;
    } else 
	return FALSE;
}

// only one directory
static void SetAVIDirectory(ImagePtr img, unsigned int which) {
    struct avi_internal* internal = (struct avi_internal*)img->handler->internal;
    if (internal->pFrame)
	AVIStreamGetFrameClose(internal->pFrame);
    if (internal->pStream)
	AVIStreamRelease(internal->pStream);
    if (AVIStreamOpenFromFile(&internal->pStream, 
			      img->name, 
			      streamtypeVIDEO,
			      0,
			      OF_READ, 
			      NULL) == 0) {
      unsigned int level = 1;
      unsigned int iwhich = img->numdirs - which - 1;
      unsigned int idx;
	AVISTREAMINFO psi;
	AVIStreamInfo(internal->pStream, &psi, sizeof(psi));
	internal->width = psi.rcFrame.right - psi.rcFrame.left;
	internal->height = psi.rcFrame.bottom - psi.rcFrame.top;
	internal->frames = psi.dwLength;
	if (internal->frames < columns) {
	    img->numtilesx = internal->frames;
	    img->numtilesy = 1;
	} else {
	    img->numtilesx = columns;
	    img->numtilesy = internal->frames / columns;
	    if ((img->numtilesx * img->numtilesy) < internal->frames)
		++img->numtilesy;
	}
	for (idx = 0; idx < iwhich; ++idx)
	  level *= 2;
	img->twidth = internal->width / level;
	img->theight = internal->height / level;
	img->width = img->numtilesx * img->twidth;
	img->height = img->numtilesy * img->theight;
	img->istiled = TRUE;
	img->subfiletype = Virtual;
	internal->pFrame = AVIStreamGetFrameOpen(internal->pStream, 
						 (LPBITMAPINFOHEADER)AVIGETFRAMEF_BESTDISPLAYFMT);
    }
}

static HBITMAP
LoadAVITile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
  unsigned int* bits = 0;
  HBITMAP hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, img->theight, &bits, 24);
  if (bits) {
      struct avi_internal* internal = (struct avi_internal*)img->handler->internal;
      LPBITMAPINFOHEADER lpbi;
      DWORD index = y * columns + x;
      if (index < internal->frames) {
	  lpbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(internal->pFrame, index);
	  if (lpbi) {
	    //	      RECT rect;
	    //	      char buffer[64];
	      BYTE* pdata = (BYTE*)lpbi+lpbi->biSize+lpbi->biClrUsed * sizeof(RGBQUAD);
	      HDC hdctemp = CreateCompatibleDC(hdc);
	      SelectObject(hdctemp, hbitmap);
	      SetStretchBltMode(hdctemp, HALFTONE);
	      StretchDIBits(hdctemp, 
			    0, 0, img->twidth, img->theight, 
			    0, 0, internal->width, internal->height,
			    pdata, 
			    (const BITMAPINFO *)lpbi, 
			    DIB_RGB_COLORS, SRCCOPY);
	      //	      wsprintf(buffer, "Frame %d", index);
	      //	      SetRect(&rect, 0, 0, img->twidth, img->theight);
	      //	      SetBkMode(hdctemp, TRANSPARENT);
	      //	      SetTextColor(hdctemp, RGB(255, 255, 255));
	      //	      DrawText(hdctemp, buffer, -1, &rect, DT_SINGLELINE |  DT_CENTER | DT_BOTTOM); 
	      DeleteDC(hdctemp);
	  } else {
	    //	      MessageBox(0, "AVIStreamGetFrame", "error", MB_OK);
	  }
      }
  }
  return hbitmap;
}

static void CloseAVIImage(ImagePtr img) {
    if (img->handler && img->handler->internal) {
	struct avi_internal* internal = (struct avi_internal*)img->handler->internal;
	AVIStreamGetFrameClose(internal->pFrame);
	AVIStreamRelease(internal->pStream);
	MYFREE(internal);
    }
    AVIFileExit();
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptAVIImage,
			 GetAVIDescription,
			 GetAVIExtension,
			 OpenAVIImage,
			 SetAVIDirectory,
			 LoadAVITile,
			 CloseAVIImage);
}

