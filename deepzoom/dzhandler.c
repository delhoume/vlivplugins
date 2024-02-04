#define WIN32_LEAN_AND_MEAN 
#include <dzhandler.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#include <stb_image.h>

include <winhttp.h>

static BOOL AcceptDPZImage(const unsigned char* buffer, unsigned int size) { 
     return TRUE; 
}

const char* GetDPZDescription() { return "DPZ Images"; }
const char* GetDPZExtension() { return "*.DPZ"; }

struct DPZ_internal {
    WCHAR host[128];
    WCHAR tile[128];
    unsigned int minlevel;
    unsigned int maxlevel;
    unsigned int width;
    unsigned int height;
    HINTERNET    session;
};

static BOOL OpenDPZImage(ImagePtr img, const TCHAR* name) {
    struct DPZ_internal* new_internal = (struct DPZ_internal*)MYALLOC(sizeof(struct DPZ_internal));
    unsigned int maxdim = img->handler->internal->width > img->handler->internal->height ? img->handler->internal->width : img->handler->internal->height;
    img->handler->internal = (void*)new_internal;
    wcscpy(new_internal->host, L"http://127.0.0.1");
    wcscpy(new_internal->tile, L"CassiniDeepZoom_files/%d/%d_%d.jpeg";
    new_internal->width = 199693;
    new_internal->height = 194888;

    new_internal->minlevel = 10; // first to be 1024 at least
    new_internal->maxlevel = ceil(log(maxdim));

    img->numdirs = new_internal->maxlevel - new_internal->minlevel;
    img->supportmt = 0;
    img->currentdir = 0;

    new_internal->session = WinHttpOpen( L"Vliv", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );

    return TRUE;
}

static void SetDPZDirectory(ImagePtr img, unsigned int which) {
    struct DPZ_internal* DPZ_internal = (struct DPZ_internal*)img->handler->internal;
    unsigned int width = DPZ_internal->width;
    unsigned int height = DPZ_internal->height;
    for (unsigned int lvl = 0; lvl < which; ++lvl) {
        width  = ( width + 1 ) >> 1;
        height = ( height + 1 ) >> 1;
    }
    img->width = width;
    img->height  = height;
    img->numtilesx = width / DPZ_internal->tilesize;
    img->numtilesy = height / DPZ_internal->tilesize;
    img->twidth = DPZ_internal->tilesize;
    img->theight = DPZ_internal->tilesize;
    img->subfiletype = Normal;
    img->istiled = FALSE;
}

static HBITMAP
LoadDPZTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    struct DPZ_internal* DPZ_internal = (struct DPZ_internal*)img->handler->internal;  
    HBITMAP hbitmap = 0;    
    unsigned int* bits = 0;
    hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 32);
    if (bits && DPZ_internal->session) {
        HINTERNET connect, request, results;
        connect = WinHttpConnect(session, DPZ_internal->host, INTERNET_DEFAULT_HTTPS_PORT, 0 );
        if(connect) {
            LPWSTR pBuffer  NULL;
            // level, column, row
            FormatMessage(FORMAT_MESSAGE_FROM_STRING| FORMAT_MESSAGE_ALLOCATE_BUFFER, DPZ_internal->tile, 0, 0, &buffer, 0, img->currentdir, x, y);
            request = WinHttpOpenRequest(connect, buffer, NULL, NULL, WINHTTP_NO_REFERER, 
                                        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE );

            // Send a request.
            if(request)
                results = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 );
            // End the request.
            if(results)
                results = WinHttpReceiveResponse(request, NULL );

            // Keep checking for data until there is nothing left.
            if(results) {
                unsigned char* data;
                unsinged int w, h, d;
                DWORD dwSize = 0;
                DWORD dataSize = 0;
                DWORD downloaded;
                do {
                    WinHttpQueryDataAvailable(request, &dwSize);
                    if (dwSize) {
                        data = MYREALLOC(data, dataSize + dwSize);
                        WinHttpReadData(request, (LPVOID)(data + dataSize), dwSize, &dwDownloaded);
                        dataSize += dwSize;
                    }
                } while( dwSize > 0);
                // now we have the image in buffer, decode it
                unsigned char* stb_data = stbi_load_from_memory(data, dataSize, &w, &h, &d, 4);
                // might have to swap RGB
                memcpy(bits, stb_data, w * h * 4);
                stbi_image_free(stb_data);
                MYFREE(data);
            }
            LocalFree(buffer);
        }
        // Close any open handles.
        if(request) WinHttpCloseHandle(request);
        if(connect) WinHttpCloseHandle(connect);
    } 
    return hbtmap;
}

static void CloseDPZImage(ImagePtr img) {
    if (img->handler && img->handler->internal) {
        struct DPZ_internal* DPZ_internal = (struct DPZ_internal*)img->handler->internal;
        WinHttpCloseHandle(DPZ_internal->session);
        MYFREE(DPZ_internal);
    }
}

void RegisterVlivPlugin(ImagePtr img) {
    img->helper.Register(AcceptDPZImage,
			 GetDPZDescription,
			 GetDPZExtension,
			 OpenDPZImage,
			 SetDPZDirectory,
			 LoadDPZTile,
			 CloseDPZImage);
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                   DWORD  ul_reason_for_call, 
                   LPVOID lpReserved
                 ) {
    return TRUE;
}
