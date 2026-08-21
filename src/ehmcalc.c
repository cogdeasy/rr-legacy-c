/*
 *----------------------------------------------------------------------
 *  EHMCALC.C TREND CALCULATION AND ALERT GENERATION
 *
 *  FOR EACH ENGINE THE FLIGHT REPORT CHAIN IS WALKED AND THE FOLLOWING
 *  QUANTITIES ARE DERIVED
 *
 *      EGTM    THE LATEST EXHAUST GAS TEMPERATURE MARGIN
 *      RATE    THE DETERIORATION OF THAT MARGIN, DEG C PER 1000 CYCLES,
 *              HELD IN TENTHS, OBTAINED BY A LEAST SQUARES FIT OF
 *              MARGIN AGAINST CYCLES SINCE NEW
 *      CYCREM  CYCLES REMAINING BEFORE THE MARGIN REACHES THE RED LINE
 *      VIBMAX  THE WORST BROADBAND VIBRATION SEEN IN THE WINDOW
 *
 *  THE ARITHMETIC IS DONE IN LONG INTEGERS THROUGHOUT.  THE OPERATIONS
 *  MACHINE HAS NO FLOATING POINT UNIT AND THE SOFTWARE LIBRARY IS FAR
 *  TOO SLOW FOR A FLEET SWEEP.  SCALING IS BY POWERS OF TEN, THE
 *  COMMENTS AGAINST EACH VARIABLE GIVE THE SCALE IN USE.
 *
 *  A.M.S.  11-SEP-1989
 *  MOD 1   04-MAY-1992  A.M.S.  LEAST SQUARES REPLACES FIRST/LAST SLOPE.
 *  MOD 3   21-JAN-1995  D.O'N.  LIFE LIMITED PART CHECK ADDED.
 *----------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include "ehm.h"

static char work[132];

/*
 *  CLCONE  -  CALCULATE ONE ENGINE.  CALLED FROM EHMCALC, NOT INTENDED
 *             TO BE CALLED FROM OUTSIDE THIS MODULE.
 */

static int clcone(struct engrec *e)
{
	register int ix;
	struct fltrec *f;
	long n;                 /* NUMBER OF POINTS                     */
	long sx;                /* SUM OF CYCLES, THOUSANDS * 10        */
	long sy;                /* SUM OF MARGINS, DEG C                */
	long sxy;               /* SUM OF PRODUCTS                      */
	long sxx;               /* SUM OF SQUARES                       */
	long x;
	long y;
	long num;
	long den;
	long slope;             /* DEG C PER 1000 CYCLES, TENTHS        */
	int  last;              /* LAST MARGIN SEEN                     */
	int  oilbad;            /* OIL PRESSURE EXCEEDANCES             */
	int  days;

	e->egtm   = 0;
	e->rate   = 0;
	e->vibmax = 0;
	e->cycrem = 0L;
	e->alert  = ALNONE;
	e->areas  = 0;

	n = 0L;
	sx = sy = sxy = sxx = 0L;
	last = 0;
	oilbad = 0;

	for (ix = e->frep; ix >= 0; ix = f->next) {
		f = &flttab[ix];

		/*  CYCLES ARE SCALED TO HUNDREDS SO THAT THE SUM OF THE
		 *  SQUARES CANNOT OVERFLOW A THIRTY TWO BIT LONG EVEN FOR
		 *  AN ENGINE APPROACHING ITS LIFE LIMIT.
		 */

		x = f->cyc / 100L;
		y = (long)f->egtm;

		n++;
		sx  += x;
		sy  += y;
		sxy += x * y;
		sxx += x * x;

		last = f->egtm;

		if (f->vib > e->vibmax)
			e->vibmax = f->vib;
		if (f->oilp > 0 && f->oilp < OILPLO)
			oilbad++;
	}

	if (n == 0L) {
		/*  NO REPORTS.  THE ENGINE IS NOT TRENDED BUT THE SHOP
		 *  VISIT DATE AND THE PART LIVES ARE STILL CHECKED BELOW.
		 */
		e->egtm = -1;
	} else {
		e->egtm = last;
	}

	/*  LEAST SQUARES SLOPE.  AT LEAST THREE POINTS ARE DEMANDED
	 *  BEFORE A RATE IS PUBLISHED, TWO POINTS THROUGH NOISE GAVE
	 *  FAR TOO MANY SPURIOUS ALERTS ON THE OLD SCHEME.
	 */

	if (n >= 3L) {
		num = n * sxy - sx * sy;
		den = n * sxx - sx * sx;
		if (den != 0L) {
			/*  SLOPE IS DEG C PER HUNDRED CYCLES.  MULTIPLY BY
			 *  TEN FOR PER THOUSAND AND BY TEN AGAIN FOR THE
			 *  TENTH OF A DEGREE SCALE.  THE SIGN IS INVERTED
			 *  SO THAT A DETERIORATING ENGINE READS POSITIVE.
			 */
			slope = -(num * 100L) / den;
			e->rate = (int)slope;
		}
	}

	/*  CYCLES REMAINING TO THE RED LINE AT THE CURRENT RATE.        */

	if (e->rate > 0 && e->egtm > EGTMLO)
		e->cycrem = ((long)(e->egtm - EGTMLO) * 10000L) /
			    (long)e->rate;
	else if (e->rate > 0)
		e->cycrem = 0L;
	else
		e->cycrem = 999999L;

	/* ---- ALERT RULES.  ORDER MATTERS, THE HIGHEST WINS. --------- */

	if (n > 0L) {
		if (e->egtm <= EGTMLO) {
			e->areas |= AREGTM;
			e->alert = ALACT;
		} else if (e->egtm <= EGTMWN) {
			e->areas |= AREGTM;
			if (e->alert < ALWTCH)
				e->alert = ALWTCH;
		}
		if (e->rate >= RATLIM) {
			e->areas |= ARERAT;
			if (e->alert < ALWTCH)
				e->alert = ALWTCH;
		}
		if (e->vibmax >= VIBACT) {
			e->areas |= ARVIB;
			e->alert = ALACT;
		} else if (e->vibmax >= VIBWRN) {
			e->areas |= ARVIB;
			if (e->alert < ALWTCH)
				e->alert = ALWTCH;
		}
		if (oilbad >= 2) {
			e->areas |= AROILC;
			if (e->alert < ALWTCH)
				e->alert = ALWTCH;
		}
	}

	if (e->csn >= LLPLIM) {
		e->areas |= ARLLP;
		e->alert = ALACT;
	}

	if (isdate(e->svdue)) {
		days = daydif(runday, e->svdue);
		if (days <= DUEWIN) {
			e->areas |= ARDUE;
			if (e->alert < ALWTCH)
				e->alert = ALWTCH;
		}
		if (days < 0) {
			e->alert = ALACT;
		}
	}

	/*  AN ENGINE ALREADY DECLARED UNSERVICEABLE IS AN AIRCRAFT ON
	 *  GROUND CASE AND OUTRANKS EVERYTHING ELSE.
	 */

	if (e->status == STUNSV)
		e->alert = ALAOG;

	return (e->alert);
}

/*
 *  EHMCALC  -  SWEEP THE WHOLE FLEET.  RETURNS THE NUMBER OF ENGINES
 *              CARRYING AN ALERT OF WATCH LEVEL OR ABOVE.
 */

int ehmcalc(void)
{
	register int i;
	int nalert;

	if (nengs <= 0) {
		errmsg("EHM-921 NO FLEET LOADED, NOTHING TO CALCULATE");
		return (-1);
	}

	nalert = 0;
	for (i = 0; i < nengs; i++)
		if (clcone(&engtab[i]) >= ALWTCH)
			nalert++;

	calcdn = 1;
	sprintf(work, "EHM-104 TREND SWEEP COMPLETE, %d ENGINES, %d ALERTS",
		nengs, nalert);
	logmsg(work);
	return (nalert);
}

/*
 *  ARETXT  -  EXPAND THE ALERT REASON BITS INTO A PRINTABLE STRING OF
 *             FIXED WIDTH.  A DOT MEANS THE REASON IS NOT PRESENT.
 *             ORDER IS EGT MARGIN, RATE, VIBRATION, OIL, LIFE, DUE.
 */

char *aretxt(char *out, int areas)
{
	out[0] = (areas & AREGTM) ? 'M' : '.';
	out[1] = (areas & ARERAT) ? 'R' : '.';
	out[2] = (areas & ARVIB)  ? 'V' : '.';
	out[3] = (areas & AROILC) ? 'O' : '.';
	out[4] = (areas & ARLLP)  ? 'L' : '.';
	out[5] = (areas & ARDUE)  ? 'D' : '.';
	out[6] = '\0';
	return (out);
}

/*
 *  ALRTXT  -  ALERT LEVEL AS A FOUR CHARACTER MNEMONIC.
 */

char *alrtxt(int lev)
{
	switch (lev) {
	case ALNONE:
		return ("    ");
	case ALWTCH:
		return ("WTCH");
	case ALACT:
		return ("ACT ");
	case ALAOG:
		return ("AOG ");
	}
	return ("????");
}
