CC  = cl
RC  = rc
LD  = link

VLIVDIR = ..\..\vliv\src

TIFF = tiff-4.3.0\libtiff	
TIFFFLAGS = /I$(TIFF)
TIFFLIB = $(TIFF)\libtiff.lib

ZLIB = zlib-1.2.11
ZLIBFLAGS = /I$(ZLIB)
ZLIBLIB = $(ZLIB)\zlib.lib

JPEG = libjpeg-turbo2.1.2
JPEGFLAGS = /I$(JPEG)\include
JPEGLIB = $(JPEG)\lib\turbojpeg-static.lib

PNG = lpng1637
PNGFLAGS = /I$(PNG)
PNGLIB = $(PNG)\libpng.lib

WEBP = libwebp-0.4.1-windows-x86
WEBPFLAGS = /I$(WEBP)\include
WEBPLIB = $(WEBP)\lib\libwebp.lib

ZSTD = zstd-1.5.2
ZSTDLIB  = $(ZSTD)\lib\zstd.lib

DEBUG=/Ox 
LDDEBUG=

#DEBUG=/Od /Zi
#LDDEBUG=/debug

CFLAGS = /nologo /W3 $(DEBUG) /MD /D_NO_CRT_STDIO_INLINE /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR)
LDFLAGS = $(LDDEBUG)

SYSLIBS = wininet.lib user32.lib gdi32.lib kernel32.lib comctl32.lib comdlg32.lib shlwapi.lib \
	  shell32.lib advapi32.lib version.lib strsafe.lib

CUSTOMLIBS = $(TIFFLIB) $(JPEGLIB) $(PNGLIB) $(ZLIBLIB) $(WEBPLIB)
OBJECTS = inthandler.obj jpghandler.obj ppmhandler.obj bmphandler.obj pnghandler.obj tifhandler.obj webphandler.obj

all: vliv.dll 

jpghandler.obj : jpghandler.c jpghandler.h
	$(CC) $(CFLAGS) $(JPEGFLAGS) /c jpghandler.c

ppmhandler.obj : ppmhandler.c ppmhandler.h
	$(CC) $(CFLAGS) /c ppmhandler.c

bmphandler.obj : bmphandler.c bmphandler.h
	$(CC) $(CFLAGS) /c bmphandler.c

pnghandler.obj : pnghandler.c pnghandler.h
	$(CC) $(CFLAGS) $(PNGFLAGS) $(ZLIBFLAGS) /c pnghandler.c

tifhandler.obj : tifhandler.c tifhandler.h
	$(CC) $(CFLAGS) $(TIFFFLAGS) $(ZLIBFLAGS) /c tifhandler.c

webphandler.obj : webphandler.c webphandler.h
	$(CC) $(CFLAGS) $(WEBPFLAGS) /c webphandler.c

inthandler.obj : inthandler.c inthandler.h
	$(CC) $(CFLAGS) /c inthandler.c

vliv.dll: $(OBJECTS)
	$(LD) $(LDFLAGS) /dll /out:vliv.dll $(OBJECTS) $(CUSTOMLIBS) $(SYSLIBS)

clean:
	del *.pdb *.ilk *.obj vliv.dll *~
