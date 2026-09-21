# Token Bucket Emulator

A time-driven, multithreaded emulator of a [token-bucket](https://en.wikipedia.org/wiki/Token_bucket) traffic shaper in C. Packets arrive on a schedule, wait for tokens, then share two servers. The program emits a millisecond-resolution event trace and queueing statistics you can use to reason about loss, delay, and occupancy.

**C · pthreads · POSIX condition variables · `sigwait` · `struct timeval`**

## What it models

Token bucket is the rate limiter behind traffic shaping and API quotas: a bucket of depth `B` refills at rate `r`. A packet may enter service only after it has collected `P` tokens. If it needs more tokens than the bucket can ever hold, it is dropped so it cannot stall the line.

This emulator runs that policy in wall-clock time with real threads, not a discrete-event calendar. Five threads share one mutex and one condition variable:

```
                    tokens @ rate r, depth B
                              │
                              ▼
  arrivals ──► Q1 (wait for tokens) ──► Q2 (wait for a server) ──► S1
                                                                  S2
  SIGINT ──► stop producers, drain Q1/Q2, finish in-flight work
```

| Thread | Role |
|---|---|
| Arrival | Sleeps the inter-arrival interval, allocates a packet, enqueues Q1 |
| Token | Deposits one token per interval; moves only the **head** of Q1 when it has enough tokens |
| S1 / S2 | Dequeue Q2 and sleep the requested service time (no bookkeeping subtracted) |
| SIGINT | Blocks in `sigwait` (not a signal handler); drains queues and lets in-service packets depart |

Wakeups use `pthread_cond_broadcast` only — no semaphores, no busy-wait.

## Design choices that matter

- **Clock and print under the same lock.** Every timestamped line is generated while the mutex is held, so the trace is monotonically increasing even when four workers race.
- **Best-effort producer sleep.** Arrival and token threads subtract bookkeeping from the last *actual* event and skip sleep if they are already late. Servers always sleep the full requested service time so measured service matches the request.
- **Infinite queues, no packet array.** Packets are `malloc`'d on arrival and freed on depart, drop, or SIGINT remove. Q1/Q2 are doubly linked lists.
- **Running-sum statistics.** Occupancy (Q1, Q2, S1, S2) is accumulated as time-in-facility and divided by emulation length — Little's law, no per-packet history. Zero-population metrics print `N/A`, never `NaN`.
- **Graceful Ctrl+C.** `SIGINT` is blocked in every thread and consumed with `sigwait`. Leftover Q1/Q2 packets are timestamp-removed; packets already in service finish and count toward stats. A background job restores `SIG_DFL` first so `kill -INT` is not discarded by the shell.

## Two driving modes

**Deterministic** — inter-arrival `1/λ`, service `1/μ`, every packet needs `P` tokens. Intervals are rounded to the nearest millisecond and capped at 10 seconds; the token interval is nearest-microsecond with the same cap.

**Trace-driven** (`-t tsfile`) — ignore `λ`, `μ`, `P`, and `n`. After emulation begins, read one line per packet (`inter-arrival-ms  tokens  service-ms`). The file is not preloaded. A malformed line exits with a message and no statistics.

## Build

Requires `gcc` and pthreads.

```bash
make warmup2
```

## Usage

```text
./warmup2 [-lambda λ] [-mu μ] [-r r] [-B B] [-P P] [-n num] [-t tsfile]
```

Flags may appear in any order.

| Flag | Default | Meaning |
|---|---|---|
| `-lambda` | `1` | Arrival rate (packets / second) |
| `-mu` | `0.35` | Service rate (packets / second) |
| `-r` | `1.5` | Token rate (tokens / second) |
| `-B` | `10` | Bucket depth |
| `-P` | `3` | Tokens required per packet |
| `-n` | `20` | Packets to generate |
| `-t` | — | Trace file; overrides `λ`, `μ`, `P`, `n` |

Deterministic run:

```bash
./warmup2 -n 1 -lambda 2 -mu 5 -r 20 -P 3
```

Trace-driven run:

```bash
./warmup2 -t sample.tsfile
```

Interrupt a long run; in-service packets still depart, then statistics print:

```bash
./warmup2 -n 20 -lambda 2 -mu 1 -r 4 -B 10 -P 3
# Ctrl+C
```

## Trace and statistics

Each served packet produces a seven-line lifetime (arrive, enter/leave Q1, enter/leave Q2, begin service, depart). Dropped packets produce a single line. After `emulation ends`:

- average inter-arrival and service time
- average occupancy of Q1, Q2, S1, S2
- mean and standard deviation of time in system
- token and packet drop probability

Times are printed as `00000xxx.xxxms` and reported in seconds with `%.6g`.

## Layout

```
args.c       CLI (UNIX options, any order)
arrival.c    packet source (deterministic or trace)
token.c      bucket refill
server.c     S1 / S2
sigint.c     sigwait shutdown
stats.c      running-sum report
timeutil.c   timeval arithmetic and sleep
warmup2.c    shared queues, move Q1→Q2, thread lifecycle
my402list.c  doubly linked list
```

