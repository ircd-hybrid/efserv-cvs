CC=gcc
CFLAGS=-Wall -ggdb
BIN=efserv
LEX=lex
BISON=bison
MAINSO=efserv.so
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
LIBS=-lefence -ldl

all: efserv efserv.so help.txt
${BIN} : modules.o Makefile
	${CC} -rdynamic ${CFLAGS} ${LDFLAGS} modules.o ${LIBS} -o ${BIN}
modules.o : modules.c
	${CC} -c ${CFLAGS} modules.c
${MAINSO} : ${OBJS}
	${CC} -shared ${LDFLAGS} ${OBJS} -o ${MAINSO}

sconfig.tab.c sconfig.tab.h : sconfig.y
	${BISON} --defines sconfig.y
lex.yy.c : sconfig.l sconfig.tab.h
	${LEX} sconfig.l

%.o : %.c
	${CC} -c ${CFLAGS} $<
help.txt : help.txt.in makehelp.pl
	perl makehelp.pl

depend:
	${CC} -MM ${SRCS} >.depend

include .depend
