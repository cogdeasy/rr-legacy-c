---
name: testing-ehm
description: How to build, run and adversarially test the EHM/MRO release 4.2C K&R C command-line system (batch listing, interactive menu, card-image loaders).
---

# Testing EHM/MRO (rr-legacy-c)

Pure stdio C program plus an overnight shell job. No server, no GUI, no credentials.
All testing is shell-only, so do **not** start a screen recording — collect text
evidence (listings, run log, exit statuses) instead.

## Devin Secrets Needed
None.

## Build

```
make clean && make                                   # stock cc, K&R sources
make clean && make CFLAGS="-O -I include -std=c89 -pedantic"
```
Both must exit 0. Compiler warnings about `sprintf` format-overflow are expected on
modern glibc and are not build failures. K&R style (missing prototypes, global
tables, `sprintf` into fixed buffers, two-digit dates) is intentional — never "fix" it.

## Deterministic run

Always pass an explicit run date; the default comes from the system clock and the
sample data is 1996-based, so today's date makes every 1996 shop-visit date look
~70 years in the future and the alert/plan counts differ from `make run`:

```
EHMDATA=data ./ehm -b -d 960209 -o /tmp/ehm.lis     # 48 engines / 342 reports / 31 alerts
./ehmnite.sh                                        # uses system clock -> different counts
```
`ehm.log` is appended to, not overwritten — `rm -f ehm.log` before each run when you
want to assert on message counts. Exit status = error count, capped at 63.

## Interactive menu (pipe stdin, it is a plain prompt loop)

```
printf '3\n5\n6\n2\n' | EHMDATA=data ./ehm      # out-of-order -> EHM-922/931/932/921
printf '1\n2\n3\n7\n00072101\nX\n' | EHMDATA=data ./ehm
```
Options 7 and 9 consume a second line (ESN / YYMMDD). EOF anywhere exits cleanly.

## Mutating data safely

Copy `data/*.dat` to a scratch dir, mutate the copies, point `EHMDATA` at the scratch
dir. Note `-f/-r/-s` values are always prefixed with `$EHMDATA` by `dsname()` in
`src/dbio.c`, so absolute override paths do not work while `EHMDATA` is set — use
file names relative to the scratch dir.

## Cross-checks worth automating (python over the listing)

- EHM-104 alert count == alert-report detail lines == (ACT OR ABOVE + ON WATCH) footer.
- EHM-107 candidates == `PLAN LINES`; placed == plan lines minus `UNPLACED`;
  `UNPLACED` == count of `NO SLOT AVAILABLE` lines == count of EHM-106 log lines;
  sum of `BAYS n OF m USED` == placed.
- No line over 132 columns (strip form feeds first) and no page over 60 lines.
- Reason letters vs limits in `include/ehm.h` (EGTMLO 15, EGTMWN 30, RATLIM 25,
  VIBWRN 35, VIBACT 50, LLPLIM 20000 cycles-since-new, DUEWIN 90 days).

Fixed column offsets for parsing (0-based), from the `fprintf` formats in
`src/rptgen.c`:
- fleet summary line: esn 0-7, csn 37-43, status 51, reps 53-57, egtm 59-64,
  rate 66-71, vib 73-77, cycrem 79-86, sv due 89-97, alert 100-103, reason 105-110.
- alert line: lvl 0-3, esn 5-12, egtm 34-39, rate 41-46, vib 48-53, cycrem 55-63,
  sv due 66-74, reason 77-82.

## Known fragile areas (probe these, report rather than patch)

- Long paths in the *error* path abort the process: the failing data-set name is
  `sprintf`'d into a 132-byte `work` buffer (`EHM-910/915/918` in `src/dbio.c`,
  `EHM-940` in `src/ehmmain.c`). An `EHMDATA` or `-f/-r/-s/-o` path over ~99 chars
  that cannot be opened aborts with SIGABRT on glibc.
- `-f/-r/-s/-o` argument values are `strcpy`'d into 256-byte statics (unbounded).
- A short fleet card inherits the previous card's serviceability byte because
  `e->status = card[FL_STA]` indexes the static card buffer directly.
These are intentional legacy style per the maintainers; record them as findings.

## Memory safety

```
make clean && make CC=gcc CFLAGS="-O1 -g -I include -fsanitize=address,undefined" \
     LDFLAGS="-fsanitize=address,undefined"
valgrind --error-exitcode=9 ./ehm -b -d 960209 -o /dev/null   # expect 0 errors
```
Table-limit paths (EHM-911/916/919/923) need generated files with >200 fleet cards,
>4000 flight cards, >60 slot cards and >120 alerting engines.
