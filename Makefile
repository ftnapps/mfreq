#
# Makefile
# (c) 1994-2013 by Markus Reschke
#


# global variables
DIST = $(notdir ${CURDIR})
NAME = mfreq
VERSION = $(subst ${NAME}-,,${DIST})

# global defaults (change if desired)
#DEFINES = -DCFG_PATH="\"/etc/cfg-path\"" -DTMP_PATH="\"/tmp\""

# compiler flags
CC = gcc
CFLAGS = -Wall -g -I/usr/include -I. ${DEFINES}
LDFLAGS =

# libraries to link
LIBS =

# where to install stuff
PREFIX = /usr/local

# objects
OBJ_COMMON = log.o misc.o tokenizer.o
OBJ_MAIN = mfreq-index.o mfreq-list.o mfreq-srif.o 
OBJ_EXTRA = index.o req.o fts.o list.o
OBJECTS = ${OBJ_COMMON} ${OBJ_MAIN} ${OBJ_EXTRA}
OBJ_INDEX = mfreq-index.o index.o ${OBJ_COMMON}
OBJ_LIST = mfreq-list.o list.o index.o ${OBJ_COMMON}
OBJ_SRIF = mfreq-srif.o index.o req.o fts.o ${OBJ_COMMON}

# header files
HEADERS = common.h variables.h functions.h

# programs
PROGS = mfreq-index mfreq-list mfreq-srif


#
# built dynamically linked version (default action)
#

all: $(PROGS)

mfreq-index: ${OBJ_INDEX}
	${CC} -o ${@} ${OBJ_INDEX} ${LDFLAGS} ${LIBS}

mfreq-list: ${OBJ_LIST}
	${CC} -o ${@} ${OBJ_LIST} ${LDFLAGS} ${LIBS}

mfreq-srif: ${OBJ_SRIF} 
	${CC} -o ${@} ${OBJ_SRIF} ${LDFLAGS} ${LIBS}


#
# compile source objects
#

# rule for all c-files
${OBJECTS}: %.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c ${@:.o=.c}


#
# make distribution archive
#

dist: ${PROGS}
	rm -f *.tgz
	strip ${PROGS}
	cd ..; tar -czf ${DIST}/${DIST}.tgz \
          ${DIST}/*.h ${DIST}/*.c ${DIST}/Makefile \
          ${DIST}/README ${DIST}/CHANGES ${DIST}/*.pdf \
          ${DIST}/sample-cfg \
	  ${DIST}/mfreq-index ${DIST}/mfreq-srif ${DIST}/mfreq-list


#
# install
#

install: ${PROGS}
	strip ${PROGS}
	cp -f mfreq-index ${PREFIX}/bin
	cp -f mfreq-list ${PREFIX}/bin
	cp -f mfreq-srif ${PREFIX}/bin


#
# extras
#

clean:
	rm -f ${PROGS} *.o *.tgz


