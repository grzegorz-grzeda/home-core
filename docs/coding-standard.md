# C coding standard

HomeCore uses C11 with project rules informed by
[BARR-C:2018](https://barrgroup.com/1-general-rules) and
[SEI CERT C](https://www.sei.cmu.edu/library/sei-cert-c-and-c-coding-standards/).
The rules below define the project's adopted subset; this is not a claim of
BARR-C, CERT C, or MISRA compliance.

## Scope and adoption

Apply these rules to new and changed first-party C code and headers. Improve
existing code in the area being changed; avoid unrelated formatting rewrites.
Third-party code under `external/` follows its upstream conventions. Preserve
license notices and do not hand-edit generated code.

When a low-level requirement needs an exception, explain the constraint and
assumptions in a nearby comment and describe the exception in the change summary.
Use [architecture](architecture.md) for module boundaries and
[development](development.md) for validation commands.

## Formatting and naming

- Follow the repository's `.clang-format`: four spaces, no tabs, attached opening
  braces, and a 100-column limit. Keep formatting changes scoped to edited code.
- Always use braces for `if`, `else`, `for`, `while`, and `do` bodies, including
  single statements. Give intentional empty polling loops a comment.
- Use `snake_case` for functions and variables. Prefix public functions and global
  objects with their module name, such as `vfs_`, `board_`, or `arch_`.
- Use uppercase names for macro constants. Use descriptive names rather than
  encoding a variable's type in its name. Retain established public type names.
- Comments explain contracts, hardware constraints, ownership, and reasoning.
  Avoid comments that simply restate an assignment or condition.

## Types and declarations

- Use fixed-width integer types for registers, binary formats, and values whose
  width is part of the interface. Use `size_t` for object sizes and array counts,
  `ptrdiff_t` for pointer differences, and `bool` for boolean state.
- Match existing public interface types when implementing an API. Validate range
  before converting sizes or values to a narrower type; a cast does not validate.
- Use `uintptr_t` for necessary pointer/integer conversions. Keep hardware address
  conversions in low-level code and document alignment and address assumptions.
- Mark internal functions and file-local objects `static`. Expose only the
  interfaces other modules need. Use `const` for data that should not be modified.
- Initialize values before use. Use designated initializers where they make
  structure initialization clearer. Avoid variable-length arrays in firmware.

## Headers and interfaces

- Headers must be self-contained and protected by include guards. Include the
  headers needed for their own declarations rather than relying on caller order.
- In an implementation file, include its own interface header first. If hardware
  setup requires a different order, document that dependency.
- Declare public functions in their owning header; use `(void)` for functions
  taking no arguments. Do not put storage definitions in public headers.
- Document pointer validity, buffer capacity, ownership, lifetime, return values,
  and interrupt-context restrictions when they are not obvious from the API.
- Document every declaration in `include/homecore` with a Doxygen comment
  inside its header's module group. `doxygen Doxyfile` must finish without
  warnings; see [API documentation](development.md#api-documentation).
- Preserve architecture/SoC/board boundaries. Portable kernel and subsystem code
  must not depend on board register addresses or GPIO pin assignments.

## Memory and buffers

- Make allocation ownership and release responsibility explicit. Check allocation
  failures, and preserve the old allocation when `realloc` fails.
- Check bounds before indexing or copying. Track buffer capacities explicitly;
  account for terminators when working with strings. Validate size arithmetic
  before allocation, including addition and multiplication overflow.
- Do not allocate or free memory in interrupt handlers. Keep large or unbounded
  objects off the stack; the configured main stack is small.
- Define the lifetime of pointers retained by an API. Do not retain pointers to
  expired stack objects or return references to local storage.
- Handle zero-length operations according to the interface contract. Do not
  access a buffer merely because its pointer is non-null.

## Expressions and control flow

- Avoid signed overflow, invalid shift counts, and shifts of negative signed
  values. Use appropriately typed unsigned masks for register bit operations.
- Avoid implicit signed/unsigned mixing and unchecked narrowing. Check that a
  conversion is representable before casting; do not cast just to silence a warning.
- Keep side effects clear. Do not modify the same object through multiple
  unsequenced expressions or hide assignments inside complex conditions.
- Prefer functions or `static inline` functions to function-like macros. If a
  macro is necessary, parenthesize expressions, evaluate each argument at most
  once, and use `do { ... } while (0)` for statement macros.
- Avoid recursion in firmware unless stack usage is explicitly bounded and
  justified. Use explicit fall-through comments for intentional switch fall-through.
- Hardware waits during initialization should have a bounded failure path where
  practical. Document deliberately blocking console reads and permanent halt loops.

## Errors and resource handling

- Check fallible operations and handle partial transfers where the API permits
  them. Do not silently discard errors; document intentional best-effort behavior.
- Follow the owning interface's return conventions. Existing VFS/libc interfaces
  use their documented failure values and `errno`; do not introduce a competing
  error scheme within them.
- Release acquired resources on every error path. A local `goto` cleanup path is
  acceptable when it makes ownership and cleanup easier to verify.
- Use assertions for internal invariants, not as a replacement for runtime input
  validation or error handling. Assertions must not contain required side effects.

## Interrupts and hardware access

- Keep interrupt handlers short and bounded. Do not perform blocking UART I/O,
  allocation, or ordinary libc formatting in a normal interrupt handler.
  Fatal fault/panic handlers may intentionally halt and are a separate case.
- Use CMSIS device definitions for registers and core operations. Keep volatile
  accesses at the hardware boundary; do not hand-copy vendor register layouts
  when suitable device definitions are available.
- `volatile` does not provide atomicity or synchronization. Protect shared state
  using appropriate critical sections or a separately validated atomic protocol.
  Restore the previous interrupt mask rather than unconditionally enabling IRQs.
- Account for register semantics: write-one-to-clear bits, read side effects, and
  reserved bits can make read-modify-write operations incorrect.
- Use CMSIS barriers where the architecture or peripheral requires them; explain
  unusual ordering requirements. Do not assume a compiler barrier alone orders
  hardware transactions.

## Compiler extensions and enforcement

C11 is the language baseline. Necessary GNU attributes, linker sections, and
inline assembly are allowed in architecture, SoC, board, and runtime integration
code. Document their assumptions and isolate them from portable logic. Keep CPU
and floating-point ABI flags consistent across all linked code.

Use `.clang-format` for layout and brace insertion, and review semantic rules
explicitly: formatting alone does not enforce ownership, interrupt safety, or
error handling. Review brace insertion around preprocessor conditionals manually.
Check the documented builds and relevant regressions before submitting code.
Resolve warnings introduced by a change; do not suppress them broadly.

The firmware currently enables `-Wall -Wextra`; documented host-test commands
also use `-Werror`. Add stricter warnings or static-analysis checks incrementally,
reviewing their diagnostics before making them mandatory. CI enforces the quality
script, including formatting and builds with warnings treated as errors. Semantic
coding-standard review remains a separate mandatory completion gate.

## Required review gate

The [mandatory review procedure in AGENTS.md](../AGENTS.md#mandatory-coding-standard-review)
is part of this standard. Every first-party C/header change, including tests,
requires a final-diff semantic review, formatting checks, and applicable build
and regression validation. Review must cover affected contracts as well as
changed lines. Re-review edits made after the review.

Introduced or exposed violations in affected code paths must be fixed or have
a justified low-level exception recorded with the rule, location, reason, and
validation evidence. Report the result and any exceptions explicitly. Missing
checks or unresolved findings mean the review is incomplete. Passing tests is
not a substitute for review, and local review is required even where CI has no
automated enforcement.
