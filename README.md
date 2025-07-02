# C-Shell - A Custom Shell in C

## Overview

**Newshell** is a lightweight Unix-style shell written in C that supports:

- Interactive and batch execution modes
- Command history tracking and execution
- I/O redirection (`<`, `>`)
- Pipelining (`|`)
- Built-in command support (`cd`, `path`, `exit`)
- Environment path manipulation
- Signal handling for `Ctrl+C` and `Ctrl+Z`

---

## Features

### Interactive Mode

Launches a prompt:

```
newshell>
```

### Batch Mode

Executes commands from a file:

```bash
./main.out commands.txt
```

### History Commands

- `myhistory` — List the last 20 commands
- `myhistory-c` — Clear command history
- `myhistory-eN` — Execute the N-th command from history (e.g., `myhistory-e3`)

### Redirection

- Input: `command < input.txt`
- Output: `command > output.txt`

### Piping

Chain commands:

```bash
ls -l | grep ".c" | wc -l
```

### Path Management

- `path + /new/dir` — Add directory to `PATH`
- `path - /remove/dir` — Remove directory from `PATH`

---

## Build Instructions

Use the following `Makefile` to compile:

\`\`\`makefile
main.out: main.c
gcc main.c -o main.out
\`\`\`

Then run:

```bash
make
```

Or manually:

```bash
gcc main.c -o main.out
```

---

## Usage

- To start the shell interactively:

  ```bash
  ./main.out
  ```

- To run in batch mode with a file:
  ```bash
  ./main.out commands.txt
  ```

---

## Notes

- Input is capped at 1024 characters per line
- Command history stores up to 20 entries
- Shell supports basic signal handling and piping but not background jobs
- `PATH` changes affect only the current shell session
