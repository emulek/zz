bin_PROGRAMS = zz

zz_SOURCES = \
encoder.cpp \
getopt.cpp \
huffman.cpp \
main.cpp \
matrix.cpp \
rbtree.cpp \
rnd.cpp \
rs.cpp \
zfile.cpp \
zstring.cpp \
encoder.hpp \
getopt.hpp \
huffman.hpp \
matrix.hpp \
rbtree.hpp \
rnd.hpp \
rs.hpp \
zfile.hpp \
zstring.hpp

noinst_HEADERS = gettext.h

AM_CXXFLAGS = -std=c++11 -Wall

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
