# henrysh 🚀

A lightweight Unix shell written from scratch in C, built as a hands-on exploration of how shells actually work under the hood — process creation, I/O redirection via pipes, job control, and signal handling.

> This project lives in the `henrysh/` directory of the `orbit-sh` repository.

## Overview

`henrysh` is a simple interactive command-line shell that mimics core `bash`/`sh` behavior. It reads a line of input, parses it into a command (and arguments), and executes it as a child process using the standard POSIX `fork`/`exec`/`wait` pattern. On top of that baseline it adds:

- **Built-in commands** (`cd`, `exit`) that run in the shell's own process instead of a fork'd child
- **Two-stage pipelines** (`cmd1 | cmd2`) using `pipe()` and `dup2()`
- **Background job execution** (`cmd &`) with PID tracking
- **Zombie process reaping** via a `SIGCHLD` handler, so completed background jobs are cleaned up and reported automatically

It's intentionally compact (a single `main.c`) so the entire control flow — prompt → read → parse → dispatch → execute → wait — is easy to read top to bottom.

## Features

- Interactive REPL prompt (`henrysh> `)
- Execute any external program on `$PATH` via `execvp`
- Built-in `cd <dir>` for changing the working directory
- Built-in `exit` to leave the shell
- Simple two-command pipelines: `cmd1 | cmd2`
- Background execution with `&`, e.g. `sleep 5 &`
- Automatic reaping of finished background jobs with a `[pid] completed` notification
- Graceful exit on `Ctrl+D` (EOF)

## Requirements

- A POSIX-compliant environment (Linux, macOS, WSL, or similar) — the code relies on `unistd.h`, `sys/wait.h`, and `signal.h`
- `gcc` (or any C99-compatible compiler)
- `make`

> Note: this will **not** build natively on Windows (no MSVC/MinGW support out of the box) since it depends on POSIX process and signal APIs. Use WSL on Windows.

## Getting Started

### Clone the repository

```bash
git clone https://github.com/henryagyare/orbit-sh.git
cd orbit-sh/henrysh
```

### Build

```bash
make
```

This compiles `src/main.c` with `gcc -Wall -Wextra -g -std=c99` and produces a `henrysh` binary in the `henrysh/` directory.

### Run

```bash
./henrysh
```

### Clean up build artifacts

```bash
make clean
```

## Usage Examples

Once running, you'll see the `henrysh>` prompt. A few things you can try:

```
henrysh> ls -la
henrysh> pwd
henrysh> cd ..
henrysh> ls | wc -l
henrysh> sleep 5 &
Background job [12345] started
henrysh> (keep working while it runs...)
Background job [12345] completed
henrysh> exit
Leaving... 🚀
```

Pressing `Ctrl+D` at the prompt also exits the shell:

```
henrysh> ^D
Goodbye! 🚀
```

## Project Structure

```
orbit-sh/
├── README.md
└── henrysh/
    ├── Makefile        # Build rules (gcc, -Wall -Wextra -std=c99)
    └── src/
        └── main.c      # Entire shell implementation
```

## How It Works

The shell runs a classic read-eval loop in `main()`:

1. **Prompt & read** — prints `henrysh> ` and reads a line of input (up to `MAX_CMD` = 1024 chars) with `fgets`. EOF (`Ctrl+D`) exits cleanly.
2. **Tokenize** — splits the line on whitespace using `strtok`, filling an `args[]` array (capped at `MAX_ARGS` = 64 tokens).
3. **Pipeline detection** — scans the tokens for a bare `|`. If found, the token list is split into `cmd1` and `cmd2`, a `pipe()` is created, and two children are `fork()`'d: the first has its `stdout` redirected to the pipe's write end via `dup2`, the second has its `stdin` redirected from the pipe's read end. The parent closes both pipe file descriptors and waits on both children.
4. **Background detection** — if the last token is `&`, the command is flagged to run in the background and the `&` token is stripped before execution.
5. **Built-ins** — `cd` and `exit` are handled directly in the parent process (they wouldn't have any effect if run in a forked child, since a child's `chdir`/`exit` doesn't affect the parent shell).
6. **External commands** — everything else is run by `fork()`-ing a child that calls `execvp(args[0], args)`, replacing the child's image with the requested program (resolved against `$PATH`).
7. **Waiting** — foreground commands are waited on synchronously with `waitpid`. Background commands are *not* waited on inline; instead their PID is stored in a `bg_pids[]` array (capped at `MAX_BG` = 64) so they can be tracked.
8. **Reaping** — a `SIGCHLD` handler (`handle_sigchld`) fires whenever any child process changes state. It reaps all finished children with a non-blocking `waitpid(-1, &status, WNOHANG)` loop, and for any PID that matches an entry in `bg_pids[]`, prints a `Background job [pid] completed` message and removes it from the tracking list. This prevents zombie processes from accumulating and gives background jobs asynchronous completion notifications.

## Limitations & Known Issues

This is a learning project, so a number of things a "real" shell supports are intentionally out of scope (for now):

- Only a **single pipe** (two commands) is supported — no multi-stage pipelines like `a | b | c`
- No I/O redirection (`>`, `>>`, `<`)
- No quoting or escaping (`"a b"`, `\ `, etc.) — arguments are split on raw whitespace
- No environment variable expansion (`$HOME`, `$PATH`, etc.) or globbing (`*.txt`)
- No `jobs`, `fg`, or `bg` commands to inspect/manage background jobs after launch
- Fixed-size input/argument buffers (`MAX_CMD`, `MAX_ARGS`, `MAX_BG`) rather than dynamic allocation
- No command history or line editing (no readline/libedit integration)

## Roadmap / Ideas

- [ ] Multi-stage pipelines (`cmd1 | cmd2 | cmd3 | ...`)
- [ ] Output/input/append redirection (`>`, `>>`, `<`)
- [ ] Quoted argument parsing
- [ ] Environment variable expansion
- [ ] `jobs` / `fg` / `bg` built-ins for job control
- [ ] Command history (up-arrow recall)

## Contributing

This is primarily a personal learning project, but issues and pull requests with fixes, cleanups, or small feature additions are welcome.

## License

No license file is currently included in this repository. All rights are reserved by the author unless a license is added.

## Author

Built by [Henry Agyare](https://github.com/henryagyare).
