/*
 *----------------------------------------------------------------------
 *  EHMMAIN.C ENGINE HEALTH MONITORING SYSTEM - MAIN PROGRAM
 *
 *  USAGE
 *
 *      ehm                     RUN AT THE TERMINAL, MENU DRIVEN
 *      ehm -b                  RUN THE OVERNIGHT BATCH SEQUENCE
 *      ehm -d YYMMDD           SET THE RUN DATE, DEFAULT IS TODAY
 *      ehm -f FILE             FLEET MASTER OVERRIDE
 *      ehm -r FILE             FLIGHT REPORT FILE OVERRIDE
 *      ehm -s FILE             SLOT FILE OVERRIDE
 *      ehm -o FILE             SEND THE PRINTED OUTPUT TO FILE
 *
 *  THE BATCH SEQUENCE IS LOAD, SWEEP, SCHEDULE, PRINT ALL REPORTS.  IT
 *  IS DRIVEN FROM THE OVERNIGHT JOB EHMNITE.SH AND ITS EXIT STATUS IS
 *  THE ERROR COUNT, CAPPED AT 63 SO THAT THE SHELL CAN READ IT.
 *
 *  R.T.H.  14-MAR-1988
 *  MOD 5   09-FEB-1996  D.O'N.  RUN DATE TAKEN FROM THE SYSTEM CLOCK.
 *----------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "ehm.h"
#include "dbio.h"

static char fltfil[256];        /* FLEET MASTER NAME OVERRIDE          */
static char repfil[256];        /* FLIGHT REPORT FILE NAME OVERRIDE    */
static char sltfil[256];        /* SLOT FILE NAME OVERRIDE             */
static char outfil[256];        /* PRINT FILE NAME, EMPTY FOR STDOUT   */
static char cmd[128];           /* OPERATOR COMMAND LINE               */
static char work[132];

static FILE *prtfp = NULL;      /* PRINT STREAM                        */

/*
 *  SETDAY  -  SET THE RUN DATE FROM THE SYSTEM CLOCK.  THE SYSTEM HAS
 *             ALWAYS WORKED IN SIX DIGIT DATES, SEE THE NOTE IN
 *             STRUTL.C ABOUT THE CENTURY.
 */

static int setday(void)
{
	time_t clk;
	struct tm *t;
	char buf[40];

	clk = time((time_t *)0);
	t = localtime(&clk);
	sprintf(buf, "%02d%02d%02d",
		t->tm_year % 100, t->tm_mon + 1, t->tm_mday);
	memcpy(runday, buf, DATSIZ);
	runday[DATSIZ] = '\0';
	return (0);
}

/*
 *  OPNPRT  -  OPEN THE PRINT STREAM.  A FAILURE FALLS BACK TO THE
 *             TERMINAL, THE RUN IS NOT ABANDONED.
 */

static int opnprt(void)
{
	if (outfil[0] == '\0') {
		prtfp = stdout;
		return (1);
	}
	prtfp = fopen(outfil, "w");
	if (prtfp == NULL) {
		sprintf(work, "EHM-940 CANNOT OPEN PRINT FILE %.100s", outfil);
		errmsg(work);
		prtfp = stdout;
		return (0);
	}
	return (1);
}

/*
 *  LOADAL  -  LOAD ALL THREE DATA SETS.
 */

static int loadal(void)
{
	if (ldfleet(fltfil[0] ? fltfil : (char *)NULL) < 0)
		return (0);
	if (ldflts(repfil[0] ? repfil : (char *)NULL) < 0)
		return (0);
	if (ldslot(sltfil[0] ? sltfil : (char *)NULL) < 0)
		return (0);
	return (1);
}

/*
 *  BATCH  -  THE OVERNIGHT SEQUENCE.
 */

static int batch(void)
{
	if (!loadal())
		return (errcnt());
	if (ehmcalc() < 0)
		return (errcnt());
	if (wrkscd() < 0)
		return (errcnt());

	rptflt(prtfp);
	rptalr(prtfp);
	rptpln(prtfp);
	fflush(prtfp);
	return (errcnt());
}

/*
 *  MENU  -  PRINT THE OPERATOR MENU.
 */

static int menu(void)
{
	printf("\n");
	printf("        ENGINE HEALTH MONITORING SYSTEM  RELEASE %s\n",
		VERSION);
	printf("        RUN DATE %s\n", runday);
	printf("\n");
	printf("        1   LOAD FLEET MASTER AND FLIGHT REPORTS\n");
	printf("        2   RUN TREND SWEEP\n");
	printf("        3   BUILD WORKSHOP INDUCTION PLAN\n");
	printf("        4   PRINT FLEET STATUS SUMMARY\n");
	printf("        5   PRINT ALERT REPORT\n");
	printf("        6   PRINT INDUCTION PLAN\n");
	printf("        7   PRINT ENGINE HISTORY\n");
	printf("        8   RUN THE COMPLETE OVERNIGHT SEQUENCE\n");
	printf("        9   CHANGE RUN DATE\n");
	printf("        X   EXIT\n");
	printf("\n");
	printf("SELECT OPTION - ");
	fflush(stdout);
	return (0);
}

/*
 *  GETCMD  -  READ ONE OPERATOR REPLY.  RETURNS 0 AT END OF INPUT.
 */

static int getcmd(void)
{
	register char *p;

	if (fgets(cmd, sizeof(cmd), stdin) == NULL)
		return (0);
	for (p = cmd; *p != '\0'; p++)
		if (*p == '\n' || *p == '\r') {
			*p = '\0';
			break;
		}
	upcase(strim(cmd));
	return (1);
}

/*
 *  TERM  -  THE TERMINAL DIALOGUE.
 */

static int term(void)
{
	char esn[32];

	for (;;) {
		menu();
		if (!getcmd())
			break;
		if (cmd[0] == '\0')
			continue;

		switch (cmd[0]) {
		case '1':
			loadal();
			break;
		case '2':
			ehmcalc();
			break;
		case '3':
			wrkscd();
			break;
		case '4':
			rptflt(prtfp);
			break;
		case '5':
			rptalr(prtfp);
			break;
		case '6':
			rptpln(prtfp);
			break;
		case '7':
			printf("ENGINE SERIAL NUMBER - ");
			fflush(stdout);
			if (!getcmd())
				goto done;
			strncpy(esn, cmd, sizeof(esn) - 1);
			esn[sizeof(esn) - 1] = '\0';
			rpteng(prtfp, upcase(strim(esn)));
			break;
		case '8':
			batch();
			break;
		case '9':
			printf("RUN DATE YYMMDD - ");
			fflush(stdout);
			if (!getcmd())
				goto done;
			if (isdate(cmd)) {
				strcpy(runday, cmd);
				calcdn = 0;
				sprintf(work, "EHM-108 RUN DATE SET TO %s",
					runday);
				logmsg(work);
			} else
				errmsg("EHM-941 RUN DATE MUST BE SIX DIGITS");
			break;
		case 'X':
		case 'Q':
			goto done;
		default:
			errmsg("EHM-942 OPTION NOT RECOGNISED");
			break;
		}
		fflush(prtfp);
	}
done:
	return (errcnt());
}

/*
 *  SETFIL  -  TAKE A FILE NAME OVERRIDE FROM THE COMMAND LINE.  A NAME
 *             THAT WILL NOT FIT IS REJECTED RATHER THAN TRUNCATED, THE
 *             OPERATOR WOULD OTHERWISE BE GIVEN THE WRONG DATA SET.
 */

static int setfil(char *dst, int siz, char *val)
{
	if ((int)strlen(val) >= siz) {
		fprintf(stderr, "EHM-947 FILE NAME TOO LONG - %.80s\n", val);
		exit(2);
	}
	strcpy(dst, val);
	return (0);
}

/*
 *  MAIN  -  ARGUMENT DECODE AND DISPATCH.  THE ARGUMENT SCAN IS DONE BY
 *           HAND, GETOPT IS NOT PRESENT ON ALL OF THE TARGET SYSTEMS.
 */

int main(int argc, char *argv[])
{
	register int i;
	int isbatc;
	int rc;

	isbatc = 0;
	fltfil[0] = repfil[0] = sltfil[0] = outfil[0] = '\0';
	setday();

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			fprintf(stderr, "EHM-943 UNEXPECTED ARGUMENT %s\n",
				argv[i]);
			exit(2);
		}
		switch (argv[i][1]) {
		case 'b':
			isbatc = 1;
			break;
		case 'd':
			if (++i >= argc)
				goto noarg;
			if (!isdate(argv[i])) {
				fprintf(stderr,
				 "EHM-944 RUN DATE MUST BE YYMMDD\n");
				exit(2);
			}
			strcpy(runday, argv[i]);
			break;
		case 'f':
			if (++i >= argc)
				goto noarg;
			setfil(fltfil, (int)sizeof(fltfil), argv[i]);
			break;
		case 'r':
			if (++i >= argc)
				goto noarg;
			setfil(repfil, (int)sizeof(repfil), argv[i]);
			break;
		case 's':
			if (++i >= argc)
				goto noarg;
			setfil(sltfil, (int)sizeof(sltfil), argv[i]);
			break;
		case 'o':
			if (++i >= argc)
				goto noarg;
			setfil(outfil, (int)sizeof(outfil), argv[i]);
			break;
		case 'h':
			fprintf(stderr,
			 "USAGE - ehm [-b] [-d YYMMDD] [-f FLEET] [-r REPORTS] [-s SLOTS] [-o PRINT]\n");
			exit(0);
		default:
			fprintf(stderr, "EHM-945 OPTION %s NOT KNOWN\n",
				argv[i]);
			exit(2);
		}
	}

	loginit(DSLOG, isbatc);
	opnprt();

	sprintf(work, "EHM-100 RUN DATE %s, MODE %s", runday,
		isbatc ? "BATCH" : "TERMINAL");
	logmsg(work);

	if (isbatc)
		rc = batch();
	else
		rc = term();

	if (prtfp != NULL && prtfp != stdout)
		fclose(prtfp);
	logend();

	if (rc > 63)
		rc = 63;
	exit(rc);

noarg:
	fprintf(stderr, "EHM-946 OPTION %s REQUIRES A VALUE\n", argv[argc-1]);
	exit(2);
}
