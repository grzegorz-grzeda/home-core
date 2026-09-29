# A first session with HomeCore

This tour takes about ten minutes. You will boot HomeCore in an emulator, look
around, keep some notes in its RAM disk, switch an LED on and off, and write a
small BASIC program. No hardware is needed.

Every command and output below is checked against the real firmware in CI (see
`tests/qemu_docs_test.py`), so what you see should match exactly, except where
a step says the numbers will differ.

## 1. Start it

You need `qemu-system-arm` (on Ubuntu or Debian: `sudo apt install qemu-system-arm`)
and a HomeCore image for the emulated LM3S6965EVB board. Either download
`homecore-vX.Y.Z-lm3s6965evb.elf` from the latest release on the
[Releases page](https://github.com/grzegorz-grzeda/home-core/releases) and run
it, using the file's actual name:

```bash
qemu-system-arm -M lm3s6965evb -nographic -kernel homecore-vX.Y.Z-lm3s6965evb.elf
```

or build it yourself with the [quick start](../README.md#build-it-yourself) and
run `bash scripts/run-qemu-lm3s6965evb.sh`.

HomeCore boots in a fraction of a second:

```text
Timer with period zero, disabling

HomeCore OS
Version: 0.1.3
root:/$
```

The first line comes from QEMU itself and is harmless, and your version may be
newer. `root:/$` is the
prompt: you are the user `root`, in the directory `/`. To leave QEMU at any
time, press **Ctrl-A**, release it, then press **X**.

## 2. Look around

`help` lists every command:

```homecore
help
```

```text
Available commands:
  basic: BASIC language interpreter
  cat: Print file contents: cat path...
  mkdir: Create directories: mkdir path...
  ls: List directory entries: ls [path]
  help: Display this help message
  id: Print user and group IDs
  whoami: Print current user
  pwd: Print working directory
  cd: Change working directory: cd [path]
  write: Write text to a file or device: write path text...
  cp: Copy a file: cp source target
  rm: Remove files: rm path...
  touch: Create empty files: touch path...
  rmdir: Remove empty directories: rmdir path...
  reboot: Reset the board
  clear: Clear terminal
  uptime: Show elapsed time since startup
  mem: Show heap usage
```

Everything in HomeCore is a path. The top level has two directories: `/dev`
holds devices, and `/ram` is a small disk kept in memory:

```homecore
ls /
ls /dev
```

```text
ram/
dev/
uptime
led0
uart2
uart1
uart0
```

`uart0` is the serial port you are typing into, `led0` is an LED, and
`uptime` is a clock you can read like a file. Directory names end with `/`.

## 3. Keep some notes

Move into the RAM disk, make a directory, and write a file. `write` puts the
rest of the line into a file, creating it if needed; `cat` prints it:

```homecore
cd /ram
mkdir notes
write notes/todo.txt buy solder
cat notes/todo.txt
```

```text
buy solder
```

The prompt now shows `root:/ram$`, because relative paths such as
`notes/todo.txt` start from the current directory. Copy the uptime clock into a
file, then list the directory:

```homecore
cp /dev/uptime notes/boot.txt
ls notes
```

```text
boot.txt
todo.txt
```

`notes/boot.txt` now holds the milliseconds since startup at the moment of
the copy. `rm` removes files and `rmdir` removes empty directories.

## 4. Switch an LED

LEDs are files too. Write `on`, `off`, or `toggle` to one, and read it to see
its state:

```homecore
write /dev/led0 on
cat /dev/led0
write /dev/led0 toggle
cat /dev/led0
```

```text
[led0] on
1
[led0] off
0
```

QEMU's board has no real LED, so HomeCore prints `[led0] on` and
`[led0] off` instead. On a real board, such as the STM32F4DISCOVERY with its
four LEDs, the light changes and nothing is printed.

## 5. Write a BASIC program

`basic` starts the built-in BASIC interpreter, whose prompt is `>`. Lines
with a number are stored as a program; `RUN` runs it. This one blinks the LED
three times, waiting 200 milliseconds between changes:

```basic
10 FOR N = 1 TO 3
20 led(0, 1)
30 T = millis()
40 IF millis() - T < 200 THEN 40
50 led(0, 0)
60 T = millis()
70 IF millis() - T < 200 THEN 70
80 NEXT N
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

Press **Ctrl-C** to leave BASIC and return to the shell. The
[BASIC guide](basic.md) explains the language and has more examples.

## 6. Check memory and time

`mem` shows how much memory is in use, and `uptime` how long HomeCore has
been running. Your numbers will differ; for example:

```text
root:/ram$ mem
Heap total: 27336 bytes
Allocated: 3952 bytes
Reusable: 1184 bytes
Unclaimed: 22200 bytes
Available: 23384 bytes
Stack: used 2136 of 3072 bytes
root:/ram$ uptime
up 0 days, 00:03:12.408
```

"Available" is the memory left for new files and programs.

## 7. Reboot

`reboot` resets the board. Everything in memory is gone afterwards, including
the RAM disk, which is formatted again at every boot:

```homecore
cd /
reboot
```

```homecore
ls /ram
```

```text
```

HomeCore has no permanent storage yet; SD card support is planned.

## Where next

- [BASIC guide](basic.md): the language, LEDs, timing, and more programs.
- [Shell and system services](shell.md): every command in detail.
- [Board guides](boards/): running HomeCore on real STM32 boards.
- [Architecture](architecture.md): how it works inside.
