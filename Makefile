OS = LINUX
#OS = MACOSX
#OS = MACOSX_CLANG
#OS = WINDOWS

ifeq ($(OS), LINUX)
ALL = MotionCal imuread
CC = gcc
CXX = g++
AR = ar
RANLIB = ranlib
CFLAGS = -O2 -Wall -D$(OS) -I./libcalib
WXCONFIG = wx-config
WXFLAGS = `$(WXCONFIG) --cppflags`
CXXFLAGS = $(CFLAGS) `$(WXCONFIG) --cppflags`
LDFLAGS =
SFLAG = -s
CLILIBS = -lglut -lGLU -lGL -lm
MAKEFLAGS = --jobs=12

else ifeq ($(OS), MACOSX)
ALL = MotionCal.dmg
CC = gcc
CXX = g++
CFLAGS = -O2 -Wall -D$(OS) -DGL_SILENCE_DEPRECATION -I./libcalib
AR = /usr/bin/ar
RANLIB = /usr/bin/ranlib
# brew install wxwidgets
WXCONFIG = wx-config
WXFLAGS = `$(WXCONFIG) --cppflags`
CXXFLAGS = -std=c++11 $(CFLAGS) `$(WXCONFIG) --cppflags`
SFLAG = 
# wx-config --libs opengl seems to take care of these now
#CLILIBS = -lglut -lGLU -lGL -lm
VERSION = 0.01

else ifeq ($(OS), MACOSX_CLANG)
ALL = MotionCal.app
CC = /usr/bin/clang
CXX = /usr/bin/clang++
AR = /usr/bin/ar
RANLIB = /usr/bin/ranlib
CFLAGS = -O2 -Wall -DMACOSX -DGL_SILENCE_DEPRECATION -I./libcalib
WXCONFIG = wx-config
WXFLAGS = `$(WXCONFIG) --cppflags`
CXXFLAGS = -std=c++11 $(CFLAGS) `$(WXCONFIG) --cppflags`
SFLAG =
# wx-config --libs opengl seems to take care of these now
#CLILIBS = -lglut -lGLU -lGL -lm
VERSION = 0.01

else ifeq ($(OS), WINDOWS)
ALL = MotionCal.exe
#MINGW_TOOLCHAIN = i586-mingw32msvc
MINGW_TOOLCHAIN = i686-w64-mingw32
CC = $(MINGW_TOOLCHAIN)-gcc
CXX = $(MINGW_TOOLCHAIN)-g++
AR = $(MINGW_TOOLCHAIN)-ar
RANLIB = $(MINGW_TOOLCHAIN)-ranlib
WINDRES = $(MINGW_TOOLCHAIN)-windres
CFLAGS = -O2 -Wall -D$(OS) -I./libcalib
WXFLAGS = `$(WXCONFIG) --cppflags`
CXXFLAGS = $(CFLAGS) $(WXFLAGS)
LDFLAGS = -static -static-libgcc
SFLAG = -s
#WXCONFIG = ~/wxwidgets/3.0.2.mingw-opengl-i586/bin/wx-config
#WXCONFIG = ~/wxwidgets/3.0.2.mingw-opengl/bin/wx-config
#WXCONFIG = ~/wxwidgets/3.1.0.mingw-opengl/bin/wx-config
WXCONFIG = ../wxWidgets-3.1.0/wx-config
CLILIBS = -lglut32 -lglu32 -lopengl32 -lm
MAKEFLAGS = --jobs=12
# ?= can detect empty vs undefined. thus it gets set to notwsl only when not defined at all
# then we can use ifdef to tell whether or not we are running under WSL.
WSLENV ?= notwsl

endif

LIB_OBJS = libcalib/libcalib.o
OBJS = visualize.o serialdata.o rawdata.o magcal.o matrix.o fusion.o quality.o mahony.o libcalib/libcalib.a
IMGS = checkgreen.png checkempty.png checkemptygray.png

all: $(ALL)

MotionCal: gui.o portlist.o images.o $(OBJS)
	$(CXX) $(SFLAG) $(CFLAGS) $(LDFLAGS) -o $@ $^ `$(WXCONFIG) --libs all,opengl` $(CLILIBS)

MotionCal.exe: resource.o gui.o portlist.o images.o $(OBJS)
	$(CXX) $(SFLAG) $(CFLAGS) $(LDFLAGS) -o $@ $^ `$(WXCONFIG) --libs all,opengl`
ifdef WSLENV
	# confusingly, if WSLENV is not empty, we are NOT inside WSL
	-pjrcwinsigntool $@
	-./cp_windows.sh $@
endif

resource.o: resource.rc icon.ico
	$(WINDRES) $(WXFLAGS) -o resource.o resource.rc

images.cpp: $(IMGS) png2c.pl
	perl png2c.pl $(IMGS) > images.cpp

MotionCal.app: MotionCal Info.plist icon.icns
	mkdir -p $@/Contents/MacOS
	mkdir -p $@/Contents/Resources/English.lproj
	sed "s/1.234/$(VERSION)/g" Info.plist > $@/Contents/Info.plist
	/bin/echo -n 'APPL????' > $@/Contents/PkgInfo
	cp $< $@/Contents/MacOS/
	cp icon.icns $@/Contents/Resources/
	-pjrcmacsigntool $@
	touch $@

MotionCal.dmg: MotionCal.app
	mkdir -p dmg_tmpdir
	cp -r $< dmg_tmpdir
	hdiutil create -ov -srcfolder dmg_tmpdir -megabytes 20 -format UDBZ -volname MotionCal $@

imuread: imuread.o $(OBJS)
	$(CC) -s $(CFLAGS) $(LDFLAGS) -o $@ $^ $(CLILIBS)

imuread: 

clean:
	rm -f gui MotionCal imuread *.o *.exe *.sign? images.cpp
	rm -f libcalib/*.o libcalib/*.a
	rm -rf MotionCal.app MotionCal.dmg .DS_Store dmg_tmpdir

gui.o: gui.cpp gui.h imuread.h Makefile
portlist.o: portlist.cpp gui.h Makefile
imuread.o: imuread.c imuread.h Makefile
visualize.o: visualize.c imuread.h Makefile
serialdata.o: serialdata.c imuread.h Makefile
rawdata.o: rawdata.c imuread.h Makefile
magcal.o: magcal.c imuread.h Makefile
matrix.o: matrix.c imuread.h Makefile
fusion.o: fusion.c imuread.h Makefile
quality.o: quality.c imuread.h Makefile
mahony.o: mahony.c imuread.h Makefile

libcalib/libcalib.o: libcalib/libcalib.cpp libcalib/libcalib.h Makefile

libcalib/libcalib.a: $(LIB_OBJS)
	$(AR) rc $@ $+
	$(RANLIB) $@

