#

OBJ	= umenu.o \
	  list.o

LIBS	= -lcurses

CFLAGS	= -O2

NAME	= umenu

all: ${OBJ}
	cc ${LIBS} ${OBJ} -o ${NAME}

install: all
	 su -c "cp ./umenu /inemabin/umenu"
clean:
	rm -f ${OBJ}

fclean: clean
	rm -f ${NAME}

re: clean all
