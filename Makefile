CC=gcc
CFLAGS=-Wall -ggdb
BIN=efserv
SRCS=commands.c\
     config.c\
     efserv.c\
     utils.c
OBJS=${SRCS:%.c=%.o}

all: efserv
${BIN} : ${OBJS}
	${CC} ${LDFLAGS} ${LIBS} ${OBJS} -o ${BIN}
%.o : %.c
	${CC} -c ${CFLAGS} $<

