/*
 *----------------------------------------------------------------------
 *  DBIO.H    FLAT FILE RECORD LAYOUTS FOR THE EHM DATA SETS
 *
 *  THE DATA SETS ARE SEQUENTIAL CARD IMAGE FILES OF 80 COLUMNS.  EVERY
 *  FIELD IS FIXED POSITION.  COLUMN NUMBERS BELOW ARE ONE RELATIVE AS
 *  PRINTED IN THE FILE FORMAT NOTE, THE OFFSETS ARE ZERO RELATIVE FOR
 *  USE WITH FLDCPY().
 *
 *  A LINE WHOSE FIRST COLUMN IS AN ASTERISK IS A COMMENT AND IS SKIPPED.
 *----------------------------------------------------------------------
 */

#ifndef DBIO_H
#define DBIO_H

#define CARD    81              /* 80 COLUMNS PLUS TERMINATOR         */
#define CMTCHR  '*'

/*  FLEET MASTER  -  FLEET.DAT                                        */

#define FL_ESN  0               /* COL  1- 8  ENGINE SERIAL NUMBER    */
#define FL_TYP  9               /* COL 10-15  ENGINE MARK             */
#define FL_REG  16              /* COL 17-23  AIRFRAME REGISTRATION   */
#define FL_OPR  24              /* COL 25-28  OPERATOR CODE           */
#define FL_TSN  29              /* COL 30-36  HOURS SINCE NEW         */
#define FL_CSN  37              /* COL 38-44  CYCLES SINCE NEW        */
#define FL_BST  45              /* COL 46-49  BUILD STANDARD          */
#define FL_STA  50              /* COL 51      SERVICEABILITY CODE    */
#define FL_DUE  52              /* COL 53-58  SHOP VISIT DUE YYMMDD   */

/*  FLIGHT REPORTS  -  FLIGHTS.DAT                                    */

#define FR_ESN  0               /* COL  1- 8  ENGINE SERIAL NUMBER    */
#define FR_DAT  9               /* COL 10-15  FLIGHT DATE YYMMDD      */
#define FR_FLT  16              /* COL 17-22  FLIGHT NUMBER           */
#define FR_EGT  23              /* COL 24-27  EGT DEG C               */
#define FR_EGM  28              /* COL 29-32  EGT MARGIN DEG C        */
#define FR_N1   33              /* COL 34-37  N1 PERCENT * 10         */
#define FR_N2   38              /* COL 39-42  N2 PERCENT * 10         */
#define FR_FF   43              /* COL 44-48  FUEL FLOW KG/HR         */
#define FR_OIP  49              /* COL 50-52  OIL PRESSURE PSI        */
#define FR_OIT  53              /* COL 54-56  OIL TEMP DEG C          */
#define FR_VIB  57              /* COL 58-60  VIBRATION * 10          */
#define FR_CYC  61              /* COL 62-68  CYCLES SINCE NEW        */

/*  WORKSHOP SLOTS  -  SLOTS.DAT                                      */

#define SL_SHP  0               /* COL  1- 4  SHOP CODE               */
#define SL_WKC  5               /* COL  6-11  WEEK COMMENCING YYMMDD  */
#define SL_CAP  12              /* COL 13-14  BAYS AVAILABLE          */

/*  SPARES AND ROTABLE POOL  -  SPARES.DAT                            */

#define SP_PNO  0               /* COL  1-10  PART NUMBER             */
#define SP_DSC  11              /* COL 12-35  PART DESCRIPTION        */
#define SP_TYP  36              /* COL 37-42  ENGINE MARK FITTED TO   */
#define SP_WSC  43              /* COL 44-47  WORKSCOPE MNEMONIC      */
#define SP_QOH  48              /* COL 49-52  QUANTITY ON HAND        */
#define SP_QOO  53              /* COL 54-57  QUANTITY ON ORDER       */
#define SP_LED  58              /* COL 59-61  LEAD TIME, DAYS         */
#define SP_ROT  62              /* COL 63      R ROTABLE C CONSUMABLE */

/*  DEFAULT DATA SET NAMES.  OVERRIDDEN BY THE EHMDATA ENVIRONMENT
 *  VARIABLE ON UNIX, OR BY THE EHM$DATA LOGICAL NAME ON VMS.
 */

#define DSFLET  "fleet.dat"
#define DSFLTS  "flights.dat"
#define DSSLTS  "slots.dat"
#define DSSPRS  "spares.dat"
#define DSLOG   "ehm.log"

#endif
