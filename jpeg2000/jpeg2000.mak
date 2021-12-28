CC  = cl
LD  = link

VLIVDIR = ..\..\vliv\src

DEBUG=/Ox 
#DEBUG=/Od /Zi

JASPER = jasper-1.900.1\src\libjasper
JASPERLIB = $(JASPER)\build\jasper.lib
JASPERFLAGS = /I$(JASPER)\include /DJAS_WIN_MSVC_BUILD

CFLAGS = /nologo /W3 $(DEBUG) /MD /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR)

SYSLIBS = user32.lib

all: jpeg2000.dll

jpeg2000.obj : jpeg2000.c jpeg2000.h
	$(CC) $(CFLAGS) $(JASPERFLAGS) /c jpeg2000.c

jpeg2000.dll: jpeg2000.obj
	$(LD) /dll /out:jpeg2000.dll jpeg2000.obj $(JASPERLIB) $(SYSLIBS)

clean:
	del jpeg2000.obj jpeg2000.dll
