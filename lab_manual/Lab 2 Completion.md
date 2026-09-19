# SC2005 Lab 2 completion record

This record follows the `lab2` branch and the instructions in [Lab 2 Manual(1).pdf](<Lab 2 Manual(1).pdf>). Line references below point to the final working tree in this checkout. The manual's final verification asks for the stock scheduler in `kernel/proc.c`; the strict even-PID implementation is kept as an applicable patch so the assessment version is preserved without leaving the final working scheduler altered.

## Lab-wide learning objectives

The manual's six outcomes are the reason for the process traces, scheduler experiment, and user-thread work recorded below:

1. Locate process state, saved kernel context, trapframe, and user address space; explain why a user program cannot directly read the kernel's `proc[]` table.
2. Trace how a forked child receives its first kernel context and reaches `forkret()` the first time the scheduler selects it.
3. Explain timer preemption through `yield()`, `sched()`, `scheduler()`, and `swtch()`, while recognizing that these paths are also used when no timer caused the switch.
4. Implement strict priority for runnable even-PID processes, and reason about when runnable odd-PID processes can starve.
5. Explain why a context switch preserves the return address, stack pointer, and callee-saved registers.
6. Complete a cooperative user-thread library and explain how a library switch differs from the kernel switching xv6 processes.

## Individual source edits

This section records every source and support-file edit, including the reason for each individual edit.

- `lab_manual/Lab 2 Completion.md`: added this completion record with a learning objective for each numbered task, source locations for the explanations, individual file edits and their purposes, plus a clear account of runtime evidence that still needs to be collected.

### Build the supplied scheduler workloads

- `Makefile:151-152`: added `$U/_schedtest` and `$U/_starvetest` to `UPROGS`, after `_uthread_test`. This makes both manual workloads user commands in `fs.img`; the generic user-program link rule is at `Makefile:108-110`, and `fs.img` copies the `UPROGS` binaries at `Makefile:155-156`.
- `user/schedtest.c:1-97`: added the finite scheduler workload referenced by Parts 2.1 and 2.5. It forks four children, gates their start through a pipe, sleeps one tick between child creations, prints each child's PID and parity, runs four CPU-work rounds per child, and waits for all children. Each round uses the manual's 4,000,000-iteration workload. The pipe gate creates a comparable point at which the children become runnable; finite work lets both scheduler versions eventually make progress.
- `user/schedtest.c:5-7`: defines `NCHILD`, `ROUNDS`, and `WORK` as compile-time constants. They are not command-line options. The PDF says 4,000,000 iterations; the source packet on the local `upstream/lab2` ref defined 40,000,000, so `WORK` is set to 4,000,000 here to match the manual's stated workload.
- `user/schedtest.c:9-12`: added `parity(pid)` so the output identifies which PID class ran. This is evidence for explaining the policy using actual PIDs rather than inferring priority from child creation order.
- `user/schedtest.c:15-23`: added the volatile arithmetic loop. Volatile work keeps the loop observable to the compiler; while preempted, a child remains runnable unless it blocks on I/O or a pipe.
- `user/schedtest.c:31-41`: creates the gate and forks the children, with a failure message if pipe creation or a fork fails.
- `user/schedtest.c:43-57`: each child closes the gate's write end, blocks until the parent releases it, prints its start/PID/parity, then performs and reports the finite rounds.
- `user/schedtest.c:59-78`: the parent prints each created PID and calls `sleep(1)` between forks, giving the optional FCFS exercise distinguishable creation times. This delay is not a scheduler fairness guarantee.
- `user/schedtest.c:80-96`: closes unused pipe ends, releases all created children together, waits for them, and reports completion. `schedtest` therefore terminates and can show postponement followed by progress; its print sequence is not a proof of fairness.
- `user/starvetest.c:1-70`: added the manual's observation workload. It creates two children with opposite PID parity, waits until both are ready, prints the even and odd PIDs, and releases them through a gate. After release, each child loops forever without sleeping, printing, or exiting. Under strict even-PID priority with `CPUS=1`, this keeps one even child eligible and provides a sustained opportunity to observe the odd child being postponed.
- `user/starvetest.c:11-13` and `:17-40`: create the gate/readiness pipes and fork the two children; each child reports readiness before blocking on the release gate.
- `user/starvetest.c:42-43`: starts the endless volatile CPU loop. There is no voluntary block or exit after release, so timer preemption returns the child to `RUNNABLE`.
- `user/starvetest.c:50-63`: verifies opposite parity, prints the two PIDs and observation instructions, then releases both children. The PID labels let a `Ctrl-p` listing be interpreted without guessing which child is which.
- `user/starvetest.c:65-70`: closes the final writer and waits. The parent normally remains blocked because the children never exit; stop the observation with QEMU's `Ctrl-a x` sequence, not a shell command that may itself be starved.
- The workload files were absent from the checked-out `origin/lab2` tree but present on the local `upstream/lab2` ref at commit `a961811` (`add schedtest and starvetest so students can visually evaluate their implementation`). They are included here because the Lab 2 PDF explicitly requires these supplied workloads and their build entries.

### Preserve strict even-PID scheduling for Part 2

- `lab_manual/even_pid_scheduler.patch:1-46`: stores the strict scheduler change as a patch against the original `kernel/proc.c`. It wraps the scheduler's existing table scan in two priority passes: pass `0` accepts `p->pid & 1 == 0`; pass `1` accepts odd PIDs and runs only when the even pass found no runnable process.
- The patch continues to acquire each `p->lock` before checking state or changing it. It keeps the `RUNNABLE` check, `RUNNING` transition, `c->proc` assignment, `swtch(&c->context, &p->context)`, post-switch cleanup, and release of the process lock. It retains table order within each parity class.
- `kernel/proc.c:445-483` remains the supplied stock scheduler in the final working tree, as the manual requires before Part 3 and final verification. To use the Part 2 scheduler for the experiment, apply `git apply lab_manual/even_pid_scheduler.patch`; restore the stock implementation with `git apply --reverse lab_manual/even_pid_scheduler.patch` after saving the experiment evidence.
- The patch's `found` flag records whether a process was dispatched during a priority pass. If an even process was dispatched, the scheduler does not enter the odd pass in that outer scan. Once the scan completes, the next outer pass starts at `proc[0]`. If no even process was found, the odd pass can dispatch odd processes. A continuously runnable even process can therefore postpone odd processes indefinitely on one CPU.

### Complete the cooperative user-thread library

- `user/uthread.c:14-29`: added `struct thread_context` with `ra`, `sp`, and `s0` through `s11` in that order. Each field is one 64-bit word, so the assembly offsets are `0`, `8`, and `16` through `104` bytes. The order mirrors the saved-register sequence in `kernel/swtch.S:9-34`.
- `user/uthread.c:31-35`: added one saved context to each `struct thread`, alongside its 8192-byte stack and state. A thread needs its own stack because each cooperative thread can suspend at a different call depth while sharing one xv6 process and address space.
- `user/uthread.c:38-39`: changed the switch declaration to accept pointers to the context structures and declared the new-thread bootstrap symbol. This makes the C-to-assembly interface describe the same two context objects that `thread_switch` receives in `a0` and `a1`.
- `user/uthread.c:52-81`: kept the circular runnable-thread search, sets the selected thread to `RUNNING`, changes `current_thread`, and calls `thread_switch` with the outgoing and incoming contexts. When the only runnable thread is selected again, line 80 restores its state to `RUNNING` without switching; this keeps the state field truthful on the no-alternative path.
- `user/uthread.c:84-112`: `thread_create()` now handles a full thread table, creates an initial context, and only then marks the slot `RUNNABLE`. It sets `ra` to `thread_bootstrap`, rounds the top of the thread's own stack down to a 16-byte boundary for `sp`, and stores the function pointer in `s0`. It initializes `s1` through `s11` to zero so a reused slot does not restore stale register values.
- `user/uthread_switch.S:9-39`: saves outgoing `ra`, `sp`, and `s0` through `s11` at the offsets above, loads the incoming values from matching offsets, then returns through the restored `ra`. Caller-saved registers do not need to survive this ordinary function-call boundary; the callee-saved registers must retain their values, while `ra` and `sp` select the continuation and stack of the resumed thread.
- `user/uthread_switch.S:41-48`: added `thread_bootstrap`. A never-run thread returns from `thread_switch` to this bootstrap with its own aligned `sp` and its function pointer in `s0`. The bootstrap moves that pointer to `a0`, calls the thread function, and calls `thread_exit` if the function returns. This gives a returning function a valid return path instead of letting it return to its own entry address.
- The stack alignment follows the RISC-V ABI requirement that `sp` be 16-byte aligned at procedure entry and remain aligned during standard-ABI execution. The register classifications and alignment rule are in the [RISC-V ABIs Specification, §§1.1 and 2.1](https://riscv-non-isa.github.io/riscv-elf-psabi-doc/).
- `user/user.h:48-51` already declares `thread_init`, `thread_create`, `thread_yield`, and `thread_exit`; no prototype edit was needed. `user/uthread_test.c` was already in the branch and is built as `_uthread_test` at `Makefile:150`.

## Task-by-task learning record

Each objective below states what that manual task is trying to teach. These are explanations from the checked source; they are not claims that the manual's runtime observations were captured in this editing session.

### Before you begin — checkpoint 0

**Learning objective:** confirm the selected branch and build baseline before interpreting source or runtime behavior; separate a source-based prediction from output that was actually observed.

The active branch is `lab2`. The manual asks for a clean one-CPU build and an unmodified `uthread_test` run before editing. Source inspection predicts that the starter test returns without thread output because `thread_switch()` is commented out; this is a prediction only. No baseline build or output was captured here.

### Part 1 — Trace processes in xv6

#### 1.1 Locate process state and ownership

**Learning objective:** distinguish a process's metadata/state from its saved kernel registers, saved user registers, and mapped user memory; recognize that the scheduler may select only `RUNNABLE` processes.

- `kernel/proc.h:82-104` defines the process states and `struct proc`. Its `state` and `pid` fields are protected by `p->lock`; the structure separately holds `pagetable`, `trapframe`, and saved kernel `context`.
- `kernel/proc.c:11` defines the kernel-global `proc[NPROC]` table. User code cannot directly dereference that kernel table through its user page table; kernel code such as the scheduler can read it.
- `kernel/proc.c:322` assigns a forked child `RUNNABLE` while holding its process lock. `kernel/proc.c:467` changes a selected runnable process to `RUNNING` while holding the same lock. `kernel/proc.c:518` changes a running process back to `RUNNABLE` in `yield()` before it calls `sched()`.
- Only `RUNNABLE` means a process is eligible for scheduler selection. `SLEEPING` waits for a wakeup, `RUNNING` is already executing on a CPU, `ZOMBIE` has exited but has not been reaped, and `UNUSED` has no live process entry.

#### 1.2 Trace a newly forked command

**Learning objective:** explain how a new process gets a kernel stack and an initial saved continuation, then reaches `forkret()` on its first scheduling.

- The shell forks a child at `user/sh.c:187`; the child later follows the shell's `exec` path at `user/sh.c:79`.
- `kernel/proc.c:110-147` allocates a process and initializes its saved kernel context: `ra` points to `forkret()` and `sp` points to the top of that process's kernel stack.
- `kernel/proc.c:322` makes the child `RUNNABLE`. `kernel/proc.c:445-483` selects it, sets it to `RUNNING`, assigns `c->proc`, and restores the child's context with `swtch()`.
- `kernel/proc.c:523-545` explains and implements the first return through `forkret()`. It releases the process lock inherited from the scheduler and enters the return-to-user path through `usertrapret()`.
- This route distinguishes `struct proc.context` (a kernel continuation) from `struct proc.trapframe` (user register state used at trap entry/return) and `struct proc.pagetable` (the process's user address space).

#### 1.3 Observe the process table through Ctrl-p

**Learning objective:** use the console's kernel debugging snapshot to observe PID, state, and process name, and understand why the snapshot is not a synchronized user-visible process API.

- `kernel/console.c:141-143` handles `Ctrl-p` by calling `procdump()`.
- `kernel/proc.c:670-697` prints non-`UNUSED` entries as PID, state label, and name. The source comment says it deliberately takes no process locks to avoid wedging a stuck machine, so the listing is a diagnostic snapshot that may observe a changing process table.
- Runtime checkpoint not captured here: the manual asks the student to save the idle-shell `Ctrl-p` PID/state/name listing. No listing is invented in this record.

### Part 2 — Modify scheduling and reason about context switching

#### 2.1 Observe the default scheduler

**Learning objective:** establish a same-workload baseline and separate what the stock scheduler visibly does from assumptions about fairness.

- The final `kernel/proc.c:445-483` keeps the branch's stock scheduler: it scans process-table slots in order, switches to each `RUNNABLE` entry it encounters, and continues after `swtch()` returns.
- `user/schedtest.c:5-7` fixes the child count, rounds, and loop work. Its children block on a shared release gate, then perform finite CPU rounds. Their eventual exit permits later progress, so output order alone does not demonstrate an indefinite fairness guarantee.
- The manual asks for `make CPUS=1 qemu` followed by `schedtest`, with the full output and actual PIDs saved as a baseline. That output was not captured in this session.

#### 2.2 Trace preemption and the scheduler loop

**Learning objective:** connect timer delivery to a process becoming runnable and show which saved context each `swtch()` call stores and restores.

- In the user-originated trap path, `kernel/trap.c:37-81` recognizes device interrupts and calls `yield()` when `devintr()` reports a timer (`which_dev == 2`). The kernel-originated path also yields on a timer at `kernel/trap.c:147-155` when a process is running.
- `kernel/proc.c:514-520` acquires the process lock, changes the process to `RUNNABLE`, and calls `sched()`.
- `kernel/proc.c:493-510` checks that only `p->lock` is held, the process is no longer `RUNNING`, and interrupts are disabled; `swtch(&p->context, &mycpu()->context)` saves the process's kernel continuation and restores that CPU's scheduler continuation.
- `kernel/proc.c:445-483` performs the other switch direction with `swtch(&c->context, &p->context)`: it saves the per-CPU scheduler continuation and restores the chosen process's kernel continuation. When the process later yields, the first call resumes after `swtch()` in the scheduler.
- The timer is one reason to yield. Sleep, exit, and other scheduling paths use the same scheduler/context-switch machinery without a timer-triggered preemption.

#### 2.3 Explain the callee-saved register rule

**Learning objective:** use the function-calling convention to explain why a call-boundary context switch saves callee-saved registers plus `ra` and `sp`, rather than every general register.

- `kernel/proc.h:2-20` and `kernel/swtch.S:9-34` agree on the kernel context layout: return address, stack pointer, then `s0-s11`.
- The ABI identifies `s0-s11` as callee-saved; argument and temporary registers are caller-saved. A caller must preserve any caller-saved value it needs after making a call, so `thread_switch` and kernel `swtch` need not preserve those registers for the caller.
- `ra` is not itself callee-saved under the ABI. The switch routine explicitly saves it because its eventual `ret` must resume the suspended call site. `sp` is explicitly saved because each suspended process/thread needs to resume on its own stack. The ABI's general preservation rule for `sp` does not remove the need to load a different stack when switching between independent continuations.

#### 2.4 Define strict even-PID priority

**Learning objective:** express strict priority as a selection rule over currently eligible processes, preserve the kernel's lock/state/context-switch invariants, and predict starvation under an always-runnable higher-priority workload.

- The complete saved implementation is `lab_manual/even_pid_scheduler.patch:1-46`; `kernel/proc.c:445-483` stays stock in the final worktree as required by the manual.
- At each full scheduler scan, priority pass `0` accepts even-PID runnable entries. If this pass dispatched any process, the scan does not enter the odd pass. If it dispatched none, priority pass `1` accepts odd-PID runnable entries. The existing process-table order is retained within each parity class.
- The patch keeps the `p->lock` around `state` inspection and the `RUNNABLE` to `RUNNING` transition. It preserves `c->proc`, `swtch()`, scheduler cleanup after the process returns, and timer preemption. It does not turn a `RUNNING` even process on another CPU into a candidate for this CPU.
- The manual's policy is intentionally not fair across parity classes: if a runnable even-PID process never blocks or exits, the scheduler can repeatedly choose it and keep odd-PID processes runnable without dispatching them.

#### 2.5 Run the workload and analyze starvation

**Learning objective:** compare real baseline/modified runs and use a deliberately endless workload plus `Ctrl-p` observations to distinguish finite postponement from a policy that can starve a process indefinitely.

- `schedtest` is finite; the manual asks to compare actual PIDs and runnable intervals under stock and strict scheduling. No run output or exact execution order is claimed here.
- `starvetest` prints the even and odd child PIDs at `user/starvetest.c:58-60`, then releases both into loops at `:42-43`. With `CPUS=1` and strict priority, the even child remains runnable after timer preemption; the odd child can remain listed as `runble` in repeated `Ctrl-p` snapshots. A snapshot may catch it as `run` if the console interrupt arrives while it is executing.
- Repeated snapshots show postponement during the observation window. The policy and workload explain how postponement can continue indefinitely while the even child remains runnable; finite observation alone cannot prove an infinite runtime event.
- Required live evidence not captured here: the baseline and strict `schedtest` outputs, the `starvetest` PID line, and two `Ctrl-p` process listings. The manual says to stop `starvetest` by exiting QEMU with `Ctrl-a x`, because the shell may itself be starved.
- Before Part 3, restore the stock scheduler with `git apply --reverse lab_manual/even_pid_scheduler.patch`; this checkout's final source is already in that stock state.

#### Optional FCFS extension

**Learning objective:** reason about a different scheduling policy by defining when a process becomes eligible, how creation time and ties are handled, how that metadata is protected, and whether preemption remains enabled.

The manual marks FCFS optional and not evaluated. No FCFS implementation was added; the saved strict-priority patch is the scheduler exercise required by the manual.

### Part 3 — Complete the user-level threading library

#### 3.1 Inspect the unfinished library

**Learning objective:** identify which responsibility belongs to the user library (thread states, stacks, selection, and saved context) and which belongs to the assembly switch, then use the test program to infer required behavior.

- The branch provided `user/uthread.c`, `user/uthread_switch.S`, and `user/uthread_test.c` with context/setup/switch TODOs. The implementation edits recorded above fill those TODOs; `user/uthread_test.c:8-73` already defines three yielding test threads and the main routine that starts them.
- The user library switches between threads inside one xv6 process. The kernel still schedules that process as one process, so cooperative user threads do not get separate kernel process-table entries or trapframes.

#### 3.2 Design the saved context and stack

**Learning objective:** make the C representation agree byte-for-byte with assembly offsets and meet the standard ABI's stack-alignment rule at a function entry.

- The fields at `user/uthread.c:14-29` use 64-bit words in the exact order used by `user/uthread_switch.S:9-39`: `ra` at 0, `sp` at 8, `s0` at 16, ..., `s11` at 104.
- `user/uthread.c:98` sets each new thread's stack pointer to the top of its own 8192-byte stack rounded down to a 16-byte boundary. Rounding down is necessary because the start address of each embedded stack is not guaranteed by the C structure's alignment alone to be 16-byte aligned.

#### 3.3 Plan the first execution of a new thread

**Learning objective:** distinguish resuming a suspended call from starting a thread that has never called the switch routine, and provide a valid return path if its function finishes normally.

- A suspended thread's saved `ra` resumes after its previous call to `thread_switch`. A new thread's initial `ra` points to `thread_bootstrap`, not directly to the user function; its initial `sp` is its aligned private stack top, and `s0` carries the function pointer.
- `thread_bootstrap` starts the function from that context. If the function returns, the bootstrap invokes `thread_exit()` rather than returning to an invalid synthetic caller.

#### 3.4 Implement one verified component at a time

**Learning objective:** treat C context layout, initial context construction, switch arguments, and assembly loads/stores as one contract whose pieces must agree.

The implementation follows the manual's dependency order: context structure in `user/uthread.c:14-35`; new context and aligned stack in `:84-112`; outgoing/incoming context pointers at `:75-79`; matching save/restore offsets in `user/uthread_switch.S:9-39`; then first-run bootstrap at `:41-48`. Source inspection cannot replace a RISC-V build and runtime check because mismatched offsets can compile and still fail when switching.

#### 3.5 Verify the completed behavior

**Learning objective:** check observable thread creation, round-robin cooperation, exact iteration counts, exit transitions, and repeatability instead of treating a successful build as proof of correct switching.

- The source-defined checks are in `user/uthread_test.c:8-60`: all three threads must announce startup, each must print values 0 through 99 exactly once, and each must report its count at exit. `thread_schedule()` must eventually report that no runnable user threads remain at `user/uthread.c:69-72`.
- Startup waits at `user/uthread_test.c:12-13`, `:31-32`, and `:50-51` make every thread wait until the others have started. The circular scan begins after the current slot, so start/count order is a consequence of runnable states and scan position, not a promise that output follows `thread_create()` order.
- The manual asks for one completed run, comparison with the unmodified baseline, and a repeat run during final verification. Those runtime outputs were not captured here, so no transcript is fabricated.

### Part 4 — Optional GDB exercise

**Learning objective:** inspect actual saved context addresses and register values at a user-thread switch, and use evidence to distinguish a bad initial context, a C/assembly layout mismatch, wrong switch arguments, and incorrect thread states.

Part 4 is optional and not evaluated. No GDB session was started for this edit. The relevant symbols are `thread_schedule()` in `user/uthread.c` and `thread_switch` in `user/uthread_switch.S`.

## Final verification status

**Learning objective:** restore the assessed stock scheduler while retaining the strict-priority implementation and evidence, then use a clean multi-CPU build and repeat run to check the completed user-thread behavior.

The manual's final source state is present: strict-priority code is saved in `lab_manual/even_pid_scheduler.patch`, `kernel/proc.c` contains the original scheduler, the workload commands are included in `UPROGS`, and the user-thread TODOs are implemented.

No build or QEMU run was performed in this Windows checkout. `make`, `qemu-system-riscv64`, `riscv64-unknown-elf-gcc`, and `perl` are not available on `PATH`; the manual's runtime evidence still needs to be collected in the course Linux/VM environment with its RISC-V toolchain and QEMU. The commands requested by the manual are:

~~~sh
make clean
make CPUS=1 qemu
~~~

Inside xv6, save a stock-scheduler `schedtest` run; apply the patch in a separate run and save another `schedtest` run; run `starvetest`, record its PID line and two `Ctrl-p` listings, and exit QEMU with `Ctrl-a x`. Then reverse the patch, run `make clean`, `make CPUS=3 qemu`, and capture two `uthread_test` runs for the completion and repeatability checks.

## Final checkpoint: what the six questions are intended to test

1. `procdump()` runs in kernel mode and can access kernel-global `proc[]`; a normal user address space cannot directly dereference that kernel table.
2. A timer interrupt returns through trap handling, calls `yield()`, changes the process to `RUNNABLE`, and switches to the per-CPU scheduler context. The scheduler later restores that process context.
3. Under strict even-PID priority, an odd-PID process can starve while an even-PID process remains runnable on the same CPU.
4. Callee-saved registers preserve values across the switch call; `ra` identifies the continuation and `sp` selects the suspended stack.
5. A never-run thread restores `ra` to `thread_bootstrap`, restores an aligned private `sp`, and carries its function pointer in `s0` for the bootstrap to call.
6. The kernel schedules an xv6 process and uses process/kernel contexts and trapframes; the library schedules cooperative user threads within that one process and saves their user-level call contexts.
