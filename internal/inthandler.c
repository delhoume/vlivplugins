#include <inthandler.h>

#include <tifhandler.h>
#include <jpghandler.h>
#include <bmphandler.h>
#include <pnghandler.h>
#include <ppmhandler.h>

void RegisterVlivPlugin(ImagePtr img) {
    RegisterJPGHandler(img);
    RegisterPPMHandler(img);
    RegisterBMPHandler(img);
    RegisterTIFHandler(img);
    RegisterPNGHandler(img);
}

