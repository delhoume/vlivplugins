#include <vliv.h>
#include "ppmhandler.h"

struct ppm_internal {
    LONG pos;
    char type;
};

#define EOF -1

// from XV sources
static int GetInt(HANDLE handle) {
  char c;
  int i, firstchar;
  DWORD readbytes;
  /* note:  if it sees a '#' character, all characters from there to end of
     line are appended to the comment string */
  /* skip forward to start of next number */
  ReadFile(handle, &c, 1, &readbytes, 0);
  while (1) {
      /* eat comments */
      if (c == '#') {   /* if we're at a comment, read to end of line */
	  firstchar = 1;
	  while (1) {
	      ReadFile(handle, &c, 1, &readbytes, 0);
	      if (firstchar && c == ' ') firstchar = 0;  /* lop off 1 sp after # */
	      else {
		  if (c == '\n' || c == EOF) break;
	      }
	  }
      }
      if (c == EOF) return 0;
      if (c >= '0' && c <= '9') break;   /* we've found what we were looking for */
      /* see if we are getting garbage (non-whitespace) */
      if (c !=' ' && c != '\t' && c != '\r' && c != '\n' && c != ',');
      ReadFile(handle, &c, 1, &readbytes, 0);
  }
  /* we're at the start of a number, continue until we hit a non-number */
  i = 0;
  while (1) {
      i = (i * 10) + (c - '0');
      ReadFile(handle, &c, 1, &readbytes, 0);
      if (c == EOF) return i;
      if (c < '0' || c > '9') break;
  }
  return i;
}

static BOOL AcceptPPMImage(const unsigned char* buffer, unsigned int size) {
    if (size >= 4) {
	if ((buffer[0] == 'P') && ((buffer[1] == '5' || buffer[1] == '6')))
	    return TRUE;
    }
    return FALSE;
}

static const char* GetPPMDescription() { return "Portable Pixmap Images"; }
static const char* GetPPMExtension() { return "*.ppm;*.pgm"; }

static BOOL OpenPPMImage(ImagePtr img, const TCHAR* name) {
    HANDLE handle = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			       FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle) {
	DWORD readbytes;
	char c, c1;
	ReadFile(handle, &c, 1, &readbytes, 0);
	ReadFile(handle, &c1, 1, &readbytes, 0);
	if (c !='P' &&  c1 != '6') {
	    CloseHandle(handle);
	    return FALSE;
	} else {
	    struct ppm_internal* ppm_internal = (struct ppm_internal*)MYALLOC(sizeof(struct ppm_internal));
	    ppm_internal->type = c1;
	    img->handler->internal = (void*)ppm_internal;
	    img->numdirs = 1;
	    img->supportmt = 1;
	    img->currentdir = 0;
	    CloseHandle(handle);
	    return TRUE;
	}
    }
    return FALSE;
}

static const int ppmtilesize = 256;

static void SetPPMDirectory(ImagePtr img, unsigned int which) {
    struct ppm_internal* ppm_internal = (struct ppm_internal*)img->handler->internal;
    HANDLE handle = CreateFile(img->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle) {
	DWORD readbytes;
	char c;
	ReadFile(handle, &c, 1, &readbytes, 0);
	ReadFile(handle, &c, 1, &readbytes, 0);
	img->width = GetInt(handle);
	img->height = GetInt(handle);
	GetInt(handle); // skip maxvalue
	img->twidth = ppmtilesize;
	img->theight = ppmtilesize;
	img->istiled = TRUE;
	img->numtilesx = (img->width / ppmtilesize);
	if ((img->numtilesx * ppmtilesize) < img->width)
	    ++img->numtilesx;
	img->numtilesy = (img->height / ppmtilesize);
	if ((img->numtilesy * ppmtilesize) < img->height)
	    ++img->numtilesy;
	ppm_internal->pos = SetFilePointer(handle, 0L, 0, FILE_CURRENT);
	CloseHandle(handle);
    }
}

static HBITMAP LoadPPMTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    struct ppm_internal* ppm_internal = (struct ppm_internal*)img->handler->internal;
    HANDLE handle = CreateFile(img->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HBITMAP hbitmap = 0;
    if (handle) {
	unsigned int* bits = 0;
#if 0
	char buffer[100];
	wsprintf(buffer, "%dx%d", x, y);
	hbitmap = VlivCreateDefaultDIBSection(hdc, img->twidth, img->theight, buffer, &bits);
	CloseHandle(handle);
	return;
#endif
	if (ppm_internal->type == '6') 
	    hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 24);
	else
	    hbitmap = img->helper.CreateIndexedDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 0, 0);
	if (bits) {
	    int scanlinesize;
	    unsigned int i, j;
	    unsigned int starty = y * img->theight;
	    unsigned int startx = x * img->twidth;
	    unsigned int readwidth = img->twidth;
	    DWORD readbytes;
	    unsigned int bpp = ppm_internal->type == '6' ? 3 : 1;
	    LONG posinfile = starty * img->width * bpp + startx * bpp;
	    LONG bytestoseek = (img->width - img->twidth) * bpp;
	    if ((startx + ppmtilesize) > img->width) {
		readwidth = img->width % ppmtilesize;
		bytestoseek = startx * bpp;
	    }
	    if (ppm_internal->type == '6') {
		scanlinesize = img->twidth * 24 / 8;
		while ((scanlinesize % 4) != 0)
		    ++scanlinesize;
	    } else {
		scanlinesize = (((img->twidth * 8) + 31) & ~31) >> 3;
	    }
	    SetFilePointer(handle, ppm_internal->pos, 0, FILE_BEGIN);
	    SetFilePointer(handle, posinfile, 0, FILE_CURRENT);
	    for (j = 0; (j < img->theight) && (starty < img->height); ++j) {
		char* dst = (char*)bits + j * scanlinesize;
		ReadFile(handle, (LPVOID)dst, readwidth * bpp, &readbytes, 0);
		// now convert;
		if (ppm_internal->type == '6') {
		    for (i = 0; i < readwidth; ++i) {
			char r = dst[3 * i];
			dst[3 * i] = dst[3 * i + 2];
			dst[3 * i + 2] = r;
		    }
		} 
		SetFilePointer(handle, bytestoseek, 0, FILE_CURRENT);
	    }
	}
	CloseHandle(handle);
    }
    return hbitmap;
}

static void ClosePPMImage(ImagePtr img) {
    if (img->handler && img->handler->internal) {
	struct ppm_internal* ppm_internal = (struct ppm_internal*)img->handler->internal;
	MYFREE(ppm_internal);
    }
}

void RegisterPPMHandler(ImagePtr img) {
    img->helper.Register(AcceptPPMImage,
			 GetPPMDescription,
			 GetPPMExtension,
			 OpenPPMImage,
			 SetPPMDirectory,
			 LoadPPMTile,
			 ClosePPMImage);
}

