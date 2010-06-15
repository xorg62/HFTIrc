INCLUDES=	-I${PREFIX}/include
INCLIBIRCCLIENT=	-I${PREFIX}/include/libircclient

LIBDIR=		-I${PREFIX}/lib
LIBIRCCLIENTLIB=	-lircclient

CC?=	gcc
AR?=	ar
RANLIB?=	ranlib
