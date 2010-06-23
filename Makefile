# HFTIRC Makefile
PREFIX?=	/usr/local

include config.mk
include config.local.mk

EXEC=hftirc
SRC= parse/parse.c \
     config.c      \
     ui.c          \
     irc.c         \
     event.c       \
     hftirc.c      \
     util.c        \
     input.c       \
     hftirc.h

CFLAGS+=	${INCLUDES} ${NCURSES5INCDIR}
CFLAGS+=	-Wall -g -D_XOPEN_SOURCE_EXTENDED -D_XOPEN_SOURCE -D_XOPEN_CURSES

LDFLAGS+=	${LIBDIR} ${NCURSES5LIBDIR}

OBJ=$(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	@echo "LINK	$@"
	@$(CC) -o $@ $(OBJ) $(LDFLAGS) $(CFLAGS)

.c.o:
	@echo "CC	$<"
	@$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o parse/*.o irc/*.o

mrproper: clean
	rm -rf $(EXEC)
