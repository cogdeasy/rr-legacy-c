#!/bin/sh
#
#-----------------------------------------------------------------------
#  EHMNITE.SH   OVERNIGHT BATCH JOB FOR THE ENGINE HEALTH MONITORING
#               SYSTEM.  SUBMITTED FROM CRON AT 23:00.
#
#               THE JOB LOADS THE DATA SETS, SWEEPS THE FLEET, BUILDS
#               THE INDUCTION PLAN AND PRINTS THE LISTING.  THE LISTING
#               IS THEN SPOOLED TO THE OPERATIONS ROOM PRINTER IF ONE
#               IS DEFINED IN EHMPRT.
#
#  EXIT STATUS  0  NORMAL
#               1  THE PROGRAM REPORTED ERRORS, SEE THE RUN LOG
#               2  THE PROGRAM COULD NOT BE RUN AT ALL
#
#  R.T.H.  20-APR-1988
#-----------------------------------------------------------------------
#

EHMHOME=${EHMHOME:-`pwd`}
EHMDATA=${EHMDATA:-$EHMHOME/data}
EHMLIS=${EHMLIS:-$EHMHOME/ehm.lis}
export EHMDATA

cd $EHMHOME || exit 2

if [ ! -x ./ehm ]
then
	echo "EHM-950 PROGRAM NOT BUILT, RUN MAKE FIRST" >&2
	exit 2
fi

./ehm -b -o $EHMLIS
RC=$?

if [ $RC -ne 0 ]
then
	echo "EHM-951 OVERNIGHT RUN REPORTED $RC ERRORS" >&2
fi

if [ -n "$EHMPRT" ]
then
	lp -d $EHMPRT $EHMLIS || echo "EHM-952 SPOOL TO $EHMPRT FAILED" >&2
fi

#  KEEP FOURTEEN DAYS OF LISTINGS FOR THE CONTROLLERS.

if [ -d $EHMHOME/keep ]
then
	cp $EHMLIS $EHMHOME/keep/ehm.`date +%y%m%d`.lis
	find $EHMHOME/keep -name 'ehm.*.lis' -mtime +14 -exec rm -f {} \;
fi

if [ $RC -ne 0 ]
then
	exit 1
fi

exit 0
