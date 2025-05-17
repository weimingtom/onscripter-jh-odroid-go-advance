# -*- Makefile -*-
#
# Makefile.Linux - Makefile rules for linux
#

#copy this file from Makefile.Linux
#
#sudo apt-get install libsdl2-dev
#sudo apt-get install liblua5.1-0-dev
#sudo apt-get install libsdl2-image-dev 
#sudo apt-get install libsdl2-ttf-dev 
#sudo apt-get install libsdl2-mixer-dev 
#sudo apt-get install libbz2-dev
#sudo apt-get install libfontconfig1-dev
#sudo apt-get install libogg-dev
#sudo apt-get install libvorbis-dev
#

EXESUFFIX =
OBJSUFFIX = .o

.SUFFIXES:
.SUFFIXES: $(OBJSUFFIX) .cpp .h

#TARGET = onscripter$(EXESUFFIX) \
#	sardec$(EXESUFFIX) \
#	nsadec$(EXESUFFIX) \
#	sarconv$(EXESUFFIX) \
#	nsaconv$(EXESUFFIX) 
TARGET = onscripter$(EXESUFFIX)
EXT_OBJS = 

# mandatory: SDL, SDL_ttf, SDL_image, SDL_mixer, bzip2
DEFS = -DLINUX -DNDEBUG -DMIYOO
#-DUSE_SDL_RENDERER
INCS = `sdl2-config --cflags`
LIBS = `sdl2-config --libs` -lSDL2_ttf -lSDL2_image -lSDL2_mixer -lbz2 -lm
EXT_FLAGS = 

# recommended: smpeg
#DEFS += -DUSE_SMPEG
#INCS += `smpeg-config --cflags`
#LIBS += `smpeg-config --libs`

# recommended: fontconfig (to get default font)
DEFS += -DUSE_FONTCONFIG
LIBS += -lfontconfig

# recommended: OggVorbis 
DEFS += -DUSE_OGG_VORBIS
LIBS += -logg -lvorbis -lvorbisfile

# optional: Integer OggVorbis
#DEFS += -DUSE_OGG_VORBIS -DINTEGER_OGG_VORBIS
#LIBS += -lvorbisidec

# optional: support CD audio
#DEFS += -DUSE_CDROM

# optional: avifile
#DEFS += -DUSE_AVIFILE
#INCS += `avifile-config --cflags`
#LIBS += `avifile-config --libs`
#TARGET += simple_aviplay$(EXESUFFIX)
#EXT_OBJS += AVIWrapper$(OBJSUFFIX)

# optional: lua
#DEFS += -DUSE_LUA
#INCS += -I/usr/include/lua
#LIBS += -llua
#EXT_OBJS += LUAHandler$(OBJSUFFIX)

# optional: SIMD optimizing
#DEFS += -DUSE_SIMD -DUSE_SIMD_X86_SSE2

# optional: multicore rendering
DEFS += -DUSE_OMP_PARALLEL
EXT_FLAGS += -fopenmp

# optional: enable builtin effects
DEFS += -DUSE_BUILTIN_EFFECTS -DUSE_BUILTIN_LAYER_EFFECTS


# optional: enable English mode
#DEFS += -DENABLE_1BYTE_CHAR -DFORCE_1BYTE_CHAR


# for GNU g++
CC = g++ 
LD = g++ -o

#CFLAGS = -g -Wall -pipe -c $(INCS) $(DEFS)
CFLAGS = -std=c++11 -O0 -g3 -Wall -fomit-frame-pointer -pipe -c $(INCS) $(DEFS) $(EXT_FLAGS)

# for GCC on PowerPC specfied
#CC = powerpc-unknown-linux-gnu-g++
#LD = powerpc-unknown-linux-gnu-g++ -o

#CFLAGS = -O3 -mtune=G4 -maltivec -mabi=altivec -mpowerpc-gfxopt -mmultiple -mstring -Wall -fomit-frame-pointer -pipe -c $(INCS) $(DEFS)

# for Intel compiler
#CC = icc
#LD = icc -o

#CFLAGS = -O3 -tpp6 -xK -c $(INCS) $(DEFS)

RM = rm -f

include Makefile.onscripter
