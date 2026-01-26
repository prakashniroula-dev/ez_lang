# Ezy — The easy, ergonomic, expressive language

A small, ergonomic systems language focused on clarity and simplicity. This repository contains the lexer, parser arena, AST definitions and starter tools to build a compiler for the Ezy language.

Short status: early development — lexer and basic AST/arena implemented; parser + codegen in progress.

---

## Features

- Clean, C-like syntax with ergonomic type system
- Primitive types (int8..int64, uint..., float32/64, char, bool, string)
- Arrays, structs, unions, optional types (T?)
- Type inference for declarations
- Simple, extensible AST and a parser arena allocator
- Clear goals for function and program entry point rules

Full design notes in Architecture.md.

---

## Quickstart

Requirements:
- gcc (or other C compiler)
- make

Build:
```sh
make clean && make all
```

Usage (lexing demo):
```sh
./ezc examples/helloworld/helloworld.ez
```

---

## Example

helloworld.ez
```ez
fn void main() {
  const x = "Hello World";
  let n = 12.2;
  print("string:", x);
  print("num:", n);
}
```

Compiled/transpiled equivalent (example):
```c
#include <stdio.h>
int main() {
  const char* x = "Hello, World!";
  float n = 12.2;
  printf("%s %s\n", "string:", x);
  printf("%s %g\n", "float:", n);
  return 0;
}
```

---

## Project layout

- include/ — headers (lexer, tokens, AST types, parser arena, logs)
- lib/ — lexer, parser arena, parser (work-in-progress)
- src/ — tools (main driver)
- examples/ — sample programs (helloworld)

---

## Contributing

Contributions welcome! Feel free to open issues or pull requests.

---

## License

MIT © Prakash Niroula — @prakashniroula-dev

---
Author: Prakash Niroula — GitHub: https://github.com/prakashniroula-dev