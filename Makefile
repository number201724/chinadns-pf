
CC = cc
CFLAGS = -std=c99 -Wall -Wextra -O2

TARGET = chinadns-ng
SRCS = chinadns.c dnsutils.c dnlutils.c netutils.c realtime.c timer.c event.c radix.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

depend:
	mkdep ${CFLAGS} ${SRCS}

clean:
	rm -rf *.o ${TARGET}

${TARGET}: ${OBJS}
	${CC} -s -o ${TARGET} ${OBJS}
