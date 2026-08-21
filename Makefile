#
#-----------------------------------------------------------------------
#  MAKEFILE FOR THE ENGINE HEALTH MONITORING SYSTEM, RELEASE 4.2C
#
#  TARGETS
#       ehm         THE PROGRAM
#       run         RUN THE OVERNIGHT SEQUENCE AGAINST THE SAMPLE DATA
#       lint        RUN LINT OVER THE SOURCES, IF IT IS INSTALLED
#       clean       REMOVE THE OBJECT FILES AND THE PROGRAM
#
#  THE SOURCES ARE NOW PROTOTYPED C AND ARE BUILT TO THE 1999 STANDARD.
#  OVERRIDE CFLAGS FOR A SITE WHOSE COMPILER PREDATES IT, FOR EXAMPLE
#  MAKE CFLAGS="-O -I include -std=c89 -pedantic".
#
#  R.T.H.  14-MAR-1988
#  MOD 6                D.O'N.  PORTED OFF K AND R C, C99 BY DEFAULT.
#-----------------------------------------------------------------------
#

CC      = cc
CFLAGS  = -O -I include -std=c99 -Wall -Wextra
LDFLAGS =
LIBS    =
PROG    = ehm

OBJS    = src/ehmmain.o \
          src/dbio.o \
          src/ehmcalc.o \
          src/wrkscd.o \
          src/rptgen.o \
          src/strutl.o \
          src/errlog.o

HDRS    = include/ehm.h include/dbio.h

all:    $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

src/ehmmain.o:  src/ehmmain.c  $(HDRS)
src/dbio.o:     src/dbio.c     $(HDRS)
src/ehmcalc.o:  src/ehmcalc.c  $(HDRS)
src/wrkscd.o:   src/wrkscd.c   $(HDRS)
src/rptgen.o:   src/rptgen.c   $(HDRS)
src/strutl.o:   src/strutl.c   $(HDRS)
src/errlog.o:   src/errlog.c   $(HDRS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

run:    $(PROG)
	EHMDATA=data ./$(PROG) -b -d 960209 -o ehm.lis
	@echo "PRINT FILE WRITTEN TO ehm.lis"

lint:
	lint -I include src/*.c

clean:
	rm -f $(OBJS) $(PROG) ehm.lis ehm.log
