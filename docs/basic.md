# Programming HomeCore in BASIC

HomeCore has a BASIC interpreter built in, like the home computers of the
1980s: type `basic` at the shell prompt and start writing programs. This guide
covers the language as HomeCore runs it, with examples you can type in. Each
example's output is checked against the real firmware in CI
(`tests/qemu_docs_test.py`).

The interpreter is [G2Basic](../external/g2basic/README.md); its
[language reference](../external/g2basic/docs/language.md) is the complete
description.

## Starting and leaving

```text
root:/$ basic
G2BASIC Interpreter with line numbers. Ctrl-C/Ctrl-D/Ctrl-Z to exit.

>
```

The `>` prompt accepts two kinds of lines. A line that starts with a number is
stored as part of the program; anything else runs at once. **Ctrl-C** (or
Ctrl-D, or Ctrl-Z) returns to the shell. Leaving BASIC or rebooting discards
the program and its variables; there is no way to save programs yet.

## Calculating

`PRINT` evaluates expressions and prints the results, separated by spaces:

```basic
PRINT 6 * 7
PRINT (2 + 3) * 4, 10 / 4
PRINT 1 / 3
PRINT max(3, 7, 5), min(2, 8)
```

```text
42
20 2.5
0.333333333
7 2
```

All values are numbers; HomeCore prints up to nine significant digits. There
are `+`, `-`, `*`, `/`, and parentheses, but no power operator, no remainder,
and no strings.

## Your first program

Number the lines, then type `RUN`:

```basic
10 FOR I = 5 TO 1 STEP -1
20 PRINT I
30 NEXT I
40 PRINT 0
RUN
```

```text
5
4
3
2
1
0
```

`FOR` counts from the first value to the second, adding `STEP` (1 when
omitted) each time `NEXT` is reached. `FOR` and `NEXT` must be on their own
lines.

## Editing a program

Typing a line number that already exists replaces that line; typing a number
alone deletes it. `LIST` shows the program and `NEW` erases it:

```basic
10 PRINT 1
20 PRINT 2
30 PRINT 3
10 PRINT 100
30
LIST
RUN
```

```text
10 PRINT 100
20 PRINT 2
100
2
```

Numbering lines in steps of 10 leaves room to insert lines later.

## Variables

A variable is created by assigning to it; there is no `LET`. Names are
case-sensitive, so `total` and `Total` are different variables. This adds the
numbers from 1 to 100:

```basic
10 S = 0
20 FOR I = 1 TO 100
30 S = S + I
40 NEXT I
50 PRINT S
RUN
```

```text
5050
```

Variables keep their values between runs, and `NEW` does not clear them;
leaving BASIC does.

## Decisions

`IF` compares two values with `=`, `<>`, `<`, `>`, `<=`, or `>=`. When the
comparison is true, it runs the statement after `THEN`, or jumps to a line
number:

```basic
10 FOR T = 15 TO 30 STEP 5
20 IF T < 20 THEN PRINT T, 0
30 IF T >= 20 THEN IF T < 25 THEN PRINT T, 1
40 IF T >= 25 THEN PRINT T, 2
50 NEXT T
RUN
```

```text
15 0
20 1
25 2
30 2
```

There is no `AND` or `ELSE`; chain `IF`s as on line 30 instead. `GOTO n`
jumps to line `n`, and `END` stops the program.

## Subroutines

`GOSUB n` jumps to line `n`, and `RETURN` comes back to the line after the
`GOSUB`. Put subroutines after an `END`:

```basic
10 X = 3
20 GOSUB 100
30 X = 7
40 GOSUB 100
50 END
100 PRINT X, X * X
110 RETURN
RUN
```

```text
3 9
7 49
```

## LEDs

HomeCore adds two functions for the board's LEDs, numbered from 0:

| Function | Effect | Result |
| --- | --- | --- |
| `led(n, state)` | Lights LED `n` when `state` is not 0, turns it off when it is 0 | The new state, 1 or 0 |
| `ledget(n)` | Reads LED `n` | 1 if lit, 0 if not |

Both return -1 when there is no LED `n`. A function call can stand alone as a
statement when you do not need its result:

```basic
led(0, 1)
PRINT ledget(0)
led(0, 0)
PRINT ledget(7)
```

```text
[led0] on
1
[led0] off
-1
```

In QEMU, which has no LEDs, HomeCore prints `[led0] on` and `[led0] off`
lines instead. On a real board the LED changes and nothing is printed. See
[the LED section of the shell guide](shell.md#leds) for each board's LEDs.

This program counts the LEDs by trying numbers until `ledget` returns -1, and
lights each one on the way:

```basic
10 N = 0
20 IF ledget(N) < 0 THEN 60
30 led(N, 1)
40 N = N + 1
50 GOTO 20
60 PRINT N
RUN
```

```text lm3s6965evb
[led0] on
1
```

```text stm32vldiscovery
[led0] on
[led1] on
2
```

The first output is from the LM3S6965EVB in QEMU, the second from the
STM32VLDISCOVERY in QEMU; the STM32F4DISCOVERY would count four.

## Time

`millis()` returns the milliseconds since HomeCore started. There is no
`SLEEP`; to wait, loop until enough time has passed. Line 110 below jumps to
itself until `D` milliseconds have gone by. This program blinks LED 0 three
times, 250 ms on and 250 ms off:

```basic
10 D = 250
20 FOR K = 1 TO 3
30 led(0, 1)
40 GOSUB 100
50 led(0, 0)
60 GOSUB 100
70 NEXT K
80 END
100 T = millis()
110 IF millis() - T < D THEN 110
120 RETURN
RUN
```

```text
[led0] on
[led0] off
[led0] on
[led0] off
[led0] on
[led0] off
```

## Functions

| Function | Result |
| --- | --- |
| `millis()` | Milliseconds since startup |
| `led(n, state)`, `ledget(n)` | LED control; see [LEDs](#leds) |
| `min(a, b, ...)`, `max(a, b, ...)` | The smallest or largest argument |

Functions are lowercase. HomeCore's BASIC has no `sin`, `sqrt`, or other math
functions, which saves flash on small boards.

## Errors

A mistake stops the line or the program with a message; `RUN` names the line:

```basic
Y = Z
10 Y = 1 / 0
RUN
```

```text
Error: undefined variable 'Z'
Error in line 10: division by zero
```

```basic
10 GOTO 500
RUN
```

```text
Error: line 500 not found
```

An error inside `PRINT` currently also prints a line containing only `!`
before the message. This is a known G2Basic issue.

## Good to know

- **One statement per line.** There is no `:` separator.
- **Keywords can be in any case** (`print`, `PRINT`); variable names cannot.
- **A running program cannot be stopped.** `RUN` returns when the program
  ends. A program that loops forever needs a reset: Ctrl-A, then X quits QEMU,
  and the reset button restarts a board.
- **Nesting is limited.** One line may nest parentheses, function calls, and
  `IF`s up to 8 levels (4 on the STM32VLDISCOVERY). Deeper lines fail with
  `expression too deeply nested`.
- **An immediate line cannot start with a digit.** `2 + 3` stores line 2;
  type `PRINT 2 + 3` instead.
