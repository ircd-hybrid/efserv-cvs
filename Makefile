# Generated automatically from Makefile.in by configure.
CC=gcc
CP=/bin/cp
RM=/bin/rm
MKDIR=/bin/mkdir
CFLAGS=-Wall -ggdb -Iincludes/
BIN=efserv
LEX=flex
BISON=@BISON@
MAINSO=efserv.so
prefix=/home/andrew/scratch/efserv
DPATH=-DPREFIX=\"${prefix}/\"
IPATH=${prefix}
SRCS=channels.c\
     commands.c\
     config.c\
     clients.c\
     clones.c\
     database.c\
     efserv.c\
     log.c\
     match.c\
     md5.c\
     msg.c\
     utils.c \
     sconfig.tab.c \
     lex.yy.c
OBJS=${SRCS:%.c=%.o}
LIBS=-ldl

all: efserv efserv.so help.txt
${BIN} : modules.o Makefile
	${CC} -rdynamic ${CFLAGS} ${LDFLAGS} modules.o ${LIBS} -o ${BIN}
modules.o : modules.c
	${CC} -c ${DPATH} ${CFLAGS} modules.c
${MAINSO} : ${OBJS}
	${CC} -shared ${LDFLAGS} ${OBJS} -o ${MAINSO}

sconfig.tab.c sconfig.tab.h : sconfig.y
	${BISON} --defines sconfig.y
lex.yy.c : sconfig.l sconfig.tab.h
	${LEX} sconfig.l

%.o : %.c
	${CC} ${DPATH} -c ${CFLAGS} $<
help.txt : help.txt.in makehelp.pl
	perl makehelp.pl

depend:
	${CC} ${CFLAGS} -MM ${SRCS} >.depend

clean:
	${RM} -f *.o *.so core efserv.core efserv

install:
	${MKDIR} -p ${prefix} ${prefix}/etc
	${CP} ${BIN} ${IPATH}
	${CP} ${MAINSO} ${IPATH}
	@echo \-----------------------------------------
	@echo efserv can now be run as ${prefix}/${BIN}.
	@echo If you have not yet done so, please copy
	@echo example.conf to ${prefix}/etc/efserv.conf and
	@echo edit as needed.

include .depend
