# Mini RTOS Scheduler

A C++17 simulation of an RTOS scheduling core: task management, three
scheduling policies (Round Robin, Priority, Rate Monotonic), and
synchronization primitives (Mutex with priority inheritance, Semaphore).
Built as a portfolio project to demonstrate embedded/OS fundamentals —
context switching, preemption, priority inversion, and how to fix it.

## Building and running

```bash
cmake -S . -B build
cmake --build build
./build/mini_rtos_demo
```

This runs five demos in sequence: Round Robin, preemptive Priority
scheduling, Rate Monotonic, and a Mutex priority-inversion demo run twice
(once with priority inheritance disabled, once enabled) so you can see the
difference directly.

## Architecture

- **`Task`** — id, priority, state machine (Ready/Running/Blocked/Terminated),
  burst time, optional period, and an `entry_` callable representing the
  task's work.
- **`Scheduler`** (abstract) — owns the ready/blocked queues and
  `currentTask_`; concrete policies implement `addTask()`, `pickNextTask()`,
  and `tick()`.
- **`RoundRobinScheduler`**, **`PriorityScheduler`**, **`RateMonotonicScheduler`**
  — the three policies, each with its own `ContextSwitchLog` recording every
  switch with a tick number, from/to task, and reason.
- **`Mutex`** — priority-inheritance-capable lock: if a higher-priority task
  blocks on a mutex held by a lower-priority task, the holder's priority is
  temporarily boosted so a medium-priority task can't starve it.
- **`Semaphore`** — counting semaphore with a real wait list.
- **`Timer`** — software-driven tick source (`tick()` must be called
  externally each simulated millisecond; there's no real hardware timer here).

## Priority convention

**Higher numeric value = higher priority** (matches FreeRTOS; the opposite
of POSIX `nice`). This is documented in `common.h`.

## Design decisions and known limitations

These are deliberate simplifications, not oversights — each is here so you
can speak to the tradeoff directly in an interview rather than get caught
by a question you haven't thought through.

### The "checkpoint" execution model — not true preemption

Tasks are plain C++ callables (`std::function<void(Task&, YieldCallback)>`).
There is no `ucontext.h` stack-switching or C++20 coroutine machinery here,
so a task's `entry_` **cannot be paused mid-function and resumed later at
the same point** — it either runs to completion in one call, or calls the
provided `yield()` callback to voluntarily end its turn early.

This means preemption in this project happens **between dispatches**, not
**inside** a task's code: `entry_` is invoked exactly once per dispatch (the
scheduler tracks this with a `needsExecute_` flag), and after that single
call, `remainingBurstTime()` — a separate tick counter, decremented purely
by the scheduler — governs how many *additional* ticks the task is
considered to keep holding the CPU before it's marked complete or is
preempted. `entry_`'s single call represents "the visible work done at
dispatch time"; the burst countdown that follows represents "still
simulated-running, no further code execution."

A real RTOS achieves true mid-function preemption via hardware interrupts
that save CPU registers and the stack pointer, or in software via
`ucontext.h`/fiber libraries. Implementing that was scoped out to keep the
project's core scheduling logic front and center, but it's worth being able
to explain the gap precisely — see the debugging notes below for how
subtle the consequences of this constraint turned out to be.

### Blocking primitives use a poll-and-yield model

Because `entry_` can't literally suspend itself waiting on a mutex, tasks
that fail to acquire a `Mutex` call `scheduler.enqueueBlocked()` themselves
and return; when the mutex is released, `Mutex::unlock()` fires a
`wakeCallback_` that the demo wires to `scheduler.enqueueReady()`. This
means a task's `entry_` re-runs from the top on each redispatch, so lock
attempts are written to check `mutex->owner() == self` first before trying
to lock again (see `runMutexDemo` in `main.cpp`).

### No automatic resource cleanup hooks

The scheduler core has no generic "task completed" event that external code
can subscribe to. The mutex demo detects `Low`'s completion by checking
`task->isTerminated()` in the demo loop after each `tick()` call, rather
than via a callback fired by the scheduler itself. A production RTOS
simulation would likely add a completion-hook mechanism; it was left out
here to keep `Scheduler`'s interface minimal.

### `RateMonotonicScheduler` assumes 1 tick = 1 millisecond

Task periods are `std::chrono::milliseconds`, and the scheduler compares
`period().count()` directly against its own tick counter. This is fine for
a simulation but would need proper unit conversion against a real timer
tick rate in a non-simulated system.

## The debugging journey (worth knowing for interviews)

This project was built with AI assistance (Copilot, then Claude), and the
most instructive part wasn't the initial code generation — it was that the
early Round Robin scheduler *looked* completely correct (clean context-switch
log, tick counts and burst totals that added up perfectly) while silently
never running any actual task code at all. Four real bugs were found by
demanding actual console output and tracing it by hand, not by reading code:

1. **`entry_` never executed, ever.** `Task::execute()` requires
   `canTransitionTo(Running)` to succeed before running `entry_`, but the
   scheduler was setting state to `Running` *before* calling `execute()` —
   and the state machine didn't allow a Running→Running self-transition.
   Every dispatch silently no-op'd. The scheduling *metadata* (tick counts,
   burst decrements) is pure arithmetic independent of whether real task
   code ran, which is exactly why this went unnoticed: the log was
   internally consistent but represented no actual computation.
2. **Null pointer dereference once `entry_` started running.** The
   `yield()` callback resets `currentTask_` synchronously, *during* the
   `execute()` call — so code that captured `currentTask_` *after* calling
   `execute()` was already holding a null pointer.
3. **Quantum counter not reset on yield-triggered switches**, so leftover
   quantum debt from one task carried over and wrongly truncated the next
   task's turn.
4. **`entry_` re-invoked on every tick of a quantum window** instead of
   once per dispatch, which would have silently re-run task work multiple
   times per turn — fixed with the persistent `needsExecute_` flag.

The lesson generalizes well beyond this project: a scheduler's log/metrics
looking numerically self-consistent is not evidence that the underlying
work actually happened — verify by making the system produce real,
traceable side effects (console output, in this case) and checking those
by hand, not just the summary numbers.

## Verified behavior

All three schedulers and the mutex demo were manually traced against their
actual console output (not just eyeballed) to confirm:

- Round Robin: each task's total ticks-run across all its dispatches exactly
  equals its assigned burst time.
- Priority: a higher-priority task preempts with zero added latency, and a
  preempted task resumes with its *exact* remaining burst (no work lost or
  duplicated).
- Rate Monotonic: the shorter-period task always wins when both are ready
  simultaneously, and periodic releases fire exactly on period boundaries.
- Mutex: priority inheritance measurably shortens the high-priority task's
  wait time compared to the same scenario without inheritance.
