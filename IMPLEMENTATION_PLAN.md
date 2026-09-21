# Warmup Assignment #2 — Implementation Plan

CSCI 402, Fall 2026. Token-bucket filter emulation in C with pthreads.

**Due:** 11:45PM 10/2/2026 (firm)
**Workspace:** `~/Desktop/Shared-ubuntu/warmup2`

Work **one milestone at a time**. Finish the “Done when” checklist, stop, and only then start the next one. Do not skip ahead (especially not SIGINT or trace files before the deterministic 4-thread path is correct).

```
M1 skeleton  →  M2 arrival only  →  M3 tokens + bucket
     →  M4 Q1/Q2 move  →  M5 servers  →  M6 stats
     →  M7 tsfile  →  M8 SIGINT  →  M9 grade / submit
```

---

## How to use this plan

1. Open the next milestone that is not checked off.
2. Implement **only** the “Build” list. Ignore later files and features.
3. Run the “Done when” commands. If any fail, stay on this milestone.
4. Check the milestone box at the top of that section, then stop or continue.

Hard rules that apply from the first line of C you write (do not violate them “just for now”):

- One mutex over Q1, Q2, and the token bucket (once those exist).
- `pthread_cond_broadcast` only. Never `pthread_cond_signal`. No semaphores.
- No busy-wait. No packet arrays. `malloc` a packet on arrival, free on depart/drop/remove.
- Separate `.c` files; headers are declarations only.
- Clock + print under the same mutex lock once threads exist.
- Keep time in `struct timeval` (`gettimeofday` / `timeradd` / `timersub`), not `double`.
- Spec and code stay private. No public GitHub.

Reuse from Warmup 1 (sibling folder): `../warmup1/my402list.h`, `../warmup1/my402list.c`, `../cs402.h`.

Target layout (files appear gradually; do not create empty stubs for M8 in M1):

```
warmup2/
  Makefile
  cs402.h  my402list.h  my402list.c
  warmup2.h   warmup2.c
  args.c      timeutil.c
  arrival.c   token.c    server.c    stats.c    # added in later milestones
  w2-README.txt                                 # M9
```

---

## Milestone map


| #   | Milestone      | You can run                                       | You cannot do yet          |
| --- | -------------- | ------------------------------------------------- | -------------------------- |
| 1   | Skeleton **done** | `make warmup2`; prints Parameters + begins/ends   | threads, queues, sleep     |
| 2   | Arrival thread **done** | packets print `arrives` (and drops) on a schedule | tokens, Q1, servers        |
| 3   | Token thread   | tokens fill/drop in the bucket                    | Q1/Q2, servers             |
| 4   | Q1 / Q2        | packets wait for tokens and move Q1→Q2            | service, stats, SIGINT     |
| 5   | Servers        | full 7-line packet traces; emulation drains       | `%.6g` stats, `-t`, Ctrl+C |
| 6   | Statistics     | Statistics block after `emulation ends`           | tsfile, SIGINT             |
| 7   | Trace-driven   | `-t tsfile` line-at-a-time                        | SIGINT                     |
| 8   | SIGINT         | Ctrl+C removes queues, finishes in-flight         | grading scripts            |
| 9   | Grade / submit | scripts + `warmup2.tar.gz`                        | —                          |


---

## Milestone 1 — Skeleton [done]

**Goal:** a program that compiles with `make warmup2` and prints Emulation Parameters plus timed `emulation begins` / `emulation ends`. No threads.

### Build

- Copy `cs402.h`, `my402list.h`, `my402list.c` from Warmup 1.
  - located in root directory
- `Makefile`: `make warmup2` links `warmup2.o args.o timeutil.o my402list.o` with `-lpthread -g -Wall`.
- `args.c`: parse
  ```
  warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]
  ```
  Options in any order. stdout for normal output, stderr for errors.

  | Flag      | Default | Constraint                   |
  | --------- | ------- | ---------------------------- |
  | `-lambda` | 1       | real `> 0`                   |
  | `-mu`     | 0.35    | real `> 0`                   |
  | `-r`      | 1.5     | real `> 0`                   |
  | `-B`      | 10      | integer in `(0, 2147483647]` |
  | `-P`      | 3       | integer in `(0, 2147483647]` |
  | `-n`      | 20      | integer in `(0, 2147483647]` |

  If `-t` is present, record the filename and **ignore** `-lambda`, `-mu`, `-P`, `-n` (you will not open the file until M7). Print unadjusted values (`lambda = 0.01` stays `0.01`).
- `timeutil.c`: `now()`, `elapsed_since_t0()`, print a timestamp as `00000000.000ms` (8+3 zero-padded, millisecond display of a microsecond `timeval`).
- `warmup2.c`: parse → print Parameters → `gettimeofday` into `t0` → print `emulation begins` → immediately print `emulation ends`. No threads.

Parameters block (omit the marked lines when `-t` is set):

```
Emulation Parameters:
    number to arrive = 20
    lambda = 2                    (omit if -t)
    mu = 0.35                     (omit if -t)
    r = 4
    B = 10
    P = 3                         (omit if -t)
    tsfile = FILENAME             (only if -t)
```

### Do not do yet

Threads, mutex, queues, sleeping, statistics, opening a tsfile, SIGINT.

### Done when

```
make warmup2
./warmup2
./warmup2 -n 5 -lambda 0.01 -mu 2 -r 4 -B 7 -P 2
./warmup2 -t /tmp/does-not-need-to-exist-yet
```

- First command produces `./warmup2`.
- Default run prints defaults (`n=20`, `lambda=1`, `mu=0.35`, `r=1.5`, `B=10`, `P=3`) and `00000000.000ms: emulation begins` then a slightly later `emulation ends`.
- Second run prints the values you passed, including `lambda = 0.01` (not `0.1`).
- Third run omits lambda/mu/P/n and shows `tsfile = ...`.
- Bad args (missing value, `B=0`, unknown flag) go to stderr and exit non-zero.

---

## Milestone 2 — Arrival thread only [done]

**Goal:** one thread produces `n` deterministic packets on the inter-arrival schedule and prints each `arrives` line. No token thread, no queues, no servers.

### Build

- `warmup2.h`: `Packet` (id, tokens, service_ms, `t_arrive` for now) and a small `Shared` (mutex, `t0`, parsed args, `arrived_count`).
- `arrival.c`: one thread.
- `warmup2.c`: init mutex, print Parameters, print `emulation begins` under the lock (clock+print), create arrival, join it, print `emulation ends`.

Deterministic packet fields:

- Inter-arrival target: `1/lambda` rounded to nearest ms (`cs402.h` `round()`), **max 10s**.
- Tokens needed: `P`.
- Service (store it now, unused until M5): `1/mu` rounded to nearest ms, **max 10s**.

Sleep (unlocked) is best-effort against the last **actual** arrival, not a fixed epoch grid:

```
# p0 is fictional at t0 / time 0
expected = last_actual + target_interval
remaining = expected - now
if remaining > 0: sleep remaining
else:             skip sleep          # never sleep a negative interval
```

Example: target 1000ms, p1 actually arrives at 1004ms → p2 is aimed at 2004ms, not 2000ms.

Never hold the mutex across `usleep`/`select`. After sleep: lock → `gettimeofday` → print → unlock.

Print (dropped if `P > B`):

```
p1 arrives, needs 3 tokens, inter-arrival time = 503.112ms
p1 arrives, needs 3 tokens, inter-arrival time = 503.112ms, dropped
```

Inter-arrival printed = this `arrives` timestamp − previous packet `arrives` (`p0` at 0). That is a measured interval, not the target.

After packet `n`, the thread returns. Main joins, then `emulation ends`.

### Do not do yet

Token thread, Q1/Q2, servers, stats block, `-t` reading, SIGINT. Dropped packets are only the `P > B` case.

### Done when

```
./warmup2 -n 3 -lambda 2 -P 3 -B 10
./warmup2 -n 2 -P 20 -B 5
```

- First run: three `pK arrives` lines ~500ms apart (measured, not exactly `500.000`).
- Second run: both packets print `, dropped` and never mention Q1.
- Timestamps are 8+3, never decrease.
- Process exits by itself after the last packet (no hang).
- `top` while it is sleeping does not show `warmup2` above ~0.5% CPU.

---

## Milestone 3 — Token thread and bucket

**Goal:** a second thread deposits tokens at `1/r`. The bucket has depth `B`. Overflow tokens print `dropped`. Still no Q1/Q2.

### Build

- `token.c`.
- `Shared`: `tokens` (0..B), `token_count`, `no_more_packets`.
- Arrival, after the last packet: set `no_more_packets` and `pthread_cond_broadcast` (add the CV now even though nothing waits on it yet — servers will).
- Token thread: stop as soon as `no_more_packets` is set (Q1 does not exist yet, so “Q1 empty and no future packets” is just “no future packets”).

Token interval: `1/r` rounded to nearest **microsecond** if `≤ 10s`, else 10s. Same best-effort sleep vs last **actual** token arrival. First token is relative to `t0`.

```
token t1 arrives, token bucket now has 1 token
token t1 arrives, dropped
```

Grammar: `1 token`; `0 token` (match the spec sample); `N tokens` for `N != 1`. When grading scripts disagree, follow the scripts.

Main creates arrival + token, joins both, then `emulation ends`.

### Do not do yet

Q1, Q2, moving packets, servers, stats.

### Done when

```
./warmup2 -n 1 -lambda 1 -r 4 -B 3 -P 1
```

- Tokens `t1`… appear about every 250ms.
- Bucket climbs `1 token`, `2 tokens`, `3 tokens`, then further tokens print `dropped` once full (p1 may consume nothing yet — that is OK).
- After p1 arrives, the token thread **stops** (does not run forever).
- Process exits cleanly.

---

## Milestone 4 — Q1, Q2, and the move rules

**Goal:** packets enter Q1, wait for tokens if needed, and move into Q2. No servers. Packets that reach Q2 sit there until main drains them after both producer threads join (temporary, documented in code as M5-will-replace).

### Build

- Init `My402List Q1`, `Q2` in `Shared`.
- Arrival and token implement the real bucket rules below.
- Helper (used by both threads): `try_move_head_q1_to_q2()` — if Q1 head needs `<=` bucket tokens, subtract, unlink, print leave-Q1 + enter-Q2, `broadcast`. Call it only from the paths the spec allows.

**Arrival**

1. Sleep remaining inter-arrival (unlocked).
2. Lock. If you already have a shutdown flag, leave it unused until M8.
3. Timestamp `arrives`. If `tokens > B`: print `, dropped`, do not enqueue.
4. Else always append to Q1 and print `enters Q1` (even on the fast path).
5. If Q1 was **not** empty before this append: stop. Do **not** look at the bucket.
6. If Q1 **was** empty and `bucket >= tokens_needed`: remove tokens, leave Q1, enter Q2, `broadcast`.
7. After packet `n`: `no_more_packets = 1`, broadcast, return.

**Token**

1. Sleep remaining inter-token (unlocked).
2. Lock. Exit if shutdown **or** (`no_more_packets` && Q1 empty).
3. Accept or drop the token (print).
4. If Q1 is non-empty and head is eligible: move **only the head**. Bucket is then empty — do not inspect the new head. `broadcast`.

Leave-Q1 line:

```
p1 leaves Q1, time in Q1 = 247.810ms, token bucket now has 0 token
```

`time in Q1` = leave timestamp − enter timestamp.

After join arrival + token, main (still temporary): lock, unlink every Q2 packet, free it, print nothing extra, unlock, then `emulation ends`. This is only so M4 exits; M5 deletes this drain.

### Do not do yet

Server threads, service sleep, 7-line depart traces, statistics, `-t`, SIGINT.

### Done when

Hand-check these three runs (predict before you run):

```
# Fast path: enough tokens by the time p1 arrives; Q1 time tiny; lands in Q2
./warmup2 -n 1 -lambda 2 -r 20 -B 10 -P 3

# Wait in Q1: slow tokens; token thread is the mover; p1 sits in Q1
./warmup2 -n 1 -lambda 10 -r 1 -B 10 -P 3

# Second packet must only enqueue while p1 is still in Q1
./warmup2 -n 2 -lambda 10 -r 1 -B 10 -P 3
```

- Fast path still has `enters Q1` then `leaves Q1` then `enters Q2` (do not skip Q1).
- Slow-token run: several `token tK arrives` **before** `p1 leaves Q1`.
- Two-packet slow run: p2 `enters Q1` while p1 is still in Q1; p2 must not leave before p1 (FCFS).
- Token thread stops once Q1 is empty and no more packets will arrive.
- Process exits (main’s temporary Q2 drain).

---

## Milestone 5 — Servers S1 and S2

**Goal:** two server threads drain Q2. A served packet has exactly seven chronological trace lines. Emulation ends when the last packet departs. Delete main’s temporary Q2 drain.

### Build

- `server.c`: one function, argument is server id `1` or `2`.
- Main creates S1 and S2 with arrival and token; joins all four; then `emulation ends`.

```
lock
loop:
    while (Q2 empty && !all_done)
        pthread_cond_wait(&cv, &mutex)
    if (Q2 empty && all_done)
        unlock; return
    unlink Q2 head
    timestamp leave-Q2 and begin-service     # still holding mutex
    unlock
    sleep FULL requested service_ms          # do NOT subtract bookkeeping
    lock
    timestamp depart
    free(packet)
    # loop immediately — keep busy
unlock
```

`all_done` = `no_more_packets` && Q1 empty && Q2 empty (and, later, shutdown after drain). Broadcast whenever that becomes true so a waiting server does not hang.

Served packet — **exactly 7 lines**:

```
p1 arrives, needs 3 tokens, inter-arrival time = 503.112ms
p1 enters Q1
p1 leaves Q1, time in Q1 = 247.810ms, token bucket now has 0 token
p1 enters Q2
p1 leaves Q2, time in Q2 = 0.216ms
p1 begins service at S1, requesting 2850ms of service
p1 departs from S1, service time = 2859.911ms, time in system = 3109.731ms
```


| Printed field  | Must equal                                         |
| -------------- | -------------------------------------------------- |
| inter-arrival  | this arrives − previous arrives (`p0` at 0)        |
| time in Q1     | leave Q1 − enter Q1                                |
| time in Q2     | leave Q2 − enter Q2                                |
| requesting Nms | integer **requested** service                      |
| service time   | depart − begin (measured; should be **>** request) |
| time in system | depart − arrive                                    |


Dropped packet is still exactly one `arrives ... dropped` line.

`n = 1`: one server works, the other waits on the CV and must exit when `all_done` is broadcast.

### Do not do yet

Statistics block, opening a tsfile, SIGINT.

### Done when

```
./warmup2 -n 2 -lambda 2 -mu 5 -r 20 -B 10 -P 3
./warmup2 -n 1 -lambda 2 -mu 5 -r 20 -B 10 -P 3
```

- Two-packet run: both packets have 7 lines; S1 and S2 can each take one if service overlaps.
- One-packet run: exits; the idle server does not hang.
- `requesting` is an integer ms; printed `service time` is larger and has 3 decimals.
- Timestamps stay monotonic.
- No Statistics section yet.

---

## Milestone 6 — Statistics

**Goal:** after `emulation ends`, print the Statistics block from running sums (no per-packet history array).

### Build

- `stats.c`. Update counters at the moment each event happens (arrival, drop, token drop, Q1/Q2/S enter-leave, depart).
- Print with `%.6g`. Units for time stats: **seconds**, not milliseconds. Never `NaN`. If a denominator is 0, print `N/A` and a short reason (`no packet was served`).

```
Statistics:

    average packet inter-arrival time = <real-value>
    average packet service time = <real-value>
    average number of packets in Q1 = <real-value>
    average number of packets in Q2 = <real-value>
    average number of packets at S1 = <real-value>
    average number of packets at S2 = <real-value>
    average time a packet spent in system = <real-value>
    standard deviation for time spent in system = <real-value>
    token drop probability = <real-value>
    packet drop probability = <real-value>
```


| Packet fate | Meaning                                           |
| ----------- | ------------------------------------------------- |
| Completed   | finished service at a server                      |
| Dropped     | arrived, never entered Q1 (`tokens > B`)          |
| Removed     | entered a queue, never reached a server (M8 only) |



| Statistic                                  | Who counts                                       |
| ------------------------------------------ | ------------------------------------------------ |
| avg inter-arrival, packet drop probability | **all arrived**                                  |
| avg service                                | **completed only**                               |
| avg packets in Q1/Q2/S1/S2                 | **completed only** (their time at that facility) |
| time in system, stddev                     | **completed only**                               |


- Facility average = (sum of time at that facility) / (timestamp of `emulation ends`).
- Packet drop prob = dropped / produced by arrival.
- Token drop prob = dropped-because-full / produced by token thread.
- Mean: divide by `n`, not `n-1`.
- Population variance: `Var(X) = E(X^2) - [E(X)]^2`, stddev = `sqrt`. Keep `sum_x` and `sum_x2` for time-in-system.

### Do not do yet

`-t` file reading, SIGINT. You may still run only deterministic mode.

### Done when

```
./warmup2 -n 2 -lambda 2 -mu 5 -r 20 -B 10 -P 3
./warmup2 -n 2 -P 20 -B 5 -lambda 2 -r 20
```

- First run: every statistic is a real number, seconds, no `NaN`.
- Second run: all dropped → service / Q / S / time-in-system print `N/A` + reason; inter-arrival and packet drop probability are still numbers (`drop = 1`).
- Hand-check one tiny run: avg service ≈ requested/1000; facility averages look like (time_in_facility / total_emulation_time).

---

## Milestone 7 — Trace-driven mode

**Goal:** `-t tsfile` drives inter-arrival, tokens, and service from the file. Do not read the whole file before `emulation begins`.

### Build

- After parse: if `-t`, open the file and read **only line 1** (`n`). Then print Parameters (with `tsfile = ...`, omit lambda/mu/P/n), then `emulation begins`.
- Arrival reads **one** data line per packet, as it needs it.

Format (each line ends in `\n`):

- Line 1: positive integer `n`.
- Line `k` (`k >= 2`): `interarrival_ms` `tokens` `service_ms` for packet `k-1`.
- Fields: any mix of spaces/tabs. No leading or trailing whitespace.
- Line longer than 1024 characters including `\n` is an error.

Sample (`n = 3`):

```
3
2716	2	9253
7721	1	15149
972	3	2614
```

These are **targets**. If a measured inter-arrival prints as exactly `972.000ms`, you printed the target or you are not sleeping — that is a bug.

On any format error: message to stderr, `exit()` immediately. **No statistics. No recovery.**

Trace-mode inter-arrival and service **may exceed 10s**. Token interval is still capped at 10s.

Zero-overhead sanity for `./warmup2 -t` on that sample (defaults `r=1.5`, `B=10`):


| Packet | Arrive  | Tokens | Service | What happens                    | Depart  |
| ------ | ------- | ------ | ------- | ------------------------------- | ------- |
| p1     | 2716ms  | 2      | 9253ms  | bucket ready, server free       | 11969ms |
| p2     | 10437ms | 1      | 15149ms | other server free               | 25586ms |
| p3     | 11409ms | 3      | 2614ms  | both busy → wait Q2; follows p1 | 14583ms |


Emulation must not finish faster than **25586ms**. Faster means a sleep bug.

Write your own tiny tsfiles and predict Q1-wait / Q2-wait / both before running.

### Do not do yet

SIGINT.

### Done when

```
./warmup2 -t sample.tsfile          # the 3-line sample
# wall clock ≥ ~25.6s; p3 waits in Q2; p2 is last to depart
```

- Format error file: stderr + immediate exit, no Statistics.
- Deterministic mode from M6 still works unchanged.

---

## Milestone 8 — SIGINT

**Goal:** Ctrl+C stops new arrivals/tokens, removes everything still in Q1/Q2 (timestamped), lets in-service packets finish, then prints Statistics. Add this only after M7 is solid.

### Build

Do **not** use `signal()` / `sigaction` for the trace line. Use a dedicated thread and `sigwait()`.

1. In `main`, **before** any `pthread_create`: block SIGINT with `pthread_sigmask` so every thread inherits the mask.
2. SIGINT thread: `sigwait` on that set.

On catch (under the mutex, clock+print):

```
<newline>????????.???ms: SIGINT caught, no new packets or tokens will be allowed
```

Then: set `shutdown`; arrival and token stop producing; walk Q1 and Q2 and for each packet print `pK removed from Q#`, unlink, free (removing is part of the emulation); broadcast; servers finish the packet they are **already** serving and must not take a new one; join; `emulation ends` + Statistics.

Removed packets: count in inter-arrival and drop probability populations only. Not in service / occupancy / time-in-system.

### Do not do yet

Official grading scripts (M9). Do not run those from this shared folder.

### Done when

Start a long run (`-n 20` or a slow tsfile). Hit Ctrl+C while packets are in queues:

- Leading newline + timestamped SIGINT line.
- Every leftover Q1/Q2 packet has a `removed` line (timestamped).
- In-service packet(s) still `depart`.
- Statistics print with no `NaN`.
- Process exits.

---

## Milestone 9 — Grade and submit

**Goal:** official scripts + tarball. This workspace is `Shared-ubuntu` — **copy the tree off the share first** (e.g. `~/cs402-warmup2-grade`). Scripts misbehave in shared folders.

### Build

- Fill `w2-README.txt` from the course template. **Do not delete any line.**
- Copy `section-A-all.sh`, `section-B-all.sh`, unpack `w2data.tar.gz` into the **non-shared** copy.
- `chmod 755 section-*.sh`.
- Use `analyze-trace.txt` on a clean 7-line / 1-line trace if a test looks wrong.

Section (A) tests each have a minimum emulation time. Too fast or much too slow = serious bug. The grader may use different argv/tsfiles; the published guidelines are the only grading procedure.

### Done when

- [ ] `make warmup2` on a clean 64-bit Ubuntu 20.04 tree
- [ ] Section A and B scripts run from the **non-shared** copy
- [ ] Separate `.c` files; headers are declarations only
- [ ] `w2-README.txt` is the official template, fields filled
- [ ] Tarball has **no** binaries (`.o`, `warmup2`, `core`, `.gch` — 2 points each)

```
tar cvzf warmup2.tar.gz Makefile *.c *.h w2-README.txt
tar tvzf warmup2.tar.gz
ls -l warmup2.tar.gz          # must be < 1MB
```

Upload to Bistro, verify the SHA1 ticket. Keep the code private.

---

## Appendix A — Always-on point-loss list

Check this when a milestone “works” but you are about to move on.

- Busy-wait instead of `pthread_cond_wait`
- `pthread_cond_signal` instead of `broadcast`
- Semaphores
- Packet array / reading the whole tsfile before start (M7)
- Signal handler instead of `sigwait` (M8)
- Clock and print not under the same mutex
- `double` timestamps
- Arrival/token slept the full interval without subtracting bookkeeping
- Server sleep **did** subtract bookkeeping
- Printed target times instead of measured times
- Printed capped/adjusted lambda/mu/r in Parameters
- `NaN` in Statistics
- Sample variance (`n-1`) instead of population
- Facility averages including dropped/removed packets
- Token thread still running after Q1 empty and no more packets
- Mutex held across `usleep`
- Arrival into non-empty Q1 tried to move the head
- Token path moved more than the head of Q1
- Fast path skipped enter-Q1 / leave-Q1
- Packet with `tokens > B` enqueued (blocks everyone behind it)

---

## Appendix B — Extra local tests (after M7)

Use these when you want more confidence before M8/M9. Predict the trace first.

1. Fast path, no queueing — large `r` and `B`, two packets, short service.
2. Wait in Q1 — tiny `r`; token thread moves the head.
3. Wait in Q2 — plenty of tokens, both servers busy (sample-file p3).
4. Wait in both — slow tokens and busy servers.
5. Drop — `tokens > B`; later packets still flow.
6. Token overflow — long first inter-arrival so the bucket fills and drops.
7. Deterministic 10s cap — `lambda = 0.01` sleeps 10s but Parameters still print `0.01`.
8. Late bookkeeping — if work overruns the interval, next sleep is skipped (not negative, not a full extra interval).

