/*
 *----------------------------------------------------------------------
 *  ERRLOG.C  OPERATOR MESSAGES AND THE RUN LOG
 *
 *  EVERY MESSAGE IS WRITTEN TO THE RUN LOG AND, WHEN THE SYSTEM IS
 *  RUNNING AT A TERMINAL RATHER THAN UNDER THE OVERNIGHT BATCH JOB,
 *  ALSO TO THE OPERATOR.  MESSAGE NUMBERS ARE OF THE FORM EHM-NNN AND
 *  ARE LISTED IN DOC/MESSAGE.DOC.  DO NOT REUSE A RETIRED NUMBER.
 *
 *  R.T.H.  22-MAR-1988
 *----------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include "ehm.h"
#include "dbio.h"

static FILE *logfp = NULL;      /* RUN LOG, NULL UNTIL LOGINIT CALLED  */
static int batch = 0;           /* NON ZERO WHEN RUNNING UNDER BATCH   */
static long nmsg = 0L;          /* MESSAGES WRITTEN THIS RUN           */
static long nerr = 0L;          /* OF WHICH ERRORS                     */

/*
 *  LOGINIT  -  OPEN THE RUN LOG FOR APPEND.  A FAILURE HERE IS NOT
 *              FATAL, THE RUN CARRIES ON WITH TERMINAL OUTPUT ONLY.
 */

int loginit(fname, isbatc)
char *fname;
int isbatc;
{
	char *nm;

	batch = isbatc;
	nm = (fname == NULL || *fname == '\0') ? DSLOG : fname;
	logfp = fopen(nm, "a");
	if (logfp == NULL) {
		fprintf(stderr, "EHM-901 CANNOT OPEN RUN LOG %s\n", nm);
		return (0);
	}
	fprintf(logfp, "\n");
	fprintf(logfp, "EHM-001 RELEASE %s OF %s STARTED\n", VERSION, RELDATE);
	fflush(logfp);
	return (1);
}

/*
 *  LOGMSG  -  INFORMATION MESSAGE.  TXT IS ALREADY FORMATTED BY THE
 *             CALLER, THERE IS NO VARIABLE ARGUMENT SUPPORT ON ALL OF
 *             THE COMPILERS THIS SYSTEM IS BUILT WITH.
 */

int logmsg(txt)
char *txt;
{
	nmsg++;
	if (logfp != NULL) {
		fprintf(logfp, "%s\n", txt);
		fflush(logfp);
	}
	if (!batch)
		printf("%s\n", txt);
	return (0);
}

/*
 *  ERRMSG  -  ERROR MESSAGE.  ALWAYS GOES TO THE ERROR STREAM AS WELL
 *             AS THE LOG SO THAT THE BATCH JOB LISTING SHOWS IT.
 */

int errmsg(txt)
char *txt;
{
	nmsg++;
	nerr++;
	if (logfp != NULL) {
		fprintf(logfp, "%s\n", txt);
		fflush(logfp);
	}
	fprintf(stderr, "%s\n", txt);
	return (0);
}

/*
 *  LOGEND  -  CLOSE THE RUN LOG AND REPORT THE ERROR COUNT.
 */

int logend()
{
	char buf[132];

	sprintf(buf, "EHM-002 RUN COMPLETE, %ld MESSAGES, %ld ERRORS",
		nmsg, nerr);
	if (logfp != NULL) {
		fprintf(logfp, "%s\n", buf);
		fclose(logfp);
		logfp = NULL;
	}
	if (!batch)
		printf("%s\n", buf);
	return ((int)nerr);
}

/*
 *  ERRCNT  -  ERRORS SO FAR.  USED TO SET THE EXIT STATUS.
 */

int errcnt()
{
	return ((int)nerr);
}
