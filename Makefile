CC=gcc
CFLAGS=-Wall -ggdb
BIN=efserv
SRCS=commands.c\
     config.c\
     efserv.c\
     utils.c
OBJS=${SRCS:%.c=%.o}
LIBS=-lefence

all: efserv
${BIN} : ${OBJS}
	${CC} ${LIBS} ${LDFLAGS} ${OBJS} ${LIBS} -o ${BIN}
%.o : %.c
	${CC} -c ${CFLAGS} $<

depend:
	${CC} -MM ${SRCS} >.depend

include .depend
