/*
 *----------------------------------------------------------------------
 *  SPRPRV.C  SPARES PROVISIONING AND ROTABLE POOL COVER
 *
 *  THE PASS RUNS AFTER THE INDUCTION PLAN HAS BEEN BUILT BY WRKSCD().
 *  EACH PLAN LINE IS TAKEN IN PLAN ORDER, WHICH IS THE ORDER THE BAYS
 *  WILL BE FILLED, AND THE PART NUMBERS CALLED FOR BY THE WORKSCOPE OF
 *  THAT ENGINE MARK ARE DRAWN FROM A WORKING STOCK POSITION HELD IN
 *  THE SPARES TABLE.  ONE UNIT OF EACH PART NUMBER IS TAKEN PER
 *  INDUCTION, WHICH IS THE PROVISIONING RULE AGREED WITH THE STORES
 *  OFFICE IN THE NOTE OF 14-JUN-1996
 *
 *      1.  A PART DRAWN FROM THE SHELF LEAVES THE LINE SUPPORTED.
 *      2.  WHERE THE SHELF IS EMPTY THE OUTSTANDING PURCHASE ORDERS
 *          ARE DRAWN AGAINST INSTEAD.  IF THE LEAD TIME LANDS THE PART
 *          ON OR BEFORE THE INDUCTION WEEK THE LINE IS SHORT BUT
 *          RECOVERABLE, OTHERWISE IT IS SHORT BEYOND THE WEEK.
 *      3.  WHERE NO BAY COULD BE FOUND THERE IS NO INDUCTION WEEK TO
 *          WORK TO AND A PART ON ORDER IS TAKEN AS RECOVERABLE.
 *      4.  A LINE WHOSE WORKSCOPE HAS NO PART NUMBERS HELD FOR THAT
 *          ENGINE MARK IS REPORTED AS NO PARTS LISTED.  THE STORES
 *          OFFICE IS EXPECTED TO CARD THE MISSING RANGE.
 *
 *  THE ROTABLE INDICATOR IS CARRIED FOR THE SHORTFALL SUMMARY ONLY.  A
 *  ROTABLE THAT IS SHORT IS CHASED THROUGH THE REPAIR LOOP, NOT BOUGHT,
 *  AND THE DUTY PLANNER READS THE COLUMN THAT WAY.
 *
 *  ALL ARITHMETIC IS INTEGER, AS EVERYWHERE ELSE IN THIS SYSTEM.
 *
 *  D.O'N.  14-JUN-1996
 *----------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include "ehm.h"

extern int daydif();
extern int isdate();

static char work[132];

/*
 *  WKDAYS  -  DAYS FROM THE RUN DATE TO THE INDUCTION WEEK OF THE PLAN
 *             LINE.  -1 IS RETURNED WHERE THE LINE COULD NOT BE PLACED
 *             AND THERE IS THEREFORE NO WEEK TO WORK TO.
 */

static int wkdays(p)
struct plnrec *p;
{
	if (!p->placed || !isdate(p->wkcom))
		return (-1);
	return (daydif(runday, p->wkcom));
}

/*
 *  DRAWPT  -  DRAW ONE UNIT OF THE PART S AGAINST AN INDUCTION THAT IS
 *             DAYS AHEAD OF THE RUN DATE.  THE STATUS OF THE DRAW IS
 *             RETURNED, PVFULL, PVLEAD OR PVLATE.
 */

static int drawpt(s, days)
struct sprrec *s;
int days;
{
	s->dmand++;
	if (s->avail > 0) {
		s->avail--;
		return (PVFULL);
	}
	if (s->onord > 0) {
		s->onord--;
		if (days < 0 || s->lead <= days)
			return (PVLEAD);
		return (PVLATE);
	}
	s->shrt++;
	return (PVLATE);
}

/*
 *  SPPROV  -  RUN THE PROVISIONING PASS OVER THE INDUCTION PLAN.
 *             RETURNS THE NUMBER OF PLAN LINES EXAMINED, OR -1 IF NO
 *             PLAN HAS BEEN BUILT.
 */

int spprov()
{
	register int i;
	register int j;
	int days;
	int sts;
	int ix;
	int nfull;
	int nshrt;
	int nlate;
	struct plnrec *p;
	struct sprrec *s;
	struct engrec *e;

	provdn = 0;
	if (nplns <= 0) {
		errmsg("EHM-928 NO INDUCTION PLAN, PROVISIONING ABANDONED");
		return (-1);
	}
	if (nsprs <= 0)
		logmsg("EHM-111 NO SPARES LOADED, PROVISIONING WILL BE EMPTY");

	/* ---- OPEN THE WORKING STOCK POSITION ----------------------- */

	for (j = 0; j < nsprs; j++) {
		s = &sprtab[j];
		s->avail = s->qoh;
		s->onord = s->qoo;
		s->dmand = 0;
		s->shrt  = 0;
	}

	/* ---- DRAW THE PARTS IN PLAN ORDER -------------------------- */

	nfull = 0;
	nshrt = 0;
	nlate = 0;
	for (i = 0; i < nplns; i++) {
		p = &plntab[i];
		p->pvsts  = PVNONE;
		p->pvreq  = 0;
		p->pvshr  = 0;
		p->pvlead = 0;

		ix = fndeng(p->esn);
		if (ix < 0)
			continue;
		e = &engtab[ix];
		days = wkdays(p);

		for (j = 0; j < nsprs; j++) {
			s = &sprtab[j];
			if (s->wscope != p->wscope)
				continue;
			if (strcmp(s->etype, e->etype) != 0)
				continue;

			p->pvreq++;
			sts = drawpt(s, days);
			if (sts != PVFULL) {
				p->pvshr++;
				if (s->lead > p->pvlead)
					p->pvlead = s->lead;
			}
			if (sts > p->pvsts)
				p->pvsts = sts;
		}

		if (p->pvreq == 0) {
			sprintf(work,
			 "EHM-112 ENGINE %s NO PART NUMBERS HELD FOR %s",
			 p->esn, wstext(p->wscope));
			logmsg(work);
			continue;
		}
		if (p->pvsts == PVFULL)
			nfull++;
		else if (p->pvsts == PVLEAD)
			nshrt++;
		else
			nlate++;
	}

	provdn = 1;
	sprintf(work,
"EHM-110 PROVISIONING COMPLETE, %d SUPPORTED, %d ON LEAD TIME, %d SHORT",
	 nfull, nshrt, nlate);
	logmsg(work);
	return (nplns);
}
