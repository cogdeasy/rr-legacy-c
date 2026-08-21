/*
 *----------------------------------------------------------------------
 *  DBIO.C    LOADING OF THE EHM DATA SETS INTO THE CORE TABLES
 *
 *  THE SYSTEM HOLDS THE WHOLE FLEET IN CORE FOR THE DURATION OF A RUN.
 *  THIS WAS ACCEPTABLE WHEN THE SYSTEM WAS COMMISSIONED WITH SIXTY
 *  ENGINES AND IS STILL ACCEPTABLE TODAY, BUT SEE THE TABLE LIMITS IN
 *  EHM.H BEFORE LOADING A LARGER FLEET.
 *
 *  THE THREE TABLES BELOW ARE THE ONLY DEFINITIONS IN THE SYSTEM, EVERY
 *  OTHER MODULE REFERS TO THEM THROUGH THE EXTERN DECLARATIONS IN EHM.H.
 *
 *  R.T.H.  02-APR-1988
 *  MOD 1   17-JUN-1991  A.M.S.  FLIGHT REPORTS CHAINED TO THE ENGINE.
 *  MOD 2   03-DEC-1993  A.M.S.  DATA SET DIRECTORY TAKEN FROM EHMDATA.
 *----------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ehm.h"
#include "dbio.h"

struct engrec engtab[MAXENG];
struct fltrec flttab[MAXFLT];
struct sltrec slttab[MAXSLT];
struct plnrec plntab[MAXPLN];

int nengs = 0;
int nflts = 0;
int nslts = 0;
int nplns = 0;
int calcdn = 0;
char runday[DATSIZ+1] = "960209";

extern int fldint(char *card, int off, int n);
extern long fldlng(char *card, int off, int n);
extern int isdate(char *d);

static char card[CARD];         /* CARD IMAGE WORK AREA                */
static char work[132];          /* MESSAGE WORK AREA                   */
static char path[256];          /* DATA SET NAME WORK AREA             */

/*
 *  DSNAME  -  BUILD A DATA SET NAME.  IF THE ENVIRONMENT VARIABLE
 *             EHMDATA IS SET IT IS TAKEN AS THE DIRECTORY HOLDING THE
 *             DATA SETS, OTHERWISE THE CURRENT DIRECTORY IS USED.
 */

static char *dsname(char *fname)
{
	char *dir;

	dir = getenv("EHMDATA");
	if (dir == NULL || *dir == '\0')
		strcpy(path, fname);
	else {
		strcpy(path, dir);
#ifdef VAXVMS
		strcat(path, fname);
#else
		strcat(path, "/");
		strcat(path, fname);
#endif
	}
	return (path);
}

/*
 *  GETCRD  -  READ ONE CARD IMAGE, SKIPPING COMMENT AND BLANK CARDS.
 *             RETURNS 0 AT END OF FILE, 1 OTHERWISE.
 */

static int getcrd(FILE *fp)
{
	register char *p;

again:
	if (fgets(card, CARD, fp) == NULL)
		return (0);

	/*  A LONG CARD IS TRUNCATED AT COLUMN 80 AND THE REST DISCARDED. */

	if (strchr(card, '\n') == NULL) {
		int c;
		while ((c = getc(fp)) != EOF && c != '\n')
			;
	}
	for (p = card; *p != '\0'; p++)
		if (*p == '\n' || *p == '\r') {
			*p = '\0';
			break;
		}
	if (card[0] == CMTCHR || card[0] == '\0')
		goto again;
	return (1);
}

/*
 *  FNDENG  -  LOOK UP AN ENGINE SERIAL NUMBER IN THE FLEET TABLE.
 *             RETURNS THE TABLE INDEX OR -1 IF NOT KNOWN.  A LINEAR
 *             SEARCH IS USED, THE TABLE IS SMALL AND UNSORTED.
 */

int fndeng(char *esn)
{
	register int i;

	for (i = 0; i < nengs; i++)
		if (strcmp(engtab[i].esn, esn) == 0)
			return (i);
	return (-1);
}

/*
 *  LDFLEET  -  LOAD THE FLEET MASTER.  RETURNS THE NUMBER OF ENGINES
 *              LOADED OR -1 IF THE DATA SET COULD NOT BE OPENED.
 */

int ldfleet(char *fname)
{
	FILE *fp;
	struct engrec *e;
	int nbad;

	nengs = 0;
	nbad = 0;
	fp = fopen(dsname(fname == NULL ? DSFLET : fname), "r");
	if (fp == NULL) {
		sprintf(work, "EHM-910 CANNOT OPEN FLEET MASTER %s", path);
		errmsg(work);
		return (-1);
	}

	while (getcrd(fp)) {
		if (nengs >= MAXENG) {
			sprintf(work,
			 "EHM-911 FLEET MASTER TRUNCATED AT %d ENGINES", MAXENG);
			errmsg(work);
			break;
		}
		e = &engtab[nengs];
		fldcpy(e->esn,   card, FL_ESN, ESNSIZ);
		fldcpy(e->etype, card, FL_TYP, TYPSIZ);
		fldcpy(e->acreg, card, FL_REG, REGSIZ);
		fldcpy(e->oper,  card, FL_OPR, OPRSIZ);
		e->tsn  = fldlng(card, FL_TSN, 7);
		e->csn  = fldlng(card, FL_CSN, 7);
		e->bstd = fldint(card, FL_BST, 4);
		e->status = card[FL_STA];
		fldcpy(e->svdue, card, FL_DUE, DATSIZ);

		upcase(e->esn);
		upcase(e->etype);
		upcase(e->acreg);
		upcase(e->oper);

		if (e->esn[0] == '\0') {
			nbad++;
			continue;
		}
		if (e->status != STSERV && e->status != STWTCH &&
		    e->status != STREST && e->status != STUNSV) {
			sprintf(work,
			 "EHM-912 ENGINE %s BAD STATUS CODE, TAKEN AS %c",
			 e->esn, STSERV);
			errmsg(work);
			e->status = STSERV;
		}
		if (!isdate(e->svdue)) {
			sprintf(work,
			 "EHM-913 ENGINE %s SHOP VISIT DUE DATE MISSING",
			 e->esn);
			errmsg(work);
		}

		e->nrep   = 0;
		e->frep   = -1;
		e->lrep   = -1;
		e->egtm   = 0;
		e->rate   = 0;
		e->vibmax = 0;
		e->cycrem = 0L;
		e->alert  = ALNONE;
		e->areas  = 0;
		nengs++;
	}
	fclose(fp);

	sprintf(work, "EHM-101 FLEET MASTER LOADED, %d ENGINES, %d REJECTED",
		nengs, nbad);
	logmsg(work);
	calcdn = 0;
	return (nengs);
}

/*
 *  LDFLTS  -  LOAD THE FLIGHT REPORTS AND CHAIN THEM ON TO THE OWNING
 *             ENGINE.  THE FILE IS ASSUMED TO BE IN DATE ORDER WITHIN
 *             ENGINE, WHICH IS HOW THE OVERNIGHT EXTRACT WRITES IT.
 *             REPORTS FOR AN ENGINE NOT IN THE FLEET MASTER ARE
 *             REPORTED AND DROPPED.
 */

int ldflts(char *fname)
{
	FILE *fp;
	struct fltrec *f;
	struct engrec *e;
	int ix;
	int norph;

	nflts = 0;
	norph = 0;
	if (nengs <= 0) {
		errmsg("EHM-914 FLEET MASTER MUST BE LOADED FIRST");
		return (-1);
	}
	fp = fopen(dsname(fname == NULL ? DSFLTS : fname), "r");
	if (fp == NULL) {
		sprintf(work, "EHM-915 CANNOT OPEN FLIGHT REPORT FILE %s",
			path);
		errmsg(work);
		return (-1);
	}

	while (getcrd(fp)) {
		if (nflts >= MAXFLT) {
			sprintf(work,
			 "EHM-916 FLIGHT REPORTS TRUNCATED AT %d RECORDS",
			 MAXFLT);
			errmsg(work);
			break;
		}
		f = &flttab[nflts];
		fldcpy(f->esn,   card, FR_ESN, ESNSIZ);
		fldcpy(f->fdate, card, FR_DAT, DATSIZ);
		fldcpy(f->fltno, card, FR_FLT, 6);
		f->egt  = fldint(card, FR_EGT, 4);
		f->egtm = fldint(card, FR_EGM, 4);
		f->n1   = fldint(card, FR_N1,  4);
		f->n2   = fldint(card, FR_N2,  4);
		f->ff   = fldint(card, FR_FF,  5);
		f->oilp = fldint(card, FR_OIP, 3);
		f->oilt = fldint(card, FR_OIT, 3);
		f->vib  = fldint(card, FR_VIB, 3);
		f->cyc  = fldlng(card, FR_CYC, 7);
		f->next = -1;
		upcase(f->esn);
		upcase(f->fltno);

		ix = fndeng(f->esn);
		if (ix < 0) {
			norph++;
			continue;
		}
		if (!isdate(f->fdate)) {
			sprintf(work,
			 "EHM-917 ENGINE %s FLIGHT %s BAD DATE, RECORD DROPPED",
			 f->esn, f->fltno);
			errmsg(work);
			continue;
		}

		e = &engtab[ix];
		if (e->frep < 0)
			e->frep = nflts;
		else
			flttab[e->lrep].next = nflts;
		e->lrep = nflts;
		e->nrep++;
		nflts++;
	}
	fclose(fp);

	sprintf(work,
	 "EHM-102 FLIGHT REPORTS LOADED, %d RECORDS, %d UNKNOWN ENGINES",
	 nflts, norph);
	logmsg(work);
	calcdn = 0;
	return (nflts);
}

/*
 *  LDSLOT  -  LOAD THE WORKSHOP INDUCTION SLOTS.
 */

int ldslot(char *fname)
{
	FILE *fp;
	struct sltrec *s;

	nslts = 0;
	fp = fopen(dsname(fname == NULL ? DSSLTS : fname), "r");
	if (fp == NULL) {
		sprintf(work, "EHM-918 CANNOT OPEN SLOT FILE %s", path);
		errmsg(work);
		return (-1);
	}

	while (getcrd(fp)) {
		if (nslts >= MAXSLT) {
			sprintf(work, "EHM-919 SLOT FILE TRUNCATED AT %d SLOTS",
				MAXSLT);
			errmsg(work);
			break;
		}
		s = &slttab[nslts];
		fldcpy(s->shop,  card, SL_SHP, SHPSIZ);
		fldcpy(s->wkcom, card, SL_WKC, DATSIZ);
		s->cap  = fldint(card, SL_CAP, 2);
		s->used = 0;
		upcase(s->shop);
		if (s->shop[0] == '\0' || !isdate(s->wkcom)) {
			sprintf(work, "EHM-920 SLOT CARD REJECTED - %s", card);
			errmsg(work);
			continue;
		}
		nslts++;
	}
	fclose(fp);

	sprintf(work, "EHM-103 SLOT FILE LOADED, %d SLOTS", nslts);
	logmsg(work);
	return (nslts);
}
