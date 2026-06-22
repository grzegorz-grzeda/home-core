# home-core
HomeCore — a tiny personal computer built on STM32

## Architecture 
Kernel:
- scheduler
- heap
- VFS
- syscalls
- users/sessions

System services:
- shell
- login
- BASIC interpreter

Devices:
- console
- keyboard
- display
- uart
- spi
- i2c

Filesystems:
- ramfs first
- littlefs later

## Milestones
1. Booting monitor
   - [ ] boot on QEMU lm3s6965evb
   - [ ] UART console
   - [ ] printf
   - [ ] malloc
   - [ ] prompt >
   - [ ] help, echo, mem, heap

2. Shell
   - [ ] command parser
   - [ ] argc/argv
   - [ ] command registry
   - [ ] line editor
   - [ ] simple history

3. RAMFS/VFS
   - [ ] /dev/console
   - [ ] /sys/heap
   - [ ] /tmp
   - [ ] ls
   - [ ] cat
   - [ ] write
   - [ ] rm

4. BASIC
   - [ ] basic command
   - [ ] PRINT
   - [ ] LET
   - [ ] INPUT
   - [ ] GOTO
   - [ ] IF
   - [ ] FOR/NEXT
   - [ ] SAVE/LOAD przez VFS

5. Users
   - [ ] login
   - [ ] logout
   - [ ] whoami
   - [ ] session
   - [ ] home directory
   - [ ] simple permissions

6. Threads
   - [ ] idle thread
   - [ ] shell thread
   - [ ] sleep
   - [ ] yield
   - [ ] SysTick
   - [ ] PendSV context switch
   - [ ] mutex

7. SVC/ABI
   - [ ] syscall IDs
   - [ ] SVC wrapper
   - [ ] SVC handler
   - [ ] sys_write/sys_read/sys_open
   - [ ] shell via syscall API
   - [ ] BASIC via syscall API

## Directory structure proposal
```
src/
├── main.c
├── arch/
│   └── armv7m/
│       ├── context_switch.c
│       ├── exception.c
│       ├── svc.c
│       └── include/
│           └── hc_arch.h
├── board/
│   └── qemu_lm3s6965evb/
│       ├── startup.c
│       ├── linker.ld
│       ├── board.c
│       ├── syscalls.c
│       └── include/
│           └── board.h
├── kernel/
│   ├── thread.c
│   ├── scheduler.c
│   ├── heap.c
│   ├── syscall.c
│   ├── mutex.c
│   ├── semaphore.c
│   └── include/
│       ├── hc_thread.h
│       ├── hc_heap.h
│       └── hc_syscall.h
├── vfs/
│   ├── vfs.c
│   ├── devfs.c
│   └── include/
│       └── hc_vfs.h
├── fs/
│   ├── ramfs.c
│   └── include/
│       └── hc_ramfs.h
├── drivers/
│   ├── uart/
│   ├── spi/
│   ├── i2c/
│   ├── display/
│   └── keyboard/
├── shell/
│   ├── shell.c
│   ├── commands.c
│   └── include/
│       └── hc_shell.h
├── user/
│   ├── user.c
│   ├── session.c
│   ├── auth.c
│   └── include/
│       └── hc_user.h
├── basic/
│   ├── basic.c
│   ├── lexer.c
│   ├── parser.c
│   ├── runtime.c
│   └── include/
│       └── hc_basic.h
└── libc/
    └── newlib_syscalls.c
```

## License
Created under MIT license by Grzegorz Grzęda