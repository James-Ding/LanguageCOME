# 1. Overview

COME is a systems programming language inspired by C, designed to retain C’s mental model while removing common footguns.

Key design goals:

* Familiar to C programmers
* UTF-8 native string model
* No raw pointers in user code
* Explicit ownership for composite types
* Safe-by-default control flow
* Minimal syntax extensions

# 2. Source File Structure

Each source file must declare exactly one module.

```come
module main
```

## 2.1 Comments

COME supports both single-line and multi-line comments.

```come
// single-line comment

/*
   multi-line comment
*/
```

# 3. Modules and Imports

## 3.1 Importing Modules

Single module:

```come
import std
```

Multiple modules (single line):

```come
import (net, conv)
```

Multiple modules (multi-line):

```come
import (
    string,
    mem
)
```

Imported modules are referenced using dot notation:

```come
std.printf("hello")
net.hton(port)
```

# 4. Constants and Enumerations

## 4.1 Constants

Single constant:

```come
const PI = 3.14
```

Grouped constants:

```come
const (
    a = 1,
    b = 2,
)
```

Trailing commas are allowed.

## 4.2 Enumerations

COME uses `enum` as a value generator within `const` blocks.

```come
const ( RED = enum, YELLOW, GREEN, UNKNOWN )
```

Explicit starting value:

```come
const (
    b_alpha = enum(100),
    b_beta,
    b_delta,
)
```

* Enum values auto-increment
* Enum constants are integers

# 5. Aliases

The `alias` keyword provides C-like typedef and macro behavior.

## 5.1 Type Alias

```come
alias tcpport_t = ushort
alias Point = struct Point
```

## 5.2 Constant Alias (Define)

```come
alias MAX_ARRAY = 5
```

## 5.3 Macro Alias

```come
alias SQUARE(x) = ((x) * (x))
```

# 6. Types

## 6.1 Primitive Types

* `bool`
* `byte`, `ubyte`
* `short`, `ushort`
* `int`, `uint`
* `long`, `ulong`
* `float`, `double`
* `wchar` (32-bit Unicode scalar)

Character literals:

```come
byte c = 'A'     // ASCII
wchar w = '🤔'   // Unicode
```

## 6.2 Composite Types

### 6.2.1 Struct

```come
struct Point {
    int x;
    int y;
};
```

Initialization:

```come
struct Point p = { .x = 5, .y = 10 }
```

### 6.2.2 Union

```come
union TwoBytes {
    short signed_s
    ushort unsigned_s
    byte first_byte
}
```

### 6.2.3 Arrays

**Static Array**

```come
int arr[10]
```

* Size known at compile time
* Automatically freed
* `.free()` is not allowed

**Dynamic Array**

```come
int dyn[]
dyn.alloc(3)
dyn.free()
```

* Allocated at runtime
* Requires explicit `.free()`
* Ownership-aware

# 7. Methods and Ownership

## 7.1 Built-in Composite Methods

All composite types support:

| Method | Description |
|---|---|
| `.length()` | Logical element count |
| `.size()` | Memory size in bytes |
| `.type()` | Runtime type name |
| `.owner()` | Current owner |
| `.chown()` | Transfer ownership |

## 7.2 Struct Methods

Structs may declare methods:

```come
struct TCP_ADDR {
    tcpport_t portnumber
    byte ipaddr[16]
    method nport()
}
```

Method implementation:

```come
byte nport() {
    return net.hton(self.portnumber)
}
```

* `self` refers to the struct instance
* Methods are namespaced to the struct

# 8. Functions

## 8.1 Function Declaration

Prototype:

```come
int add(int a, int b)
```

Definition:

```come
int add(int a, int b) {
    return a + b
}
```

## 8.2 Multiple Return Values

Functions may return tuples:

```come
(int, string) add_n_compare(int a, int b) {
    return (a + b), (a > b) ? ">" : "<="
}
```

Call site:

```come
(sum, cmp) = add_n_compare(i, s)
```

# 9. Variables and Type Inference

## 9.1 `var` Keyword

```come
var x
x = 123        // x becomes int
```

Immediate realization:

```come
var y = "hello"   // y becomes string
```

Rules:

* Type is locked at first assignment
* Cannot change type later
* Still statically typed

# 10. Control Flow

## 10.1 If / Else

```come
if (flag) {
    ...
} else {
    ...
}
```

## 10.2 Switch Statement

COME switch does NOT fall through by default.

```come
switch (value) {
    case RED:
        std.printf("Red\n")
    case GREEN:
        std.printf("Green\n")
    case UNKNOWN:
        fallthrough
    default:
        std.printf("UNKNOWN\n")
}
```

* `fallthrough` must be explicit
* `default` is optional

## 10.3 Loops

```come
for (int i = 0; i < 10; i++) { }

while (cond) { }

do { } while (cond)
```

# 11. Memory Management

## 11.1 Allocation

```come
int dyn[]
dyn.alloc(3)
```

## 11.2 Ownership Propagation

```come
byte buf[]
buf.alloc(512, dyn)
```

* `buf` is owned by `dyn`
* Freeing `dyn` frees all children

```come
dyn.free()
```

# 12. Expressions and Operators

COME supports:

* Arithmetic: `+` `-` `*` `/` `%`
* Bitwise: `&` `|` `^` `~` `<<` `>>`
* Logical: `&&` `||` `!`
* Relational: `==` `!=` `<` `>` `<=` `>=`

# 13. Exports

Symbols exported from a module:

```come
export (PI, Point, add)
```

* Only exported symbols are visible externally
* Unexported symbols remain module-private

# 14. Semicolons

* `;` is optional
* Acts as a line separator
* Useful for multiple statements on one line

# 15. Design Summary

| Feature | Decision |
|---|---|
| Pointers | Removed |
| UTF-8 | Default |
| Switch | No fallthrough |
| Memory | Ownership-based |
| Arrays | Static + dynamic |
| Typing | Static with inference |
| Methods | Object-style |
| Compatibility | C mental model |
