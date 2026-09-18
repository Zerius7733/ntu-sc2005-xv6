# SC2005 Lab 1 completion record

This document records the Lab 1 work completed against the `lab1` branch in this repository. It is written for the source tree in this checkout; paths and line references refer to the edited working tree and should be rechecked after later edits.

The source instructions are in [Lab 1 Manual (2).pdf](Lab%201%20Manual%20(2).pdf).

## Learning objectives

After completing the manual and this implementation, a student should be able to:

1. Navigate a host terminal, run a foreground command, interrupt it with `Ctrl-C`, and distinguish the host shell from the xv6 shell.
2. Build and boot the course version of xv6, use the xv6 shell, and leave QEMU with `Ctrl-a x`.
3. Explain how a C file under `user/` becomes an xv6 command only after it is compiled, linked, and included in the `UPROGS` list used to build `fs.img`.
4. Use `fork()`, process IDs, process states, `exit()`, `wait()`, and `procdump()` to explain a zombie process and the later reaping of its process-table entry.
5. Trace an existing system call from a user declaration, through the generated assembly stub and `ecall`, trap handling, numbered dispatch table, and kernel handler, then apply the same path to `getproccount()`.
6. Explain pipe read/write endpoints, post-`fork()` descriptor references, EOF, failed writes with no readers, and the endpoint ownership needed for a two-pipe request/reply exchange.

## Full change record

The following are the individual source changes made for the manual. This is a complete change record, not a condensed summary.

### Build and file-system image

- `Makefile:128-146`: added `$U/_pingpong`, `$U/_proccount`, and `$U/_two_pipes` to `UPROGS`.
- This causes the generic `_%` user-program rule at `Makefile:102-105` to link each program and causes `fs.img` at `Makefile:149-150` to copy the resulting binaries into the xv6 file system.
- `user/usys.S` is intentionally not hand-edited. It is generated from `user/usys.pl` by the `Makefile:107-111` rule and is ignored by Git.

### `getproccount()` system call

- `user/user.h:22`: declared the user-visible function `int getproccount(void);`.
- `user/usys.pl:36`: added `entry("getproccount")`, which generates a user stub that loads `SYS_getproccount` into register `a7`, executes `ecall`, and returns the kernel result in `a0`.
- `kernel/syscall.h:23`: assigned system-call number `22` to `SYS_getproccount`.
- `kernel/syscall.c:104`: declared the kernel dispatch handler `sys_getproccount`.
- `kernel/syscall.c:130`: mapped number `22` to `sys_getproccount` in the `syscalls[]` dispatch table.
- `kernel/sysproc.c:25-28`: added the system-call handler. It delegates the process-table counting work to `countproc()`.
- `kernel/defs.h:104`: declared the internal `countproc()` helper.
- `kernel/proc.c:440-454`: added `countproc()`. It scans all `NPROC` entries, acquires each entry's process lock before inspecting `state`, counts every state other than `UNUSED`, releases the lock, and returns the count.
- `user/proccount.c:1-15`: added the user command. It rejects extra command-line arguments, invokes `getproccount()`, prints the returned integer, and exits with status `0` on success or `1` for invalid usage.

The count deliberately includes `USED`, `SLEEPING`, `RUNNABLE`, `RUNNING`, and `ZOMBIE`, because the contract counts every process-table entry whose state is not `UNUSED`. Each entry is locked only while its state is inspected, so the result is a sequence of per-entry observations rather than a frozen global snapshot.

### `pingpong`

- `user/pingpong.c:8-55`: kept the supplied five-message pipe experiment but added error checks for `pipe()`, `fork()`, `write()`, and `read()`.
- `user/pingpong.c:50`: writes a terminating byte after each four-byte read so `%s` receives a valid C string.
- `user/pingpong.c:37-38` and `user/pingpong.c:54-55`: closes descriptors when each process is finished with them.
- `Makefile:138`: includes `_pingpong` in the xv6 image.

### Zombie observation

- `user/zombie.c:12`: changed the parent delay from `sleep(5)` to `sleep(100)` and documented that the longer xv6 tick delay is for the `Ctrl-p` observation window. The delay is in clock ticks, not seconds.

### `two_pipes`

- `user/two_pipes.c:9-17`: checks both `pipe()` calls and closes the first pipe if creation of the second pipe fails.
- `user/two_pipes.c:19-27`: checks `fork()` and closes all four descriptors on the fork-failure path.
- `user/two_pipes.c:29-55`: implements the child. The child closes the unused write endpoint of the first pipe and unused read endpoint of the second pipe, reads exactly four bytes of `ping`, adds a terminating byte, prints the received message, writes four bytes of `pong`, closes both remaining descriptors, and exits with status `0`.
- `user/two_pipes.c:57-86`: implements the parent. The parent closes the unused read endpoint of the first pipe and unused write endpoint of the second pipe, writes four bytes of `ping`, closes its first-pipe writer, reads and safely terminates four bytes of `pong`, closes its second-pipe reader, waits for the child, and exits with a status reflecting write/read/wait failures.
- `Makefile:143`: includes `_two_pipes` in the xv6 image.

### Existing worktree change kept outside this completion

Before this work began, the worktree already reported `D lab_manual/Lab 2 Manual(1).pdf`. I did not restore, edit, or otherwise use that deletion. It remains separate from the Lab 1 changes recorded above.

## How to build and run the completed lab

The commands below use the labels from the PDF. Do not type `host$` or `xv6$`; they identify which shell should receive the command.

### Host shell preparation

~~~sh
pwd
ls Makefile
make clean
make qemu
~~~

`make qemu` runs on the host. It compiles the kernel and user programs, generates `user/usys.S`, builds `fs.img` from `UPROGS`, and starts QEMU. Exit QEMU with `Ctrl-a x`, not `Ctrl-C`.

For the terminal warm-up, use:

~~~sh
pwd
ls
cd <repository-directory>
cat README.md
sleep 30
~~~

Press `Ctrl-C` while `sleep 30` is in the foreground. `Ctrl-C` interrupts the host command; it is not typed as command text.

### xv6 shell checks

After `make qemu`, run:

~~~text
ls
cat README
mkdir lab1tmp
cd lab1tmp
cd ..
forktest
usertests
pingpong
proccount
two_pipes
~~~

Expected observations:

- `forktest` ends with `fork test OK`.
- `usertests` completes without a failed test.
- `pingpong` prints five four-byte messages received by its parent. The exact PID is variable.
- `proccount` prints one non-negative integer. It normally includes the running `proccount` process itself.
- `two_pipes` prints the child line containing `received ping` before the parent line containing `received pong`; both PIDs are variable.

## Verified source traces

### Existing `getpid()` path

The branch-specific trace is:

| Order | Phase | Location | Symbol or generated item | Role |
|---:|---|---|---|---|
| 1 | Build/source | `user/user.h:21` | `int getpid(void);` | User-visible declaration. |
| 2 | Build/source | `user/usys.pl:35` | `entry("getpid")` | Requests generation of the user stub. |
| 3 | Build | `Makefile:107-108` | `user/usys.S` rule | Runs `perl user/usys.pl` and writes the generated assembly. |
| 4 | Build | `Makefile:110-111` | `user/usys.o` rule | Compiles the generated assembly into the user library. |
| 5 | Build | `Makefile:100-105` | `ULIB` and `_%` rule | Links `usys.o` into a user command. |
| 6 | Generated/runtime | generated `user/usys.S` | `getpid:`, `li a7, SYS_getpid`, `ecall`, `ret` | Places the syscall number in `a7` and enters the kernel. This file is generated and not line-cited as source. |
| 7 | Build/runtime number | `kernel/syscall.h:12` | `SYS_getpid 11` | Defines the number used by the generated stub and kernel table. |
| 8 | Runtime | `kernel/trap.c:53-67` | `usertrap()` | Recognizes a user system-call trap (`r_scause() == 8`), advances `epc` past `ecall`, and calls `syscall()`. |
| 9 | Runtime | `kernel/syscall.c:134-150` | `syscall()` | Reads `p->trapframe->a7`, indexes `syscalls[]`, calls the handler, and writes the return value to `p->trapframe->a0`. |
| 10 | Runtime | `kernel/syscall.c:119` | `[SYS_getpid] sys_getpid` | Selects the numbered handler. |
| 11 | Runtime | `kernel/sysproc.c:19-22` | `sys_getpid()` | Reads `myproc()->pid`; the returned value becomes the user-visible result in `a0`. |

The file to edit for the generated stub is `user/usys.pl`, not the generated `user/usys.S` file.

### New `getproccount()` path

The new call follows the same path:

1. `user/user.h:22` exposes `getproccount()` to user C code.
2. `user/usys.pl:36` generates the `getproccount` assembly stub.
3. The Makefile regenerates and compiles `user/usys.S` through `Makefile:107-111`.
4. `kernel/syscall.h:23` assigns number `22`.
5. `kernel/trap.c:53-67` handles the generic user `ecall`; no per-call edit is needed there.
6. `kernel/syscall.c:104` declares the handler and `kernel/syscall.c:130` registers number `22` in `syscalls[]`.
7. `kernel/syscall.c:134-150` dispatches the call and stores the result in `a0`.
8. `kernel/sysproc.c:25-28` runs `sys_getproccount()`.
9. `kernel/defs.h:104` exposes the internal helper declaration, and `kernel/proc.c:440-454` performs the per-entry locked state count.
10. `user/proccount.c:12` prints the returned value.

The number must agree in `SYS_getproccount`, the generated `li a7, ...` stub, and the dispatcher index. The generic trap path is shared by all system calls, so it does not need a new branch for this call.

## Process and zombie observations

`fork()` returns the child PID to the parent and `0` to the child. In `kernel/proc.c:302-303`, xv6 explicitly sets the copied child's `a0` register to zero; `fork()` returns the child's PID in the parent at `kernel/proc.c:313-325`.

The observation sequence is:

1. The child created by `user/zombie.c` calls `exit(0)`.
2. `kernel/proc.c:378-384` records the status and changes the child's state to `ZOMBIE`, then schedules away without freeing the process-table slot.
3. While the parent is in `sleep(100)`, run `Ctrl-p` in the QEMU console and look for a `zombie` row printed by `kernel/proc.c:691-700`.
4. The parent later calls `exit(0)`. Its parent is the shell, so the shell's `wait()` eventually finds and reaps it.
5. `kernel/proc.c:408-420` finds a zombie child and calls `freeproc(pp)`. The `freeproc()` implementation at `kernel/proc.c:152-172` returns the slot to `UNUSED`.

A zombie has stopped executing, but its process-table entry has not yet been returned to `UNUSED`. `getproccount()` can differ from a `Ctrl-p` listing because the two observations happen at different times, `Ctrl-p` skips `UNUSED` entries, and `getproccount()` includes the command process while it is running. The counting helper also observes entries one at a time while processes can change state between inspections.

## Two-pipe descriptor plan

Each `pipe(int p[2])` creates a read endpoint at index `0` and a write endpoint at index `1`.

| Object | Endpoint | Parent keeps | Child keeps | Closed by parent | Closed by child |
|---|---|---:|---:|---:|---:|
| `p1` | `p1[0]` read | No | Yes | Yes | No, until after reading |
| `p1` | `p1[1]` write | Yes | No | No, until after writing | Yes |
| `p2` | `p2[0]` read | Yes | No | No, until after reading | Yes |
| `p2` | `p2[1]` write | No | Yes | Yes | No, until after replying |

The exchange is:

~~~text
parent --write "ping"--> p1[1] -> p1 -> p1[0] --read--> child
parent <--read "pong"-- p2[0] <- p2 <- p2[1] <--write-- child
~~~

The parent and child inherit references to all four descriptors after `fork()`; the explicit closes in `user/two_pipes.c:32-34` and `user/two_pipes.c:59-61` remove the unused references. `kernel/proc.c:305-308` calls `filedup()` during fork, and `kernel/file.c:48-55` increments the underlying open-file reference count. A descriptor close calls `fileclose()` at `kernel/file.c:60-78`; the pipe endpoint is marked closed only when the last reference to that open-file object is gone. Leaving an unused writer open can keep a reader blocked instead of allowing EOF. `kernel/pipe.c:77-103` makes a write fail when `readopen` is zero, while `kernel/pipe.c:106-130` returns EOF after the pipe is empty and `writeopen` is zero.

## Verification checklist

Run the following after installing the course prerequisites on a Linux host or inside a working xv6 toolchain environment:

~~~sh
make clean
make qemu
~~~

Inside xv6:

~~~text
forktest
usertests
pingpong
proccount
two_pipes
zombie
~~~

While `zombie` is still sleeping, press `Ctrl-p`. Confirm a `zombie` row, then wait for the shell prompt and press `Ctrl-p` again. Confirm that the child row is gone after `wait()` reaps it.

The expected two-pipe ordering is structurally guaranteed by the program:

~~~text
<child-pid>: received ping
<parent-pid>: received pong
~~~

The PID values and process-count value are runtime-dependent and must not be hard-coded into a test or report.

## Verification performed in this checkout

Static source verification was completed by checking the changed files, syscall number registration, `UPROGS` entries, endpoint closures, string termination, and the diff for whitespace errors.

Runtime compilation and QEMU execution could not be performed in the current Windows environment because `make`, the RISC-V cross-compiler, QEMU, and Perl are not available on `PATH`; the WSL distribution query is also unavailable due to an access-denied service response. The commands in the previous section are the pending runtime verification steps.

## Reference answers for the final checkpoint

1. A file under `user/` alone is only source code. The Makefile must compile and link it, and `UPROGS` must pass the resulting binary to `mkfs` so it appears in `fs.img`.
2. The generated stub puts the system-call number in `a7`; `ecall` enters the kernel. The kernel returns the result through `a0`.
3. `getproccount()` counts non-`UNUSED` entries during its call, including the running command and any zombie or sleeping entries then present. `Ctrl-p` is a separate observation and displays names/states at another time.
4. After `fork()`, parent and child have separate descriptor-table entries that refer to shared open-file objects. Closing one descriptor removes one reference; it does not necessarily close the underlying pipe endpoint.
5. A read from an empty pipe returns `0` when all write references are closed (EOF). A write with no read references returns `-1`.



