---
layout: ../layouts/DocsLayout.astro
title: Abel — Language specification v0.1
description: The normative v0.1 contract for the Abel programming language.
showToc: true
---

# Abel Language Specification

**Status:** Normative v0.1

This document defines the first version of the Abel scripting language. The
language is designed to be easy to learn, easy to read, and useful for small
numerical programs. It is intentionally small so that an interpreter can be
implemented incrementally.

Sections marked **Non-normative** describe possible future work and are not
part of the v0.1 language contract.

## 1. Design goals

Abel has the following goals:

- readable syntax with little ceremony;
- clear behavior for beginners;
- useful scalar and one-dimensional numerical operations;
- the same language in an interactive REPL and in script files; and
- a stable foundation for future versions.

Abel v0.1 is an interpreted language. The implementation strategy is not part
of the language definition.

## 2. Programs and execution

An Abel program is a sequence of statements. Programs may be entered into an
interactive REPL or stored in a file with the `.abel` extension.

The observable behavior of a valid program must not depend on whether it is
run from the REPL or from a script file, except for environment-specific input
and output behavior.

## 3. Lexical rules

### 3.1 Identifiers

An identifier starts with a letter or underscore and may contain letters,
digits, and underscores afterward.

Examples:

```abel
value
sample_2
_temporary
```

Identifiers are case-sensitive. `value` and `Value` are different identifiers.

### 3.2 Literals

Numbers are written as decimal numeric literals. Abel v0.1 exposes one numeric
type; implementations represent numbers as floating-point values.

```abel
42
3.14
0.5
```

Strings are enclosed in double quotes. A string may contain ordinary text and
the escape sequences `\\`, `\"`, and `\n`.

```abel
"hello"
"line one\nline two"
```

The boolean literals are `true` and `false`. The null literal is `null`.

Array literals use square brackets and comma-separated values:

```abel
[]
[1, 2, 3]
["red", "green", "blue"]
```

Arrays are one-dimensional. Values in one array do not need to have the same
type.

### 3.3 Keywords

The following words are keywords in v0.1 and cannot be used as identifiers:

```text
and else false for fn if not null or return true while
```

### 3.4 Comments and statement endings

`//` starts a comment that continues to the end of the line.

Newlines end statements. A semicolon may be used instead of a newline and may
separate multiple statements on one line.

```abel
x = 1
y = 2; z = x + y
```

Whitespace is otherwise insignificant except inside literals.

## 4. Statements

### 4.1 Assignment

Assignment binds a value to a name. Assigning to an existing name replaces its
value. Variables do not require declarations and may hold values of different
types over time.

```abel
answer = 6 * 7
answer = "forty-two"
```

An assignment is itself a statement and does not produce a value.

### 4.2 Blocks

Blocks are enclosed in braces. Statements inside a block are separated by
newlines or semicolons.

```abel
{
    value = 10
    print(value)
}
```

Standalone blocks are valid, although they are mainly used by other
statements.

### 4.3 Conditional statements

`if` executes its block when its condition is truthy. `else` executes its block
when the condition is not truthy.

```abel
if score >= 50 {
    print("pass")
} else {
    print("retry")
}
```

### 4.4 While loops

`while` repeatedly executes its block while its condition is truthy.

```abel
count = 0
while count < 3 {
    print(count)
    count = count + 1
}
```

### 4.5 For-in loops

`for` iterates over an array in order. The loop variable receives each element
in turn.

```abel
for value in [10, 20, 30] {
    print(value)
}
```

The loop variable is assigned for each iteration. Mutating the loop variable
does not mutate the array element.

### 4.6 Function declarations

Functions are declared with `fn`, a name, a parameter list, and a block.

```abel
fn square(value) {
    return value * value
}
```

Function names share the same namespace as variables. Declaring a function
assigns a callable function value to its name.

### 4.7 Return statements

`return` exits the current function and optionally provides a value.

```abel
fn greet(name) {
    if name == "" {
        return
    }
    return "Hello, " + name
}
```

A function that reaches its closing brace without returning produces `null`.
Using `return` outside a function is a runtime error.

## 5. Expressions

Expressions produce values and may be used in assignments, conditions, return
statements, and function arguments.

### 5.1 Operators

Operators are listed from highest to lowest precedence:

1. Function calls and array indexing
2. Unary `-` and `not`
3. `*`, `/`, `%`
4. `+`, `-`
5. `<`, `<=`, `>`, `>=`
6. `==`, `!=`
7. `and`
8. `or`

Parentheses can be used to override precedence.

```abel
result = (2 + 3) * 4
```

### 5.2 Arithmetic

The arithmetic operators `+`, `-`, `*`, `/`, and `%` operate on numbers.

`+` also concatenates two strings:

```abel
message = "Abel " + "language"
```

Division by zero is a runtime error. Applying an arithmetic operator to
unsupported types is a runtime error.

### 5.3 Comparison and equality

The ordering operators compare numbers. Equality operators compare values of
the same type and return a boolean. Values of different types are unequal,
except that `null == null` is `true`.

Arrays are equal when they have the same length and corresponding elements are
equal. Equality does not mutate either operand.

### 5.4 Logical operators

`and` and `or` return boolean values and use short-circuit evaluation. `not`
returns the logical negation of its operand.

Only `false` and `null` are falsey. Numbers, strings, and arrays are truthy,
including zero, empty strings, and empty arrays.

## 6. Functions and calls

A call evaluates its function expression and then evaluates its arguments from
left to right. The number of arguments must match the number of parameters.

```abel
fn add(left, right) {
    return left + right
}

total = add(2, 3)
```

Calling a non-function value, calling an undefined name, or providing the
wrong number of arguments is a runtime error.

Functions may call other functions, including themselves recursively. Function
scope is local to the call. Names defined outside a function are available to
the function through lexical lookup; assignments update the nearest existing
binding, or create a local binding when no existing binding is found.

## 7. Arrays

Arrays are mutable, ordered, zero-indexed, one-dimensional collections.

```abel
values = [4, 8, 15]
first = values[0]
values[1] = 16
```

The index must be a number representing a valid whole-number index. An
invalid, fractional, negative, or out-of-range index is a runtime error.

Array indexing reads an element. An indexed assignment replaces an existing
element; it does not grow the array.

Slicing, concatenation operators, multidimensional arrays, broadcasting, and
matrix operations are not part of v0.1.

## 8. Built-in functions

The following functions are provided by the initial runtime:

| Function | Behavior |
| --- | --- |
| `print(value)` | Writes a readable representation of `value`. Returns `null`. |
| `length(value)` | Returns the length of a string or array. |
| `sum(array)` | Returns the numeric sum of an array. |
| `min(array)` | Returns the smallest numeric element of an array. |
| `max(array)` | Returns the largest numeric element of an array. |
| `abs(number)` | Returns the absolute value of a number. |
| `sqrt(number)` | Returns the square root of a non-negative number. |

Calling a built-in with an unsupported type, an empty array where an element is
required, or an invalid numeric value is a runtime error.

## 9. Errors

Errors are either syntax errors or runtime errors.

A syntax error prevents a program from executing. A runtime error stops the
current program or REPL input.

Diagnostics must include, when available:

- the error category;
- a concise human-readable message; and
- the source line and column.

The exact wording of diagnostics is not normative, but the error category and
the condition that caused it are observable behavior.

## 10. Compatibility policy

This section is normative.

A valid Abel v0.1 program must remain valid and retain the same observable
behavior in future compatible releases. Observable behavior includes:

- whether the program parses;
- evaluation results;
- variable and array mutations;
- function-call behavior;
- output produced by documented built-ins; and
- documented runtime-error conditions.

Future compatible releases must not:

- remove or repurpose existing syntax;
- change the meaning of an existing operator, keyword, or built-in;
- change indexing from zero-based behavior;
- change the documented truthiness rules; or
- make a previously valid program invalid by adding an unqualified keyword.

New syntax that could conflict with an existing identifier must be introduced
contextually or reserved for a future major language generation. Breaking
changes require explicit migration documentation and a separately identified
language generation.

Abel v0.1 does not define a script-level version header. Version selection and
opt-in compatibility modes may be introduced when Abel reaches 1.0.

## 11. Non-normative roadmap

The following features may be considered later and are not guaranteed:

- multidimensional arrays and matrix operations;
- slicing and broadcasting;
- modules and imports;
- a larger numerical standard library;
- static types;
- anonymous functions and closures;
- exceptions;
- formatted input/output; and
- bytecode or compiled execution.

Future proposals must describe their compatibility impact before becoming part
of a normative language version.

## 12. Complete example

```abel
fn mean(values) {
    return sum(values) / length(values)
}

scores = [72, 81, 95]
average = mean(scores)

if average >= 80 {
    print("Average: " + average)
} else {
    print("Average is below 80")
}
```

