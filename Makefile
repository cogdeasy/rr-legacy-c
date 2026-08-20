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
#  THE SOURCES ARE K AND R C.  A COMPILER THAT INSISTS ON PROTOTYPES
#  WILL NEED THE DIALECT SETTING IN CFLAGS BELOW.
#
#  R.T.H.  14-MAR-1988
#-----------------------------------------------------------------------
#

CC      = cc
CFLAGS  = -O -I include
LDFLAGS =
LIBS    =
PROG    = ehm

OBJS    = src/ehmmain.o \
          src/dbio.o \
          src/ehmcalc.o \
          src/wrkscd.o \
          src/sprprv.o \
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
src/sprprv.o:   src/sprprv.c   $(HDRS)
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
