# BSDSF24M033-OS-A03: myshell

A Unix command-line shell written in C for the Operating Systems course (Programming Assignment 03).

## Features

| Release | Feature |
|---|---|
| v1.0-base | Base shell: fork-exec-wait cycle |
| v2 | Built-ins: `cd`, `exit`, `help`, `jobs` |
| v3 | Command history: `history`, `!n` |
| v4 | GNU Readline: tab completion, arrow-key history |
| v5 | I/O redirection (`<`, `>`) and pipes (`\|`) |
| v6 | Command chaining (`;`), background jobs (`&`), zombie reaping |
| v7 | `if / then / else / fi` |
| v8 | Shell variables: `NAME=value`, `$NAME`, `set` |

## Build and run

Requires `gcc`, `make` and `libreadline-dev`.

    make
    ./bin/myshell

## Project layout

    src/        main.c, shell.c, execute.c, history.c, completion.c,
                pipeline.c, jobs.c, variables.c
    include/    shell.h
    Makefile

## Example session

    FCIT> ls | sort -r | head -3
    FCIT> sleep 10 &
    FCIT> jobs
    FCIT> MSG="Hello"
    FCIT> echo $MSG
    FCIT> if grep root /etc/passwd > /dev/null
    > then
    > echo found
    > fi

## Author

Roll no: BSDSF24M033
