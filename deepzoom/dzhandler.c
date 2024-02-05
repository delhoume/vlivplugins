#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <dzhandler.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#include <stb_image.h>

#include <winhttp.h>

static BOOL AcceptDPZImage(const unsigned char *buffer, unsigned int size)
{
    return TRUE;
}

const char *GetDPZDescription() { return "Deep Zoom Images"; }
const char *GetDPZExtension() { return "*.DZI"; }

struct DPZ_internal
{
    WCHAR host[128];
    WCHAR tile[128];
    unsigned int minlevel;
    unsigned int maxlevel;
    unsigned int width;
    unsigned int height;
    unsigned int tilesize;
    HINTERNET session;
};

static void ShowLastError(LPWSTR where) [
     LPWSTR messageBuffer = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    MessageBox(0, messageBuffer, messageBuffer, MB_OK);
    LocalFree(messageBuffer);
}


static BOOL OpenDPZImage(ImagePtr img, const TCHAR *name) {
    struct DPZ_internal *new_internal = (struct DPZ_internal *)MYALLOC(sizeof(struct DPZ_internal));
    unsigned int maxdim;
    img->handler->internal = (void *)new_internal;

    // hardcoded until we have a JSON / XML parser
    wcscpy(new_internal->host, L"127.0.0.1");
    wcscpy(new_internal->tile, L"/Cassini/CassiniDeepZoom_files/%1/%2_%3.jpeg");
    new_internal->width = 199693;
    new_internal->height = 194888;
    new_internal->tilesize = 512;
    maxdim = (new_internal->width > new_internal->height) ? new_internal->width : new_internal->height;
    new_internal->minlevel = 10; // first to be 1024 at least
    new_internal->maxlevel = ceil(log(maxdim));

    img->numdirs = 1; // new_internal->maxlevel - new_internal->minlevel;
    img->supportmt = 0;
    img->currentdir = 0;

    new_internal->session = WinHttpOpen(L"Vliv", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    return TRUE;
}

static void SetDPZDirectory(ImagePtr img, unsigned int which) {
    struct DPZ_internal *DPZ_internal = (struct DPZ_internal *)img->handler->internal;
    unsigned int width = DPZ_internal->width;
    unsigned int height = DPZ_internal->height;
     for (unsigned int lvl = 0; lvl < which; ++lvl) {
         width  = ( width + 1 ) >> 1;
         height = ( height + 1 ) >> 1;
    }
    img->width = width;
    img->height = height;
    img->istiled = TRUE;
    img->numtilesx = width / DPZ_internal->tilesize;
    img->numtilesy = height / DPZ_internal->tilesize;
    img->twidth = DPZ_internal->tilesize;
    img->theight = DPZ_internal->tilesize;
    img->subfiletype = Normal;
}

static HBITMAP
LoadDPZTile(ImagePtr img, HDC hdc, unsigned int x, unsigned int y) {
    struct DPZ_internal *DPZ_internal = (struct DPZ_internal *)img->handler->internal;
    HBITMAP hbitmap = 0;
    unsigned int *bits = 0;
    hbitmap = img->helper.CreateTrueColorDIBSection(hdc, img->twidth, -(int)img->theight, &bits, 32);
    if (bits && DPZ_internal->session)
    {
        HINTERNET connect, request;
        connect = WinHttpConnect(DPZ_internal->session, DPZ_internal->host, 8000, 0); // INTERNET_DEFAULT_HTTP_PORT, 0);
        if (connect) {
            LPWSTR pBuffer = NULL;
            // level, column, row
            unsigned int level = DPZ_internal->maxlevel - img->currentdir;
            DWORD_PTR pArgs[] = {(DWORD_PTR)level, (DWORD_PTR)x, (DWORD_PTR)y}; // FORMAT_MESSAGE_ARGUMENT_ARRAY
            if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                            DPZ_internal->tile, 0, 0,(LPWSTR)&pBuffer, 0, pArgs) == 0) {
                ShowLastError("buildin' URL string");
                MessageBox(0, pBuffer, "Url", MB_OK);
            } else {
                request = WinHttpOpenRequest(connect, L"GET", DPZ_internal->tile, NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
                LocalFree(pBuffer);
                // Send a request.
                if (request) {
                    BOOL ret = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
                    // End the request.
                    if (ret)  {
                        ret = WinHttpReceiveResponse(request, NULL);
                        // Keep checking for data until there is nothing left.
                        if (ret) {
                            unsigned char *data;
                            unsigned int w, h, d;
                            DWORD dwSize = 0;
                            DWORD dataSize = 0;
                            DWORD downloaded;
                            do {
                                WinHttpQueryDataAvailable(request, &dwSize);
                                if (dwSize) {
                                    data = MYREALLOC(data, dataSize + dwSize);
                                    WinHttpReadData(request, (LPVOID)(data + dataSize), dwSize, &downloaded);
                                    dataSize += dwSize;
                                }
                            } while (dwSize > 0);
                            // now we have the image in buffer, decode it
                            unsigned char *stb_data = stbi_load_from_memory(data, dataSize, &w, &h, &d, 4);
                            // might have to swap RGB
                            memcpy(bits, stb_data, w * h * 4);
                            stbi_image_free(stb_data);
                            MYFREE(data);
                        } else  {
                            ShowLastError(L"WinHttpReceiveResponse");
                        }
                    } else {
                        ShowLastError(L"WinHttpSendRequest");
                    }
                } else {
                    ShowLastError(L"WinHttpOpenRequest");
                }
            } else {
                ShowLastError("WinHttpConnect");
            }
        }
        // Close any open handles.
        if (request)
            WinHttpCloseHandle(request);
        if (connect)
            WinHttpCloseHandle(connect);
    }
    return hbitmap;
}

static void CloseDPZImage(ImagePtr img)
{
    if (img->handler && img->handler->internal)
    {
        struct DPZ_internal *DPZ_internal = (struct DPZ_internal *)img->handler->internal;
        WinHttpCloseHandle(DPZ_internal->session);
        MYFREE(DPZ_internal);
    }
}

void RegisterVlivPlugin(ImagePtr img)
{
    img->helper.Register(AcceptDPZImage,
                         GetDPZDescription,
                         GetDPZExtension,
                         OpenDPZImage,
                         SetDPZDirectory,
                         LoadDPZTile,
                         CloseDPZImage);
}

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved)
{
    return TRUE;
}
