/*
 *----------------------------------------------------------------------
 *  WRKSCD.C  WORKSHOP INDUCTION SCHEDULING
 *
 *  ENGINES CARRYING AN ALERT, OR WITH A SHOP VISIT DUE DATE INSIDE THE
 *  PLANNING WINDOW, ARE PLACED INTO THE WORKSHOP SLOTS READ FROM THE
 *  SLOT FILE.  THE RULE IS THE ONE AGREED WITH FLEET PLANNING IN THE
 *  NOTE OF 12-OCT-1990
 *
 *      1.  AIRCRAFT ON GROUND ENGINES FIRST, EARLIEST SLOT WINS.
 *      2.  THEN ACT LEVEL ALERTS IN ASCENDING CYCLES REMAINING.
 *      3.  THEN WATCH LEVEL ALERTS IN ASCENDING SHOP VISIT DUE DATE.
 *      4.  AN ENGINE IS NEVER PLANNED INTO A WEEK EARLIER THAN THE
 *          CURRENT RUN DATE.
 *      5.  WHERE NO SLOT CAN BE FOUND THE ENGINE IS STILL LISTED, WITH
 *          THE SLOT FIELDS BLANK, SO THAT PLANNING CAN SEE THE SHORT
 *          FALL.  THIS WAS ADDED AFTER THE DERBY BAY SHORTAGE OF 1994.
 *
 *  THE SORT IS A STRAIGHT SELECTION SORT ON A TABLE OF INDICES.  THE
 *  CANDIDATE LIST IS NEVER LONG ENOUGH TO JUSTIFY ANYTHING BETTER.
 *
 *  D.O'N.  12-OCT-1990
 *  MOD 2   30-NOV-1994  D.O'N.  UNPLACED ENGINES CARRIED ON THE PLAN.
 *----------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include "ehm.h"

extern int daydif();
extern int isdate();

static char work[132];

/*
 *  WSCOPE  -  DECIDE THE WORKSCOPE FROM THE ALERT REASONS AND THE
 *             HOURS RUN.  THE TURN ROUND TIME IN DAYS IS RETURNED
 *             THROUGH TATP.
 */

static int wscope(e, tatp)
struct engrec *e;
int *tatp;
{
	if (e->areas & ARLLP) {
		*tatp = 90;
		return (WSFULL);
	}
	if ((e->areas & AREGTM) && e->egtm <= EGTMLO) {
		*tatp = 65;
		return (WSPERF);
	}
	if (e->areas & ARVIB) {
		*tatp = 45;
		return (WSHOTS);
	}
	if (e->areas & ARERAT) {
		*tatp = 45;
		return (WSHOTS);
	}
	*tatp = 21;
	return (WSINSP);
}

/*
 *  WSTEXT  -  WORKSCOPE MNEMONIC FOR THE PRINTED PLAN.
 */

char *wstext(ws)
int ws;
{
	switch (ws) {
	case WSINSP:
		return ("INSP");
	case WSHOTS:
		return ("HSR ");
	case WSPERF:
		return ("PERF");
	case WSFULL:
		return ("OVHL");
	}
	return ("    ");
}

/*
 *  PRIOF  -  PRIORITY OF AN ENGINE, ONE IS THE MOST URGENT.  ZERO IS
 *            RETURNED FOR AN ENGINE THAT DOES NOT NEED A SLOT AT ALL.
 */

static int priof(e)
struct engrec *e;
{
	if (e->alert == ALAOG)
		return (1);
	if (e->alert == ALACT)
		return (2);
	if (e->alert == ALWTCH)
		return (3);
	return (0);
}

/*
 *  RANKOF  -  SECONDARY SORT KEY WITHIN A PRIORITY BAND.  SMALL IS
 *             URGENT.  CYCLES REMAINING IS USED WHERE IT IS KNOWN,
 *             OTHERWISE THE DAYS TO THE SHOP VISIT DUE DATE.
 */

static long rankof(e)
struct engrec *e;
{
	long r;

	if (e->cycrem < 999999L)
		r = e->cycrem;
	else
		r = 500000L;

	if (isdate(e->svdue)) {
		long d;
		d = (long)daydif(runday, e->svdue) * 100L;
		if (d < r)
			r = d;
	}
	return (r);
}

/*
 *  FNDSLT  -  FIND THE EARLIEST SLOT WITH A FREE BAY THAT IS NOT
 *             BEFORE THE RUN DATE.  RETURNS THE SLOT INDEX OR -1.
 */

static int fndslt()
{
	register int i;
	int best;

	best = -1;
	for (i = 0; i < nslts; i++) {
		if (slttab[i].used >= slttab[i].cap)
			continue;
		if (daydif(runday, slttab[i].wkcom) < 0)
			continue;
		if (best < 0 ||
		    daydif(slttab[i].wkcom, slttab[best].wkcom) > 0)
			best = i;
	}
	return (best);
}

/*
 *  WRKSCD  -  BUILD THE INDUCTION PLAN.  RETURNS THE NUMBER OF PLAN
 *             LINES, OR -1 IF THE TREND SWEEP HAS NOT BEEN RUN.
 */

int wrkscd()
{
	int cand[MAXPLN];       /* CANDIDATE ENGINE INDICES              */
	int ncand;
	register int i;
	register int j;
	int k;
	int t;
	int is;
	int nplaced;
	struct engrec *e;
	struct plnrec *p;

	if (!calcdn) {
		errmsg("EHM-922 TREND SWEEP MUST BE RUN BEFORE SCHEDULING");
		return (-1);
	}
	if (nslts <= 0)
		logmsg("EHM-105 NO WORKSHOP SLOTS LOADED, PLAN WILL BE EMPTY");

	/* ---- COLLECT THE CANDIDATES -------------------------------- */

	ncand = 0;
	for (i = 0; i < nengs; i++) {
		if (priof(&engtab[i]) == 0)
			continue;
		if (ncand >= MAXPLN) {
			sprintf(work,
			 "EHM-923 PLAN TRUNCATED AT %d LINES", MAXPLN);
			errmsg(work);
			break;
		}
		cand[ncand++] = i;
	}

	/* ---- SELECTION SORT ON PRIORITY THEN RANK ------------------ */

	for (i = 0; i < ncand - 1; i++) {
		k = i;
		for (j = i + 1; j < ncand; j++) {
			int pj;
			int pk;
			pj = priof(&engtab[cand[j]]);
			pk = priof(&engtab[cand[k]]);
			if (pj < pk) {
				k = j;
				continue;
			}
			if (pj == pk &&
			    rankof(&engtab[cand[j]]) <
			    rankof(&engtab[cand[k]]))
				k = j;
		}
		if (k != i) {
			t = cand[i];
			cand[i] = cand[k];
			cand[k] = t;
		}
	}

	/* ---- ALLOCATE SLOTS ---------------------------------------- */

	for (i = 0; i < nslts; i++)
		slttab[i].used = 0;

	nplns = 0;
	nplaced = 0;
	for (i = 0; i < ncand; i++) {
		e = &engtab[cand[i]];
		p = &plntab[nplns];

		strcpy(p->esn, e->esn);
		p->prio = priof(e);
		p->wscope = wscope(e, &p->tat);

		is = fndslt();
		if (is < 0) {
			p->shop[0]  = '\0';
			p->wkcom[0] = '\0';
			p->placed = 0;
			sprintf(work,
			 "EHM-106 ENGINE %s NO SLOT AVAILABLE, PRIORITY %d",
			 e->esn, p->prio);
			logmsg(work);
		} else {
			strcpy(p->shop, slttab[is].shop);
			strcpy(p->wkcom, slttab[is].wkcom);
			slttab[is].used++;
			p->placed = 1;
			nplaced++;
		}
		nplns++;
	}

	sprintf(work,
	 "EHM-107 INDUCTION PLAN BUILT, %d CANDIDATES, %d PLACED",
	 nplns, nplaced);
	logmsg(work);
	return (nplns);
}
