#define WIN32_LEAN_AND_MEAN 
#include <qoihandler.h>
#define QOI_IMPLEMENTATION
#define QOI_NO_STDIO
#define QOI_MALLOC MYALLOC
#define QOI_FREE MYFREE
#define QOI_ZEROARR
#include <qoi.h>

#include <fileapi.h>

static BOOL AcceptQOIImage(const unsigned char* buffer, unsigned int size) { 
     if (size >= 2) {
        if ((buffer[0] == 'q') && (buffer[1] == 'o') &&
            (buffer[2] == 'i') && (buffer[1] == 'f'))
            return TRUE;
    }
    return FALSE; 
}

const char* GetQOIDescription() { return "QOI Images"; }
const char* GetQOIExtension() { return "*.qoi"; }

struct qoi_internal {
    unsigned char channels;
    unsigned char colorspace;
};

static BOOL OpenQOIImage(ImagePtr img, const TCHAR* name) {
    struct qoi_internal* new_internal = (struct qoi_internal*)MYALLOC(sizeof(struct qoi_internal));
    img->handler->internal = (void*)new_internal;
    img->numdirs = 1;
    img->supportmt = 0;
    img->currentdir = 0;
    return TRUE;
}

static void SetQOIDirectory(ImagePtr img, unsigned int which) {
    struct qoi_internal* qoi_internal = (struct qoi_internal*)img->handler->internal;
   HANDLE file = CreateFile(img->name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file) {
        unsigned char header[QOI_HEADER_SIZE];
        int p = 0;
		ReadFile(file, (char*)header, QOI_HEADER_SIZE, 0, 0);		
        qoi_read_32(header, &p);
        img->width = qoi_read_32(header, &p);
        img->height  = qoi_read_32(header, &p);
        qoi_internal->channels = header[p++];
        qoi_internal->colorspace = header[p++];
        img->numtilesx = 1;
		img->numtilesy = 1;
		img->twidth = img->width;
		img->theight = img->height;
		img->subfiletype = Normal;
        img->istiled = FALSE;
		CloseHandle(file);
    }
 }

void *fload(const char *path, int *out_size) {
   HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			       FILE_ATTRIBUTE_NORMAL, NULL);
	if (file) {
	 int size = GetFileSize(file, 0);
	void* buffer = MYALLOC(size);
	ReadFile(file, (char*)buffer, size, 0, 0);
	CloseHandle(file);
	    *out_size = size;        
	    return buffer;
    } else {
        *out_size = 0;
        return 0;
    }
}

#define BLEND(x, y, a) (y + (((x-y) * a)>>8))
//#define BLEND(x, y, a) ((x * a) + (y * (255 - a)))>>8

unsigned int pot = 5;
inline unsigned char Checker(unsigned int x, unsigned int y) {
    return  (((x^y)>>pot)&1)-1 == 0 ? 255 : 150;
}

static HBITMAP
LoadQOITile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    struct qoi_internal* qoi_internal = (struct qoi_internal*)img->handler->internal;
    HBITMAP hbitmap = 0;    
    unsigned int* bits = 0;
    hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 32);
    if (bits) {
        int size;
        void* bytes = fload(img->name, &size);
        if (bytes) {
            qoi_desc desc;
            void *decp = qoi_decode(bytes, size, &desc, qoi_internal->channels);
            MYFREE(bytes);
            if (decp) {
                unsigned char* dst = (unsigned char*)bits;
                unsigned char* src = (unsigned char*)decp;
                switch (desc.channels) {
                   case 3: {
                       for (unsigned int y = 0; y < desc.height; ++y) {
                           for (unsigned int x = 0; x < desc.width; ++x, dst += 4, src += 3) {
                               // RGB to BGRA
                               dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; // dst[3] = 0xff;
                           }
                       }
                   }
                   break;
                   case 4:
                        for (unsigned int y = 0; y < desc.height; ++y) {
                            for (unsigned int x = 0; x < desc.width; ++x, dst += 4, src += 4) {
                                unsigned char background = Checker(x, y);
                                unsigned char alpha = src[3];
                                // RGBA to BGRA
                                dst[0] = BLEND(src[2], background, alpha);
                                dst[1] = BLEND(src[1], background, alpha); 
                                dst[2] = BLEND(src[0], background, alpha);
                                // dst[3] = 0xff;
                            }
                        }
                    break;
               }
                MYFREE(decp);
            } 
        }
    }
    return hbitmap;
}

static void CloseQOIImage(ImagePtr img) {
    if (img->handler && img->handler->internal) {
        struct qoi_internal* qoi_internal = (struct qoi_internal*)img->handler->internal;
        MYFREE(qoi_internal);
    }
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptQOIImage,
			 GetQOIDescription,
			 GetQOIExtension,
			 OpenQOIImage,
			 SetQOIDirectory,
			 LoadQOITile,
			 CloseQOIImage);
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                   DWORD  ul_reason_for_call, 
                   LPVOID lpReserved
                 ) {
    return TRUE;
}
