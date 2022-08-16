CC  = cl
LD  = link

VLIVDIR = ..\..\vliv\src
STBDIR = ..\..\stb

DEBUG=/Ox 

CFLAGS = /EHsc /nologo /W3 $(DEBUG) /MD /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR) /I$(STBDIR)

all: exr.dll

exrhandler.obj : exrhandler.cpp exrhandler.h
	$(CC) $(CFLAGS) /c exrhandler.cpp

exr.dll: exrhandler.obj
	$(LD) /dll /map /out:exr.dll exrhandler.obj

clean:
	del exrhandler.obj exr.dll
