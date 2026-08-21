/*
 *----------------------------------------------------------------------
 *  RPTGEN.C  PRINTED OUTPUT
 *
 *  ALL REPORTS ARE 132 COLUMNS AND 60 LINES TO THE PAGE FOR THE LINE
 *  PRINTER IN THE OPERATIONS ROOM.  A FORM FEED IS WRITTEN AT THE HEAD
 *  OF EVERY PAGE EXCEPT THE FIRST.  DO NOT EXCEED COLUMN 132, THE
 *  PRINTER WRAPS AND THE PLANNERS COMPLAIN.
 *
 *  THE REPORTS ARE
 *
 *      RPTFLT  FLEET STATUS SUMMARY
 *      RPTALR  ALERT REPORT, EXCEPTIONS ONLY
 *      RPTPLN  WORKSHOP INDUCTION PLAN
 *      RPTENG  SINGLE ENGINE HISTORY WITH THE FLIGHT REPORT DETAIL
 *
 *  A.M.S.  06-JUN-1988
 *  MOD 4   18-JUL-1993  A.M.S.  ALERT REASON COLUMN ADDED.
 *----------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include "ehm.h"

extern char *datfmt(char *out, char *d);

#define PAGLIN  60              /* LINES PER PAGE INCLUDING HEADINGS   */

static int pageno = 0;          /* PAGE NUMBER WITHIN THE CURRENT RUN  */
static int linect = 0;          /* LINES USED ON THE CURRENT PAGE      */
static char title[80];          /* CURRENT REPORT TITLE FOR THE HEAD   */
static char dbuf[16];
static char abuf[16];

/*
 *  NEWPAG  -  THROW A PAGE AND PRINT THE STANDARD HEADING BLOCK.
 */

static int newpag(FILE *fp)
{
	char dt[16];

	pageno++;
	if (pageno > 1)
		fprintf(fp, "\f");
	datfmt(dt, runday);
	fprintf(fp,
"ENGINE HEALTH MONITORING SYSTEM %-6s%-52s RUN %-9s PAGE %4d\n",
		VERSION, title, dt, pageno);
	fprintf(fp,
"-----------------------------------------------------------------------------------------------------------------\n");
	linect = 2;
	return (0);
}

/*
 *  CHKPAG  -  ENSURE N LINES ARE AVAILABLE, THROWING A PAGE IF NOT.
 */

static int chkpag(FILE *fp, int n)
{
	if (linect == 0 || linect + n > PAGLIN)
		newpag(fp);
	return (0);
}

/*
 *  RPTOPN  -  START A NEW REPORT.  THE PAGE COUNTER IS NOT RESET, THE
 *             WHOLE RUN IS ONE LISTING AS FAR AS THE PRINTER ROOM IS
 *             CONCERNED.
 */

static int rptopn(char *t)
{
	strncpy(title, t, sizeof(title) - 1);
	title[sizeof(title) - 1] = '\0';
	linect = 0;
	return (0);
}

/*
 *  RPTFLT  -  FLEET STATUS SUMMARY, ONE LINE PER ENGINE.
 */

int rptflt(FILE *fp)
{
	register int i;
	struct engrec *e;
	int nact;
	int nwtch;

	if (nengs <= 0) {
		errmsg("EHM-930 NO FLEET LOADED, REPORT ABANDONED");
		return (-1);
	}

	rptopn("FLEET STATUS SUMMARY");
	chkpag(fp, 4);
	fprintf(fp,
"ESN      TYPE   REG     OPR  HRS SN  CYC SN BSTD ST  REPS   EGTM   RATE   VIB  CYC REM  SV DUE     ALERT REASON\n");
	fprintf(fp, "\n");
	linect += 2;

	nact = 0;
	nwtch = 0;
	for (i = 0; i < nengs; i++) {
		e = &engtab[i];
		chkpag(fp, 1);
		fprintf(fp,
"%-8s %-6s %-7s %-4s %7ld %7ld %4d  %c %5d %6d %6d %5d %8ld  %-9s  %-4s %-6s\n",
			e->esn, e->etype, e->acreg, e->oper,
			e->tsn, e->csn, e->bstd, e->status, e->nrep,
			e->egtm, e->rate, e->vibmax, e->cycrem,
			datfmt(dbuf, e->svdue),
			alrtxt(e->alert), aretxt(abuf, e->areas));
		linect++;
		if (e->alert >= ALACT)
			nact++;
		else if (e->alert == ALWTCH)
			nwtch++;
	}

	chkpag(fp, 3);
	fprintf(fp, "\n");
	fprintf(fp,
"TOTAL ENGINES %4d      ACT OR ABOVE %4d      ON WATCH %4d      FLIGHT REPORTS %5d\n",
		nengs, nact, nwtch, nflts);
	linect += 2;
	return (nengs);
}

/*
 *  RPTALR  -  ALERT REPORT.  EXCEPTIONS ONLY, IN DESCENDING SEVERITY.
 *             THE REASON LEGEND IS PRINTED AT THE FOOT OF THE REPORT
 *             AT THE REQUEST OF THE DUTY CONTROLLERS.
 */

int rptalr(FILE *fp)
{
	register int i;
	int lev;
	int nout;
	struct engrec *e;

	if (!calcdn) {
		errmsg("EHM-931 TREND SWEEP MUST BE RUN BEFORE THIS REPORT");
		return (-1);
	}

	rptopn("ENGINE ALERT REPORT");
	chkpag(fp, 4);
	fprintf(fp,
"LVL  ESN      TYPE   REG     OPR   EGTM   RATE    VIB   CYC REM  SV DUE     REASON  RECOMMENDED ACTION\n");
	fprintf(fp, "\n");
	linect += 2;

	nout = 0;
	for (lev = ALAOG; lev >= ALWTCH; lev--) {
		for (i = 0; i < nengs; i++) {
			e = &engtab[i];
			if (e->alert != lev)
				continue;
			chkpag(fp, 1);
			fprintf(fp,
"%-4s %-8s %-6s %-7s %-4s %6d %6d %6d %9ld  %-9s  %-6s  %s\n",
				alrtxt(e->alert), e->esn, e->etype,
				e->acreg, e->oper, e->egtm, e->rate,
				e->vibmax, e->cycrem,
				datfmt(dbuf, e->svdue),
				aretxt(abuf, e->areas),
				(e->alert >= ALACT) ?
				  "REMOVE AT NEXT OPPORTUNITY" :
				  "MONITOR, REVIEW NEXT SWEEP");
			linect++;
			nout++;
		}
	}

	if (nout == 0) {
		chkpag(fp, 1);
		fprintf(fp, "     NO ENGINES ARE CARRYING AN ALERT.\n");
		linect++;
	}

	chkpag(fp, 4);
	fprintf(fp, "\n");
	fprintf(fp,
"REASON CODES   M EGT MARGIN LOW   R DETERIORATION RATE   V VIBRATION   O OIL SYSTEM   L LIFE LIMIT   D SHOP VISIT DUE\n");
	linect += 2;
	return (nout);
}

/*
 *  RPTPLN  -  WORKSHOP INDUCTION PLAN, IN THE ORDER BUILT BY WRKSCD.
 */

int rptpln(FILE *fp)
{
	register int i;
	struct plnrec *p;
	int nun;

	if (nplns <= 0) {
		errmsg("EHM-932 NO INDUCTION PLAN HAS BEEN BUILT");
		return (-1);
	}

	rptopn("WORKSHOP INDUCTION PLAN");
	chkpag(fp, 4);
	fprintf(fp,
"SEQ  PRI  ESN      SHOP  WEEK COMM  WORKSCOPE  TAT  STATUS\n");
	fprintf(fp, "\n");
	linect += 2;

	nun = 0;
	for (i = 0; i < nplns; i++) {
		p = &plntab[i];
		chkpag(fp, 1);
		fprintf(fp,
"%3d  %3d  %-8s %-4s  %-9s  %-4s      %3d  %s\n",
			i + 1, p->prio, p->esn,
			p->placed ? p->shop : "    ",
			p->placed ? datfmt(dbuf, p->wkcom) : "         ",
			wstext(p->wscope), p->tat,
			p->placed ? "PLANNED" : "NO SLOT AVAILABLE");
		linect++;
		if (!p->placed)
			nun++;
	}

	chkpag(fp, 6);
	fprintf(fp, "\n");
	fprintf(fp, "SLOT UTILISATION\n");
	fprintf(fp, "\n");
	linect += 3;
	for (i = 0; i < nslts; i++) {
		chkpag(fp, 1);
		fprintf(fp, "     SHOP %-4s WEEK COMMENCING %-9s  BAYS %2d OF %2d USED\n",
			slttab[i].shop, datfmt(dbuf, slttab[i].wkcom),
			slttab[i].used, slttab[i].cap);
		linect++;
	}

	chkpag(fp, 2);
	fprintf(fp, "\n");
	fprintf(fp, "PLAN LINES %3d      UNPLACED %3d\n", nplns, nun);
	linect += 2;
	return (nplns);
}

/*
 *  RPTENG  -  SINGLE ENGINE HISTORY.  ESN IS THE ENGINE SERIAL NUMBER
 *             AS KEYED BY THE CONTROLLER.
 */

int rpteng(FILE *fp, char *esn)
{
	int ix;
	register int j;
	struct engrec *e;
	struct fltrec *f;
	char t[80];

	ix = fndeng(esn);
	if (ix < 0) {
		char buf[132];
		sprintf(buf, "EHM-933 ENGINE %s IS NOT IN THE FLEET MASTER",
			esn);
		errmsg(buf);
		return (-1);
	}
	e = &engtab[ix];

	sprintf(t, "ENGINE HISTORY - %s", e->esn);
	rptopn(t);
	chkpag(fp, 8);
	fprintf(fp, "ENGINE %-8s  MARK %-6s  AIRFRAME %-7s  OPERATOR %-4s  BUILD STANDARD %4d  STATUS %c\n",
		e->esn, e->etype, e->acreg, e->oper, e->bstd, e->status);
	fprintf(fp, "HOURS SINCE NEW %7ld   CYCLES SINCE NEW %7ld   SHOP VISIT DUE %-9s\n",
		e->tsn, e->csn, datfmt(dbuf, e->svdue));
	fprintf(fp, "EGT MARGIN %4d DEG C   DETERIORATION %4d TENTHS PER 1000 CYC   CYCLES REMAINING %8ld\n",
		e->egtm, e->rate, e->cycrem);
	fprintf(fp, "ALERT %-4s  REASONS %-6s  FLIGHT REPORTS HELD %4d\n",
		alrtxt(e->alert), aretxt(abuf, e->areas), e->nrep);
	fprintf(fp, "\n");
	fprintf(fp,
"DATE       FLIGHT    EGT  EGTM     N1     N2      FF  OILP  OILT   VIB    CYCLES\n");
	fprintf(fp, "\n");
	linect += 7;

	for (j = e->frep; j >= 0; j = f->next) {
		f = &flttab[j];
		chkpag(fp, 1);
		fprintf(fp,
"%-9s  %-6s  %5d %5d %6d %6d %7d %5d %5d %5d %9ld\n",
			datfmt(dbuf, f->fdate), f->fltno, f->egt, f->egtm,
			f->n1, f->n2, f->ff, f->oilp, f->oilt, f->vib,
			f->cyc);
		linect++;
	}

	if (e->nrep == 0) {
		chkpag(fp, 1);
		fprintf(fp, "     NO FLIGHT REPORTS HELD FOR THIS ENGINE.\n");
		linect++;
	}
	return (e->nrep);
}
