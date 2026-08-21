/*
 *----------------------------------------------------------------------
 *  EHM.H     ENGINE HEALTH MONITORING SYSTEM - COMMON DEFINITIONS
 *
 *  SYSTEM    : EHM/MRO  (ENGINE HEALTH MONITORING AND OVERHAUL CONTROL)
 *  RELEASE   : 4.2C
 *  AUTHOR    : R. T. HALLIWELL, POWERPLANT SYSTEMS GROUP
 *  CREATED   : 14-MAR-1988
 *  LAST MOD  : 09-FEB-1996  (SEE HISTORY)
 *
 *  THIS FILE MUST BE INCLUDED BY EVERY MODULE OF THE SYSTEM.
 *  DO NOT REORDER THE STRUCTURE MEMBERS - THE DISC RECORDS ARE
 *  WRITTEN AND READ BY FIELD POSITION (SEE DOC/FILEFMT.DOC).
 *----------------------------------------------------------------------
 */

#ifndef EHM_H
#define EHM_H

#define VERSION "4.2C"
#define RELDATE "09-FEB-96"

/* ------ TABLE LIMITS.  RAISE WITH CARE, CORE IS NOT FREE. --------- */

#define MAXENG  200             /* ENGINES IN THE FLEET MASTER        */
#define MAXFLT  4000            /* FLIGHT REPORTS HELD IN CORE        */
#define MAXSLT  60              /* WORKSHOP INDUCTION SLOTS           */
#define MAXPLN  120             /* LINES IN THE INDUCTION PLAN        */

#define ESNSIZ  8               /* ENGINE SERIAL NUMBER, 8 CHARS      */
#define TYPSIZ  6
#define REGSIZ  7
#define OPRSIZ  4
#define DATSIZ  6               /* DATES ARE YYMMDD, SIX DIGITS       */
#define SHPSIZ  4
#define RECSIZ  132             /* ONE CARD IMAGE / ONE PRINT LINE    */

/* ------ ENGINE SERVICEABILITY CODES ------------------------------- */

#define STSERV  'S'             /* SERVICEABLE                        */
#define STWTCH  'W'             /* ON WATCH                           */
#define STREST  'R'             /* RESTRICTED, DERATE APPLIED         */
#define STUNSV  'U'             /* UNSERVICEABLE, AOG                 */

/* ------ ALERT LEVELS RAISED BY THE TREND CALCULATION -------------- */

#define ALNONE  0
#define ALWTCH  1
#define ALACT   2
#define ALAOG   3

/* ------ ALERT REASON BITS (SET IN ENGREC.AREAS) ------------------- */

#define AREGTM  0001            /* EGT MARGIN LOW                     */
#define ARERAT  0002            /* MARGIN DETERIORATION RATE HIGH     */
#define ARVIB   0004            /* VIBRATION EXCEEDANCE               */
#define AROILC  0010            /* OIL CONSUMPTION / PRESSURE         */
#define ARLLP   0020            /* LIFE LIMITED PART CYCLES           */
#define ARDUE   0040            /* SHOP VISIT DATE DUE OR PAST        */

/* ------ TREND LIMITS.  TAKEN FROM MAINTENANCE MANUAL 72-00-00. ---- */

#define EGTMLO  15              /* DEG C, MARGIN AT OR BELOW = ACT    */
#define EGTMWN  30              /* DEG C, MARGIN AT OR BELOW = WATCH  */
#define RATLIM  25              /* DEG C PER 1000 CYCLES, TENTHS *10  */
#define VIBWRN  35              /* VIBRATION UNITS * 10, WATCH        */
#define VIBACT  50              /* VIBRATION UNITS * 10, ACT          */
#define OILPLO  25              /* OIL PRESSURE PSI, MINIMUM          */
#define LLPLIM  20000L          /* LIFE LIMIT, CYCLES SINCE NEW       */
#define DUEWIN  90              /* DAYS AHEAD TREATED AS DUE          */

/*
 *  ENGINE MASTER RECORD.  ONE PER ENGINE.  THE FIRST NINE FIELDS ARE
 *  READ FROM THE FLEET MASTER FILE, THE REMAINDER ARE COMPUTED BY
 *  EHMCALC() AND ARE NOT WRITTEN BACK TO DISC.
 */

struct engrec {
	char    esn[ESNSIZ+1];          /* ENGINE SERIAL NUMBER        */
	char    etype[TYPSIZ+1];        /* ENGINE MARK                 */
	char    acreg[REGSIZ+1];        /* AIRFRAME REGISTRATION       */
	char    oper[OPRSIZ+1];         /* OPERATOR CODE               */
	long    tsn;                    /* HOURS SINCE NEW             */
	long    csn;                    /* CYCLES SINCE NEW            */
	int     bstd;                   /* BUILD STANDARD              */
	char    status;                 /* SEE SERVICEABILITY CODES    */
	char    svdue[DATSIZ+1];        /* SHOP VISIT DUE, YYMMDD      */

	int     nrep;                   /* FLIGHT REPORTS FOUND        */
	int     frep;                   /* HEAD OF REPORT CHAIN, -1 NIL*/
	int     lrep;                   /* TAIL OF REPORT CHAIN, -1 NIL*/
	int     egtm;                   /* LATEST EGT MARGIN, DEG C    */
	int     rate;                   /* DEG C PER 1000 CYC, *10     */
	int     vibmax;                 /* WORST VIBRATION SEEN, *10   */
	long    cycrem;                 /* CYCLES TO MARGIN EXHAUSTION */
	int     alert;                  /* ALNONE / ALWTCH / ALACT ... */
	int     areas;                  /* ALERT REASON BITS           */
};

/*
 *  FLIGHT REPORT RECORD.  ONE PER CRUISE SNAPSHOT DOWNLINKED FROM THE
 *  ACARS FEED AND CONVERTED TO CARD IMAGE BY THE OVERNIGHT JOB.
 *  ALL VALUES ARE HELD AS INTEGERS - THERE IS NO FLOATING POINT
 *  HARDWARE ON THE OPERATIONS MACHINE.
 */

struct fltrec {
	char    esn[ESNSIZ+1];
	char    fdate[DATSIZ+1];        /* FLIGHT DATE, YYMMDD         */
	char    fltno[7];               /* FLIGHT NUMBER               */
	int     egt;                    /* EXHAUST GAS TEMP, DEG C     */
	int     egtm;                   /* EGT MARGIN, DEG C           */
	int     n1;                     /* FAN SPEED, PERCENT * 10     */
	int     n2;                     /* CORE SPEED, PERCENT * 10    */
	int     ff;                     /* FUEL FLOW, KG/HR            */
	int     oilp;                   /* OIL PRESSURE, PSI           */
	int     oilt;                   /* OIL TEMPERATURE, DEG C      */
	int     vib;                    /* BROADBAND VIBRATION, *10    */
	long    cyc;                    /* CYCLES SINCE NEW AT REPORT  */
	int     next;                   /* CHAIN TO NEXT REPORT, -1 END*/
};

/*  WORKSHOP INDUCTION SLOT.  ONE PER SHOP PER WEEK COMMENCING.       */

struct sltrec {
	char    shop[SHPSIZ+1];         /* SHOP CODE, E.G. DBY1         */
	char    wkcom[DATSIZ+1];        /* WEEK COMMENCING, YYMMDD      */
	int     cap;                    /* BAYS AVAILABLE               */
	int     used;                   /* BAYS TAKEN BY THE PLAN       */
};

/*  ONE LINE OF THE INDUCTION PLAN PRODUCED BY WRKSCD().              */

struct plnrec {
	char    esn[ESNSIZ+1];
	char    shop[SHPSIZ+1];
	char    wkcom[DATSIZ+1];
	int     prio;                   /* 1 = HIGHEST                  */
	int     wscope;                 /* SEE WORKSCOPE CODES BELOW    */
	int     tat;                    /* TURN ROUND TIME, DAYS        */
	int     placed;                 /* 0 = COULD NOT BE PLACED      */
};

/* ------ WORKSCOPE CODES ------------------------------------------- */

#define WSNONE  0
#define WSINSP  1               /* BORESCOPE / MINOR INSPECTION       */
#define WSHOTS  2               /* HOT SECTION REFURBISHMENT          */
#define WSPERF  3               /* PERFORMANCE RESTORATION            */
#define WSFULL  4               /* FULL OVERHAUL, LLP REPLACEMENT     */

/* ------ GLOBAL TABLES.  DEFINED IN DBIO.C. ------------------------ */

extern struct engrec engtab[MAXENG];
extern struct fltrec flttab[MAXFLT];
extern struct sltrec slttab[MAXSLT];
extern struct plnrec plntab[MAXPLN];

extern int nengs;               /* ENGINES LOADED                     */
extern int nflts;               /* FLIGHT REPORTS LOADED              */
extern int nslts;               /* SLOTS LOADED                       */
extern int nplns;               /* PLAN LINES BUILT                   */
extern int calcdn;              /* NON ZERO ONCE EHMCALC HAS RUN      */
extern char runday[DATSIZ+1];   /* RUN DATE, YYMMDD, SET BY MAIN      */

/* ------ ENTRY POINTS.  MODULES CARRYING PROTOTYPES ARE MARKED, THE
 *        REMAINDER ARE STILL DECLARED IN THE K AND R FORM.
 */

extern int ldfleet(char *fname);        /* DBIO.C, PROTOTYPED         */
extern int ldflts(char *fname);
extern int ldslot(char *fname);
extern int fndeng(char *esn);
extern int ehmcalc(void);               /* EHMCALC.C, PROTOTYPED      */
extern char *aretxt(char *out, int areas);
extern char *alrtxt(int lev);
extern int wrkscd(void);                /* WRKSCD.C, PROTOTYPED       */
extern char *wstext(int ws);
extern int rptflt();            /* RPTGEN.C                           */
extern int rptalr();
extern int rptpln();
extern int rpteng();
extern char *strim();           /* STRUTL.C                           */
extern char *upcase();
extern char *fldcpy();
extern long dtoday();
extern int daydif();
extern int loginit();           /* ERRLOG.C                           */
extern int logmsg();
extern int errmsg();

#endif
