/*
 *----------------------------------------------------------------------
 *  STRUTL.C  CHARACTER AND DATE HANDLING PRIMITIVES
 *
 *  THESE ROUTINES ARE USED BY EVERY MODULE.  THEY WORK ON THE FIXED
 *  LENGTH CARD IMAGE FIELDS DESCRIBED IN DOC/FILEFMT.DOC AND ON THE
 *  SIX DIGIT DATES USED THROUGHOUT THE SYSTEM.
 *
 *  NOTE   THE DATE ROUTINES ASSUME THE NINETEEN HUNDREDS.  THE FLEET
 *         RECORDS WILL NOT CARRY A SHOP VISIT DUE DATE BEYOND 991231
 *         SO THIS IS ADEQUATE FOR THE PLANNING HORIZON.
 *
 *  R.T.H.  14-MAR-1988
 *----------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ehm.h"

/*  DAYS IN EACH MONTH, JANUARY FIRST.  FEBRUARY IS PATCHED IN DTODAY. */

static int mdays[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

/*
 *  STRIM  -  STRIP TRAILING BLANKS FROM S IN PLACE AND RETURN S.
 */

char *strim(s)
char *s;
{
	register char *p;

	if (s == NULL)
		return (s);
	p = s;
	while (*p != '\0')
		p++;
	while (p > s && (*(p-1) == ' ' || *(p-1) == '\t' ||
			 *(p-1) == '\n' || *(p-1) == '\r'))
		p--;
	*p = '\0';
	return (s);
}

/*
 *  UPCASE  -  FOLD S TO UPPER CASE IN PLACE AND RETURN S.
 */

char *upcase(s)
char *s;
{
	register char *p;

	for (p = s; p != NULL && *p != '\0'; p++)
		if (*p >= 'a' && *p <= 'z')
			*p = *p - 'a' + 'A';
	return (s);
}

/*
 *  FLDCPY  -  COPY N COLUMNS FROM CARD, STARTING AT COLUMN OFF, INTO
 *             DST.  THE RESULT IS BLANK STRIPPED AND TERMINATED.  IF
 *             THE CARD IS SHORT THE MISSING COLUMNS ARE TAKEN AS BLANK.
 */

char *fldcpy(dst, card, off, n)
char *dst;
char *card;
int off;
int n;
{
	register int i;
	int len;

	len = strlen(card);
	for (i = 0; i < n; i++) {
		if (off + i < len)
			dst[i] = card[off+i];
		else
			dst[i] = ' ';
	}
	dst[n] = '\0';
	return (strim(dst));
}

/*
 *  FLDINT  -  AS FLDCPY BUT RETURNS THE FIELD AS AN INTEGER.  A BLANK
 *             FIELD RETURNS ZERO.  A LEADING MINUS IS HONOURED.
 */

int fldint(card, off, n)
char *card;
int off;
int n;
{
	char buf[32];
	register char *p;
	int sign;
	int val;

	if (n > 30)
		n = 30;
	fldcpy(buf, card, off, n);
	p = buf;
	while (*p == ' ')
		p++;
	sign = 1;
	if (*p == '-') {
		sign = -1;
		p++;
	}
	val = 0;
	while (*p >= '0' && *p <= '9') {
		val = val * 10 + (*p - '0');
		p++;
	}
	return (sign * val);
}

/*
 *  FLDLNG  -  AS FLDINT BUT FOR LONG QUANTITIES SUCH AS CYCLES.
 */

long fldlng(card, off, n)
char *card;
int off;
int n;
{
	char buf[32];
	register char *p;
	long val;
	int sign;

	if (n > 30)
		n = 30;
	fldcpy(buf, card, off, n);
	p = buf;
	while (*p == ' ')
		p++;
	sign = 1;
	if (*p == '-') {
		sign = -1;
		p++;
	}
	val = 0L;
	while (*p >= '0' && *p <= '9') {
		val = val * 10L + (long)(*p - '0');
		p++;
	}
	return (sign < 0 ? -val : val);
}

/*
 *  ISDATE  -  RETURN NON ZERO IF D IS SIX DIGITS AND LOOKS LIKE YYMMDD.
 */

int isdate(d)
char *d;
{
	register int i;
	int mm;
	int dd;

	if (d == NULL || strlen(d) != 6)
		return (0);
	for (i = 0; i < 6; i++)
		if (d[i] < '0' || d[i] > '9')
			return (0);
	mm = (d[2] - '0') * 10 + (d[3] - '0');
	dd = (d[4] - '0') * 10 + (d[5] - '0');
	if (mm < 1 || mm > 12)
		return (0);
	if (dd < 1 || dd > 31)
		return (0);
	return (1);
}

/*
 *  DTODAY  -  CONVERT A YYMMDD DATE TO A DAY NUMBER COUNTED FROM
 *             01-JAN-1900.  RETURNS -1L IF THE DATE IS NOT VALID.
 */

long dtoday(d)
char *d;
{
	int yy;
	int mm;
	int dd;
	int y;
	int m;
	long days;

	if (!isdate(d))
		return (-1L);

	yy = (d[0] - '0') * 10 + (d[1] - '0');
	mm = (d[2] - '0') * 10 + (d[3] - '0');
	dd = (d[4] - '0') * 10 + (d[5] - '0');

	days = 0L;
	for (y = 0; y < yy; y++) {
		days = days + 365L;
		if ((y % 4) == 0 && y != 0)     /* 1900 WAS NOT A LEAP YEAR */
			days = days + 1L;
	}
	for (m = 0; m < mm - 1; m++) {
		days = days + (long)mdays[m];
		if (m == 1 && (yy % 4) == 0 && yy != 0)
			days = days + 1L;
	}
	return (days + (long)dd);
}

/*
 *  DAYDIF  -  DAYS FROM DATE A TO DATE B.  NEGATIVE IF B IS EARLIER.
 *             ZERO IS RETURNED IF EITHER DATE IS INVALID, THE CALLER
 *             IS EXPECTED TO HAVE CHECKED WITH ISDATE FIRST.
 */

int daydif(a, b)
char *a;
char *b;
{
	long da;
	long db;

	da = dtoday(a);
	db = dtoday(b);
	if (da < 0L || db < 0L)
		return (0);
	return ((int)(db - da));
}

/*
 *  DATFMT  -  EXPAND A YYMMDD DATE INTO DD-MMM-YY FOR PRINTING.  THE
 *             RESULT IS PLACED IN OUT WHICH MUST HOLD TEN CHARACTERS.
 */

char *datfmt(out, d)
char *out;
char *d;
{
	static char mon[12][4] = {
		"JAN","FEB","MAR","APR","MAY","JUN",
		"JUL","AUG","SEP","OCT","NOV","DEC"
	};
	int mm;

	if (!isdate(d)) {
		strcpy(out, "  -   -  ");
		return (out);
	}
	mm = (d[2] - '0') * 10 + (d[3] - '0');
	sprintf(out, "%c%c-%s-%c%c", d[4], d[5], mon[mm-1], d[0], d[1]);
	return (out);
}

/*
 *  ADDDAY  -  ADD N DAYS TO THE YYMMDD DATE D, RESULT IN OUT.  USED BY
 *             THE SCHEDULER TO STEP ON A WEEK AT A TIME.  THE ROUTINE
 *             WORKS BY REPEATED INCREMENT WHICH IS QUITE FAST ENOUGH
 *             FOR THE SMALL NUMBERS OF DAYS INVOLVED.
 */

char *addday(out, d, n)
char *out;
char *d;
int n;
{
	int yy;
	int mm;
	int dd;
	int lim;
	register int i;

	if (!isdate(d)) {
		strcpy(out, d);
		return (out);
	}
	yy = (d[0] - '0') * 10 + (d[1] - '0');
	mm = (d[2] - '0') * 10 + (d[3] - '0');
	dd = (d[4] - '0') * 10 + (d[5] - '0');

	for (i = 0; i < n; i++) {
		lim = mdays[mm-1];
		if (mm == 2 && (yy % 4) == 0 && yy != 0)
			lim = 29;
		dd++;
		if (dd > lim) {
			dd = 1;
			mm++;
			if (mm > 12) {
				mm = 1;
				yy++;
				if (yy > 99)    /* WRAPS, SEE NOTE ABOVE */
					yy = 0;
			}
		}
	}
	sprintf(out, "%02d%02d%02d", yy, mm, dd);
	return (out);
}
