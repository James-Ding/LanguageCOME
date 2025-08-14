## 1. Basic Syntax

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Entry Point** | `int main(void)` or `int main(string args[])` | `int main(void)` or `int main(int argc, char **argv)` | `func main()` in `package main` |
| **Modules / Packages** | `package` keyword, imports with `import` | None — split into `.h` and `.c` with manual linking | `package` keyword, imports with `import` |
| **Header Files** | No — single package import handles declarations | Required for declarations (`.h`) | No — single package import handles declarations |
| **Comments** | `//` and `/* ... */` | `//` and `/* ... */` | `//` and `/* ... */` |
| **Semicolons** | Optional at line end | Required after each statement | Optional at line end (inserted automatically) |
| **Case Sensitivity** | Yes | Yes | Yes |

## 2. Data Types

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Primitive Types** | `boolean`, `char`, `int`, `short`, `long`, `float`, `double` | `char`, `int`, `short`, `long`, `float`, `double` | `bool`, `string`, `int`, `int8..int64`, `uint`, `uint8..uint64`, `float32`, `float64`, `complex64`, `complex128`, `byte`, `rune` |
| **Strings** | Immutable UTF-16 strings | Null-terminated char arrays | Immutable UTF-8 strings |
| **Booleans** | Built-in `boolean` | No dedicated type (use `_Bool` in C99) | Built-in `bool` |
| **Void** | `void` for no value or generic pointer | `void` for no value or generic pointer | No direct equivalent |
| **Structs** | Yes, with methods | Yes | Yes, with methods |
| **Arrays** | Fixed-size only | Fixed-size only | Fixed-size and slices |
| **Pointers** | Full pointer arithmetic | Full pointer arithmetic | Pointers allowed but no arithmetic |
| **Enums** | Yes (`enum`) | Yes (`enum`) | No native enums |
| **Type Aliases** | `typedef` | `typedef` | `type` keyword |
| **Generics** | None until C23 `_Generic` | None until C23 `_Generic` | Yes |
| **Complex Numbers** | C99 `complex` | C99 `complex` | Built-in `complex64` / `complex128` |

## 3. Variables & Constants

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Declaration** | `int x = 5;` | `int x = 5;` | `var x int = 5` or `x := 5` |
| **Constants** | `#define PI 3.14` or `const double PI = 3.14;` | `#define PI 3.14` or `const double PI = 3.14;` | `const Pi = 3.14` |
| **Scope** | Block, file (static), global | Block, file (static), global | Block, package, global |
| **Mutable Variables** | Yes | Yes | Yes |
| **Immutable Variables** | No | No | Yes via `const` |

## 4. Operators

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Arithmetic** | `+ - * / %` | `+ - * / %` | Same |
| **Bitwise** | `& | ^ << >> ~` | `& | ^ << >> ~` | Same except bitwise NOT is `^` not `~` |
| **Logical** | `&& || !` | `&& || !` | Same |
| **Pointer** | `*` dereference, `&` address-of | Same | Same |
| **Increment / Decrement** | `++` and `--` (prefix & postfix) | Same | Only as statements, not expressions |
| **Ternary Operator** | `cond ? a : b` | Same | Not available |
| **Assignment Shorthand** | `+=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=` | Same | Same |

## 5. Control Flow

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **If / Else** | Yes | Yes | Yes |
| **Switch** | Fallthrough by default | Fallthrough by default | No fallthrough unless `fallthrough` keyword |
| **For Loop** | `for(init; cond; step)` | `for(init; cond; step)` | Single `for` construct |
| **While Loop** | Yes | Yes | No |
| **Do-While Loop** | Yes | Yes | No |
| **Goto** | Yes | Yes | Yes |
| **Break / Continue** | Yes | Yes | Yes |
| **Labels** | Yes | Yes | Yes |

## 6. Functions & Procedures

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Definition** | `int add(int a, int b) { return a+b; }` | Same | `func add(a int, b int) int { return a+b }` |
| **Forward Declaration** | Needed | Needed | Not needed |
| **Multiple Return Values** | No | No | Yes |
| **Default Arguments** | No | No | No |
| **Named Parameters** | No | No | No |
| **Nested Functions** | No (except GNU C extensions) | No (except GNU C extensions) | Yes |
| **Variadic Functions** | Yes (`...`) | Yes (`...`) | Yes (`...`) |
| **Recursion** | Yes | Yes | Yes |
| **Inline Functions** | Yes (`inline`) | Yes (`inline`) | Compiler decides |

## 7. Memory Management

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Stack Allocation** | Yes | Yes | Yes |
| **Heap Allocation** | `malloc`, `calloc`, `realloc`, `free` | Same | `new`, `make`, automatic GC |
| **Garbage Collection** | No | No | Yes |
| **Manual Free** | Yes | Yes | Not needed |
| **Pointer Arithmetic** | Yes | Yes | No |
| **Memory Safety** | Unsafe | Unsafe | Safer, but still possible to misuse pointers |

## 8. Error Handling

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Return Codes** | Yes | Yes | Yes |
| **Errno** | Yes (`errno.h`) | Yes (`errno.h`) | No |
| **Exceptions** | No | No | No |
| **Error Type** | No | No | `error` interface |
| **Panic / Recover** | No | No | Yes |

## 9. Concurrency

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Threads** | POSIX threads | POSIX threads | Goroutines |
| **Channels** | No | No | Yes |
| **Async/Await** | No | No | No |
| **Mutex / Locks** | Yes via libraries | Yes via libraries | Yes (`sync.Mutex`) |
| **Atomic Ops** | Yes via libraries | Yes via libraries | Yes (`sync/atomic`) |

## 10. Object-Oriented Features

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Classes** | No | No | No (structs + methods) |
| **Inheritance** | No | No | No |
| **Interfaces** | No | No | Yes |
| **Methods** | No | No | Yes (receiver functions) |
| **Polymorphism** | No | No | Via interfaces |
| **Encapsulation** | Manual | Manual | Export control via capitalization |

## 11. Tooling & Build

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **Compiler** | `come` | `gcc`, `clang` | `go build` |
| **Build System** | Makefiles | Makefiles | Built-in toolchain |
| **Dependency Management** | Manual | Manual | `go mod` |
| **Testing Framework** | Custom | Custom | Built-in `testing` package |
| **Cross Compilation** | Manual | Manual | Built-in |

## 12. Standard Library & Ecosystem

| Facility | **COME** | **C** | **Go** |
|---|---|---|---|
| **I/O** | `stdio.h` | `stdio.h` | `fmt`, `io` |
| **Networking** | Manual (sockets) | Manual (sockets) | Built-in `net` |
| **String Handling** | Manual | Manual | Rich `strings` package |
| **Math** | `math.h` | `math.h` | `math` |
| **Date/Time** | Manual | Manual | `time` |
| **OS Access** | `unistd.h` etc. | `unistd.h` etc. | `os` package |

---

That would basically make a “Rosetta Stone” of C and Go.

Do you want me to prepare that next?
It would be quite big but extremely handy.
