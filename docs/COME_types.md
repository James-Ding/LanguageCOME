# COME Data Types

The COME language provides a set of fixed-width primitive integer types and a  type.

## Primitive Types

COME provides a set of fixed-width primitive types.

| Type | Alias | Size (Bytes) | Description |
| :--- | :--- | :--- | :--- |
| `bool` | N/A | 1 | Boolean true/false value. |
| `wchar` | N/A | 4 | A single **Unicode character**, declared with single quotes, e.g., `'A'`, `'🤔'`. |
| `byte` | `i8` | 1 | Signed 8-bit integer. |
| `ubyte` | `u8` | 1 | Unsigned 8-bit integer. |
| `short` | `i16` | 2 | Signed 16-bit integer. |
| `ushort` | `u16` | 2 | Unsigned 16-bit integer. |
| `int` | `i32` | 4 | Signed 32-bit integer. |
| `uint` | `u32` | 4 | Unsigned 32-bit integer. |
| `long` | `i64` | 8 | Signed 64-bit integer. |
| `ulong` | `u64` | 8 | Unsigned 64-bit integer. |
| `float` | `f32` | 4 | IEEE 754 Single-precision (32-bit) float. |
| `double` | `f64` | 8 | IEEE 754 Double-precision (64-bit) float. |
| `void` | N/A | 0 | Indicates no return value or a generic pointer. |

## Composite Types

Composite types are structures built from other types.  
Their memory lifetime is managed by **explicit ownership**, tied to the parent composite variable or the containing module.

| Type      | Declaration Syntax    | Initialization Syntax                           | Description                                                        |
|-----------|------------------------|--------------------------------------------------|--------------------------------------------------------------------|
| `string` | `string name;`         | `string name = "John"`                           | Immutable, UTF-8 encoded sequence of characters.                   |
| `struct` | `MyStruct data;`       | `data = MyStruct{ field: value };`               | Custom aggregated data structure.                                  |
| `array`  | `T name[];`            | `int numbers[] = [10, 20, 30]`                   | Fixed-size collection. Size inferred from the initializer list `[]`. |
| `map`    | `map map_name{};`      | `map students = { "name" : "John", "age" : 16 }` | Unordered key-value collection using `{}` for initialization.       |
| `module` | *N/A*                  | *N/A*                                            | The top-level execution scope and lifetime container.               |


## 3. Built-in Methods for Composite Types

All composite variables automatically possess the following methods. These methods enable runtime introspection, memory size checks, and management of the object's memory lifetime via the ownership system.
 Composite Type `.length()` Returns (Logical ount).`.size()` Returns (Physical Bytes).

| Method | Syntax | Return Type | Description |
| :--- | :--- | :--- | :--- |
| `type()` | `variable.type()` | `string` | Returns the **name of the object's dynamic type** as a lowercase string (e.g., `"string"`, `"map"`, `"mystruct"`). |
| `length()` | `variable.length()` | `int` | Returns the **logical count** of elements/runes/pairs contained within the variable. |
| `size()` | `variable.size()` | `long` | Returns the total memory size of the object, in **bytes**, including headers and contained data. |
| `owner()` | `variable.owner()` | `composite` / `module` | Returns the **composite variable** or **module name** that currently owns the object. |
| `chown()` | `variable.chown(new_owner_var)` | `void` | Explicitly transfers the object and its memory subtree to the `new_owner_var` (a composite variable or a module name).  |

#### **Length and Size Behavior**


| :--- | :--- | :--- |
| `string` | Number of **Unicode characters**. | Number of **bytes** (includes header). |
| `array` | Number of **elements** (fixed capacity). | Total memory for the array and its elements. |
| `map` | Number of **key-value pairs**. | Total memory used by the map structure (buckets, keys, values). |
| `struct` | **fields** number of all fields of the struct| Total memory size of all fields in the struct. |

### Usage Example

```come
int main() {
    byte b = 100
    ushort porta = 8080
    uint count = 1024
    long timestamp = 1678886400

    return 0
}
```


### Usage Example

```come
import std

int main(string args[]) {
    // Check array length
    if (args.length() > 0) {
        std.printf("First argument: %s\n", args[0])
    }

    // Array of integers
    int numbers[] = [1, 2, 3, 4, 5]
    std.printf("Count: %d\n", numbers.length())

    return 0
}
```
