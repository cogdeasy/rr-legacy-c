#!/bin/sh
#
#-----------------------------------------------------------------------
#  RUNTEST.SH   GOLDEN LISTING REGRESSION HARNESS FOR THE ENGINE HEALTH
#               MONITORING SYSTEM.
#
#               EVERY CASE RUNS THE PROGRAM AGAINST A CARD DECK HELD IN
#               TEST/DECK, WITH AN EXPLICIT RUN DATE SO THAT THE OUTPUT
#               DOES NOT DEPEND ON THE DAY THE TEST IS EXECUTED, AND
#               COMPARES THE PRINTED LISTING, THE RUN LOG AND THE EXIT
#               STATUS AGAINST THE GOLDEN COPIES IN TEST/GOLDEN.
#
#               THE WORK FILES ARE WRITTEN TO TEST/WORK WHICH IS NOT
#               PART OF THE SOURCE AND MAY BE DELETED AT ANY TIME.
#
#  USAGE        ./runtest.sh            RUN EVERY CASE
#               ./runtest.sh -g         REGENERATE THE GOLDEN FILES
#               ./runtest.sh -h         USAGE
#
#  EXIT STATUS  0  EVERY CASE PASSED
#               1  AT LEAST ONE CASE FAILED
#               2  THE HARNESS COULD NOT BE RUN AT ALL
#
#  THE HARNESS NEEDS NOTHING BUT A C COMPILER, MAKE AND A BOURNE SHELL.
#  KEEP IT IN THE STYLE OF EHMNITE.SH - NO ARRAYS, NO DOUBLE BRACKETS.
#-----------------------------------------------------------------------
#

EHMHOME=`dirname $0`
EHMHOME=`cd $EHMHOME && pwd`
DECK=$EHMHOME/test/deck
GOLD=$EHMHOME/test/golden
WORK=$EHMHOME/test/work
RUNDAY=960209

GENER=0
NPASS=0
NFAIL=0

case "$1" in
-g)
	GENER=1
	;;
-h)
	echo "USAGE - runtest.sh [-g REGENERATE THE GOLDEN FILES]"
	exit 0
	;;
"")
	;;
*)
	echo "RUNTEST - OPTION $1 NOT KNOWN" >&2
	exit 2
	;;
esac

if [ ! -x $EHMHOME/ehm ]
then
	echo "RUNTEST - PROGRAM NOT BUILT, RUN MAKE FIRST" >&2
	exit 2
fi
if [ ! -d $DECK ]
then
	echo "RUNTEST - CARD DECKS NOT FOUND IN $DECK" >&2
	exit 2
fi

rm -rf $WORK
mkdir $WORK || exit 2
if [ $GENER -ne 0 ]
then
	mkdir -p $GOLD || exit 2
fi

#-----------------------------------------------------------------------
#  REPORT  -  ONE PASS OR FAIL LINE PER CASE.  THE REASON IS PRINTED
#             AFTER THE CASE NAME SO THAT A FAILING RUN SAYS WHICH
#             PRODUCT OF WHICH CASE HAS MOVED.
#-----------------------------------------------------------------------

pass()
{
	NPASS=`expr $NPASS + 1`
	if [ $GENER -ne 0 ]
	then
		echo "GEN   $1"
	else
		echo "PASS  $1"
	fi
}

fail()
{
	NFAIL=`expr $NFAIL + 1`
	echo "FAIL  $1  -  $2"
}

#-----------------------------------------------------------------------
#  CHKFIL  -  COMPARE ONE PRODUCT AGAINST ITS GOLDEN COPY, OR WRITE THE
#             GOLDEN COPY WHEN GENERATING.  RETURNS 0 IF IT MATCHES.
#             $1 CASE NAME  $2 PRODUCT SUFFIX  $3 FILE PRODUCED
#-----------------------------------------------------------------------

chkfil()
{
	if [ ! -f $3 ]
	then
		: > $3
	fi
	if [ $GENER -ne 0 ]
	then
		cp $3 $GOLD/$1.$2
		return 0
	fi
	if [ ! -f $GOLD/$1.$2 ]
	then
		echo "NO GOLDEN FILE $GOLD/$1.$2, RUN runtest.sh -g" \
			> $WORK/$1/$2.diff
		cat $WORK/$1/$2.diff >&2
		return 1
	fi
	diff $GOLD/$1.$2 $3 > $WORK/$1/$2.diff 2>&1
	if [ $? -ne 0 ]
	then
		return 1
	fi
	return 0
}

#-----------------------------------------------------------------------
#  RUNCASE  -  RUN ONE CARD DECK THROUGH THE BATCH SEQUENCE AND CHECK
#              THE LISTING, THE RUN LOG AND THE EXIT STATUS.
#
#              $1 CASE NAME
#              $2 FLEET MASTER DECK
#              $3 FLIGHT REPORT DECK
#              $4 SLOT DECK
#              $5 EXPECTED EXIT STATUS
#
#              THE PROGRAM IS RUN IN ITS OWN WORK DIRECTORY BECAUSE THE
#              RUN LOG IS ALWAYS EHM.LOG IN THE CURRENT DIRECTORY AND IS
#              APPENDED TO, NOT OVERWRITTEN.  THE RUN DATE IS PASSED ON
#              THE COMMAND LINE, NOT TAKEN FROM THE CLOCK, SO THAT THE
#              LISTING IS THE SAME ON ANY DAY.
#-----------------------------------------------------------------------

runcase()
{
	CASE=$1
	WRK=$WORK/$CASE
	mkdir -p $WRK || exit 2

	( cd $WRK || exit 2
	  EHMDATA=
	  export EHMDATA
	  $EHMHOME/ehm -b -d $RUNDAY -f $2 -r $3 -s $4 -o ehm.lis \
		> ehm.out 2> ehm.err )
	RC=$?

	WHY=
	if [ $RC -ne $5 ]
	then
		WHY="EXIT STATUS $RC, EXPECTED $5"
	fi

	chkfil $CASE lis $WRK/ehm.lis
	if [ $? -ne 0 ]
	then
		WHY="${WHY}${WHY:+, }LISTING DIFFERS, SEE $WRK/lis.diff"
	fi
	chkfil $CASE log $WRK/ehm.log
	if [ $? -ne 0 ]
	then
		WHY="${WHY}${WHY:+, }RUN LOG DIFFERS, SEE $WRK/log.diff"
	fi

	if [ -n "$WHY" ]
	then
		fail $CASE "$WHY"
	else
		pass $CASE
	fi
}

#-----------------------------------------------------------------------
#  DECKCASE  -  RUNCASE WITH THE THREE DECKS NAMED IN THE USUAL ORDER.
#               $1 CASE  $2 FLEET  $3 FLIGHTS  $4 SLOTS  $5 STATUS
#-----------------------------------------------------------------------

deckcase()
{
	runcase $1 $DECK/$2 $DECK/$3 $DECK/$4 $5
}

#-----------------------------------------------------------------------
#  STATCASE  -  CHECK AN EXIT STATUS ONLY.  USED FOR THE CASES WHERE
#               THERE IS NO LISTING TO COMPARE, OR WHERE THE LISTING
#               CARRIES THE CLOCK DATE AND SO CANNOT BE COMPARED.
#               $1 CASE NAME  $2 EXPECTED STATUS  $3... COMMAND
#-----------------------------------------------------------------------

statcase()
{
	CASE=$1
	WANT=$2
	shift
	shift
	WRK=$WORK/$CASE
	mkdir -p $WRK || exit 2

	( cd $WRK || exit 2
	  EHMDATA=
	  export EHMDATA
	  "$@" > ehm.out 2> ehm.err )
	RC=$?

	if [ $RC -ne $WANT ]
	then
		fail $CASE "EXIT STATUS $RC, EXPECTED $WANT"
	else
		pass $CASE
	fi
}

#-----------------------------------------------------------------------
#  NITECASE  -  RUN THE OVERNIGHT JOB ITSELF OVER A DECK COPIED INTO A
#               PRIVATE DATA DIRECTORY.  THE JOB TAKES THE RUN DATE FROM
#               THE CLOCK SO ONLY THE DOCUMENTED EXIT STATUS IS CHECKED.
#               $1 CASE  $2 FLEET  $3 FLIGHTS  $4 SLOTS  $5 STATUS
#-----------------------------------------------------------------------

nitecase()
{
	CASE=$1
	WRK=$WORK/$CASE
	mkdir -p $WRK/data || exit 2
	cp $EHMHOME/ehm $WRK/ehm || exit 2
	cp $EHMHOME/ehmnite.sh $WRK/ehmnite.sh || exit 2
	cp $DECK/$2 $WRK/data/fleet.dat || exit 2
	cp $DECK/$3 $WRK/data/flights.dat || exit 2
	cp $DECK/$4 $WRK/data/slots.dat || exit 2

	( cd $WRK || exit 2
	  EHMHOME=$WRK
	  EHMDATA=$WRK/data
	  EHMPRT=
	  export EHMHOME EHMDATA EHMPRT
	  sh $EHMHOME/ehmnite.sh > nite.out 2> nite.err )
	RC=$?

	if [ $RC -ne $5 ]
	then
		fail $CASE "EXIT STATUS $RC, EXPECTED $5"
	else
		pass $CASE
	fi
}

#-----------------------------------------------------------------------
#  THE CASES.  THE EXPECTED EXIT STATUS OF THE PROGRAM IS THE ERROR
#  COUNT, SEE SECTION 3 OF THE README, SO A CLEAN CASE IS ZERO.
#-----------------------------------------------------------------------

echo "EHM/MRO GOLDEN LISTING REGRESSION HARNESS, RUN DATE $RUNDAY"
echo ""

deckcase normal       fleet-normal.dat  flights-normal.dat  slots-normal.dat  0
deckcase status       fleet-status.dat  flights-status.dat  slots-normal.dat  0
deckcase alert-egtm   fleet-egtm.dat    flights-egtm.dat    slots-normal.dat  0
deckcase alert-rate   fleet-rate.dat    flights-rate.dat    slots-normal.dat  0
deckcase alert-vib    fleet-vib.dat     flights-vib.dat     slots-normal.dat  0
deckcase alert-oil    fleet-oil.dat     flights-oil.dat     slots-normal.dat  0
deckcase alert-llp    fleet-llp.dat     flights-llp.dat     slots-normal.dat  0
deckcase alert-due    fleet-due.dat     flights-due.dat     slots-normal.dat  0
deckcase alert-multi  fleet-multi.dat   flights-multi.dat   slots-normal.dat  0
deckcase noalert      fleet-clean.dat   flights-clean.dat   slots-normal.dat  1
deckcase slot-spare   fleet-normal.dat  flights-normal.dat  slots-surplus.dat 0
deckcase slot-zero    fleet-normal.dat  flights-normal.dat  slots-zero.dat    0
deckcase slot-none    fleet-normal.dat  flights-normal.dat  slots-empty.dat   0
deckcase bound        fleet-bound.dat   flights-bound.dat   slots-surplus.dat 0
deckcase reject       fleet-reject.dat  flights-reject.dat  slots-reject.dat  7

#  THE DATA SET NAMES BELOW ARE RELATIVE SO THAT THE MESSAGE IN THE RUN
#  LOG DOES NOT CARRY THE NAME OF THE DIRECTORY THE TESTS WERE RUN FROM.

runcase nofleet  no-such-file.dat $DECK/flights-normal.dat $DECK/slots-normal.dat 1
runcase noslots  $DECK/fleet-normal.dat $DECK/flights-normal.dat no-such-file.dat 1

#  OPERATOR ERRORS.  THE PROGRAM CANNOT BE RUN AND EXITS TWO.

statcase badopt  2 $EHMHOME/ehm -b -z
statcase baddate 2 $EHMHOME/ehm -b -d 96020
statcase badarg  2 $EHMHOME/ehm -b fleet.dat
statcase noval   2 $EHMHOME/ehm -b -o
statcase usage   0 $EHMHOME/ehm -h

#  THE OVERNIGHT JOB AND ITS THREE DOCUMENTED EXIT STATUSES.

nitecase nite-ok      fleet-normal.dat flights-normal.dat slots-normal.dat 0
nitecase nite-errors  fleet-reject.dat flights-reject.dat slots-reject.dat 1
statcase nite-nobuild 2 sh $EHMHOME/ehmnite.sh

echo ""
if [ $GENER -ne 0 ]
then
	echo "GOLDEN FILES REGENERATED IN $GOLD"
	echo "CHECK THE DIFFERENCES BEFORE COMMITTING THEM."
	if [ $NFAIL -ne 0 ]
	then
		echo "$NFAIL CASE(S) EXITED WITH AN UNEXPECTED STATUS.  THE"
		echo "EXPECTED STATUSES ARE HELD IN THIS SCRIPT, NOT IN THE"
		echo "GOLDEN FILES, AND MUST BE AMENDED BY HAND."
		exit 1
	fi
	exit 0
fi

echo "CASES RUN `expr $NPASS + $NFAIL`      PASSED $NPASS      FAILED $NFAIL"

if [ $NFAIL -ne 0 ]
then
	exit 1
fi

exit 0
