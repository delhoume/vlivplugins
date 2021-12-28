#include <jpeg2000.h>

#include <jasper/jasper.h>
#include <windows.h>

static BOOL AcceptJPEG2000Image(const unsigned char* buffer, unsigned int size) { 
  if (size >= 4) { // JPC
    if ((buffer[0] == 0xff) && (buffer[1] == 0x4f) && (buffer[2] == 0xff) && (buffer[3] == 0x51))
      return TRUE;
  }
  if (size >= 12) {  // JP2
    if ((buffer[0] == 0x00) && (buffer[1] == 0x00) && (buffer[2] == 0x00) && (buffer[3] == 0x0c) &&
	(buffer[4] == 0x6a) && (buffer[5] == 0x50) && (buffer[6] == 0x20) && (buffer[7] == 0x20) &&
	(buffer[8] == 0x0d) && (buffer[9] == 0x0a) && (buffer[10] == 0x87) && (buffer[11] == 0x0a)) 
      return TRUE;
  }
  return FALSE; 
}

const char* GetJPEG2000Description() { return "JPEG2000 images"; }
const char* GetJPEG2000Extension() { return "*.jp2;*.jpc"; }

struct jpeg2000_internal {
  jas_stream_t *instream;
  jas_image_t *image;
  int fmtid;
};

static BOOL OpenJPEG2000Image(ImagePtr img, const TCHAR* name) {
    struct jpeg2000_internal* internal = (struct jpeg2000_internal*)MYALLOC(sizeof(struct jpeg2000_internal));
    jas_stream_t *instream;
    jas_init();
    instream = jas_stream_fopen(name, "rb");
    if (instream != 0) {
      int fmtid = jas_image_getfmt(instream);
      if (fmtid >= 0) {
	img->handler->internal = (void*)internal;
	img->numdirs = 1;
	img->supportmt = 0;
	img->currentdir = 0;
	internal->instream = instream;
	internal->fmtid = fmtid;
	return TRUE;
      }
    }
    return FALSE;
}

// only one directory, get image dimensions, should not have to decode all...
static void SetJPEG2000Directory(ImagePtr img, unsigned int which) {
  struct jpeg2000_internal* internal = (struct jpeg2000_internal*)img->handler->internal;
  jas_stream_t *instream = internal->instream;  
  if (instream != 0) {
    internal->image = jas_image_decode(instream, internal->fmtid, 0);
    if (internal->image != 0) {
      img->width = jas_image_width(internal->image);
      img->height = jas_image_height(internal->image);
      img->twidth = img->width;
      img->theight = img->height;
      img->numtilesx = 1;
      img->numtilesy = 1;
      img->istiled = FALSE;
    }
  }
}

static HBITMAP
LoadJPEG2000Tile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
  struct jpeg2000_internal* internal = (struct jpeg2000_internal*)img->handler->internal;
  unsigned int* bits = 0;
  HBITMAP hbitmap = 0;
  jas_image_t * image = internal->image;
  int cmptlut[3];
  unsigned int numcomponents;
  unsigned int scanlinesize;
  // only support RGB and GRAY
  switch(jas_clrspc_fam(jas_image_clrspc(image))) {
  case JAS_CLRSPC_FAM_RGB:
    cmptlut[0] = jas_image_getcmptbytype(image, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R));
    cmptlut[1] = jas_image_getcmptbytype(image, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G));
    cmptlut[2] = jas_image_getcmptbytype(image, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B));
    numcomponents = 3;
    scanlinesize = 4 * img->twidth;
    hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 32);
    break;
  case JAS_CLRSPC_FAM_GRAY:
    cmptlut[0] = jas_image_getcmptbytype(image, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));
    numcomponents = 1;
    scanlinesize = (((img->twidth * 8) + 31) & ~31) >> 3;
    hbitmap = img->helper.CreateIndexedDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 0, 0);
    break;
  default: 
    return img->helper.CreateDefaultDIBSection(hdc, img->twidth, (int)img->theight, "Unsupported", &bits);
  }
  if (bits) {
    unsigned int w, h;
    unsigned int i;
    jas_matrix_t *pixels[3];
    for (i = 0; i < numcomponents; ++i) {
      pixels[i] = jas_matrix_create(1, (unsigned int)img->twidth);
    }
    for (h = 0; h < img->theight; ++h) {
      unsigned char* start = (unsigned char*)bits + h * scanlinesize;
      for (i = 0; i < numcomponents; ++i) {
	jas_image_readcmpt(image, cmptlut[i], 0, h, img->twidth, 1, pixels[i]);
      }
      switch (numcomponents) {
      case 3:
	for (w = 0; w < img->width; ++w) {
	  start[4 * w + 0] = jas_matrix_getv(pixels[2], w);
	  start[4 * w + 1] = jas_matrix_getv(pixels[1], w);
	  start[4 * w + 2] = jas_matrix_getv(pixels[0], w);
	}
	break;
      case 1:
	for (w = 0; w < img->width; ++w) {
	  start[w] = jas_matrix_getv(pixels[0], w);
	}
	break;
      }
    }
    for (i = 0; i < numcomponents; ++i) {
      jas_matrix_destroy(pixels[i]);
    }
  }
  return hbitmap;
}

static void CloseJPEG2000Image(ImagePtr img) {
    if (img->handler && img->handler->internal) {
	struct jpeg2000_internal* internal = (struct jpeg2000_internal*)img->handler->internal;
	if (internal->image != 0) 
	  jas_image_destroy(internal->image);
	if (internal->instream != 0)
	  jas_stream_close(internal->instream);
	jas_image_clearfmts();
	MYFREE(internal);
    }
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptJPEG2000Image,
			 GetJPEG2000Description,
			 GetJPEG2000Extension,
			 OpenJPEG2000Image,
			 SetJPEG2000Directory,
			 LoadJPEG2000Tile,
			 CloseJPEG2000Image);
}

