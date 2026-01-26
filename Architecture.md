# Architecture

Ezy — The easy, ergonomic, expressive language

## Features

### Comments

```ez
// single line comment

/*
  Multiline
  Comment
*/
```

### Auto-infer types

Type is auto-inferred if possible
( ONLY in declaration )

```ez
let x = 10; // x is an integer

let y = 10.0; // y is a float

let s = "string"; // s is a string
let c = 'c'; // c is a character

let z; // z is of "var" type (can hold any type)

```

### Explicit type declaration

Types can also be explicitly declared to block
auto inference or convert to the type implicitly

```ez
let int x; // x is now integer

let var z = 10; // x is of "var" type

// Example where it can be effective

let x = 10 / 3; // causes floating division
let int x = 10 / 3; // causes integer division

// integer conversion truncates toward zero
let int x = 10.835;

```

### Primitive types

#### Integer Family

* `int8`
  * 8 bits = 1 byte
  * -128 to 127

* `int16`
  * 16 bits = 2 bytes
  * -32768 to 32767

* `int32` || `int`
  * 4 bytes
  * -2147483648 to 2147483647

* `int64`
  * 64 bits = 8 bytes
  * -9223372036854775808 to 9223372036854775807

* `uint8` || `ubyte`
  * 8 bits = 1 byte
  * 0 to 255

* `uint16`
  * 16 bits = 2 bytes
  * 0 to 65535

* `uint32` || `uint`
  * 4 bytes
  * 0 to 4294967295

* `uint64`
  * 8 bytes
  * 0 to 18446744073709551615

#### Float family

* `float32` || `float`
  * 4 bytes

* `float64`
  * 8 bytes

#### Others

* `char`
  * same as `ubyte` in storage
  * but differs as a type
  * typeof(char) != typeof(ubyte)
  * but
    type_repr(char) == type_repr(ubyte) == typeof(ubyte)

* `null`
  * used in unions, to represent null value as a type
  * default value for non-initialized values
  * default value for optional values

* `void`
  * special type used to indicate nothing, not even null
  * used in functions that return nothing
  * not usable for type of variable

### Non-primitive types

* `var`
  * stores `"any"` type.. (even custom types)
  * type checking available via
    `typeof(variable)` or `type_repr(variable)`

#### Arrays

* `T[number]` : fixed array of T type number sized.
* `T[]` : dynamic size array of T type.
* `T[number?]` : dynamic size array of T type
  guiding initial length
* `T[?number]` : dynamic size array of T type with
  max length constraint
* `T[number?number]` : dynamic size array of T type with
  both guiding initial length & max length constraint

```ez

let int[10] x; // fixed size array

// or

let int x[10]; // both supported

// dynamic size array with guiding initial length
let int[10?] x;

// or let int x[10?];

// dynamic size array, but constrain max length to 100
let int[?100] x;

// dynamic size array, with both guide & max length
let int[10?100] x;

// for easy semantics

let x = [1, 2, 3, 4]; // let infers fixed int[4], but the elements are changeable

let x[] = [1, 2, 3, 4]; // let infers dynamic int[]
let x[?] = [1,2,3,4]; // let infers dynamic int[4?]

// let infers dynamic int[4?8]
// compile error if initial length exceeds max size
let x[?8] = [1,2,3,4];

let x = [1, 'hello']; // let infers fixed var[2]

let x[]; // let infers dynamic var[]
let x[?10]; // let infers dynamic var[?10]
let x[4?8]; // let infers dynamic var[4?8]
...

```

#### Optional type (for union of null & a type)

The ways a variable can be null :

* If no value is provided at declaration
* If it's type is optional type

Optional type is simply, `type?` with a question mark
which will finally be `type | null` union.

```ez
let int x; // infers (let int? x = null)

x = 12; // allowed
x = null; // allowed

let int y = 12; // NOT NULLABLE
y = null; // compile error

// this concept carries on to every type
// even arrays or custom types

// but array elements are NON-NULLABLE unless explicitly
// declared to be nullable

// elements are NOT nullable
// array is nullable (not initialized)
let int[] x;

// elements are nullable
// array is NOT nullable (initialized on declaration)
let int?[]x = [1, 2, null, 3];

// implicit elements & array BOTH nullable
// (array not nitialized)
let int?[]x;

// explicit elements & aray BOTH nullable
let int[]??x = [1, 2, null, 3];

// EXCEPTION: var type can store ANY VALUE.. even null
let x[]; // elements are nullable
let x?[]; // no effect.. same
```

### Custom Types

#### Structs

```ez
// Syntax
struct <type_name> {
  <type> <variable>;
  ...
};
```

Usage example

```ez
struct MyType {
  
  int num;
  float flt;
  string str;

  var arb;

  int? opt_num; // optional
  float? opt_flt; // optional
}

// NOTE : the var arb "CAN" store null values but is
// NOT optional, meaning "null" has to be EXPLICITLY
passed

// Can be used as such ( ORDER MAINTAINED )
// If not correct type or required values -> error
let MyType c = {100, 1.0, "Hello World", null};

// Or this (better, order doesn't matter)
let MyType c = {
  flt = 1.0, str = "Hello", num = 100, arb = null
};
```

#### Unions

Unions are a bit different and easy to make

Note that unions cannot be initialized with any other
types other than specified in the union (including null)

```ez
Union MyType = int | string | bool

// and use it as...

let MyType x = 10; // integer allowed

let MyType z; // error -> MyType isn't nullable !
let MyType z = 2.3; // error -> no float

Union NullableType = int | string | bool | null;
let NullableType z; // allowed
```

union types can be checked easily

```ez
Union MyType = int | string | bool

let MyType x = 10;

let bool isInt = typeof(x) == typeof(int) // true
let bool isString = typeof(x) == typeof(string) // false

```

## Functions

Syntax

```ez
fn int hi() {
  return 10; // REQUIRES a return or error
  return null; // error: returning null not allowed
}

fn int? hi() {
  // may not return a value, nothing -> inferred as null
  
  return null; // explicitly return null is also okay !
}

fn hi() {
  // return type inferred, union if multiple, void if nothing
}

fn var hi() {
  // return anything, but returning is explicitly required
  return null;
}

fn var? hi() {
  // return not required, nothing -> inferred as null
}

fn void hi() {
  // do stuff ...

  // may not return ANY value AT ALL
  return; // just a simple return; is allowed tho
}
```

## Program entry point

Program entry point is the main function

Rules

* may only be defined once across code
* automatically called by program
* return type may only be one of void or int
* may have no arguments or only one argument of type
  string[]

```ez

// example
fn void main() {
  ...
}

// may have arguments ( command line )

fn int main(string[] args) {
  if (args.length == 0) return 1;
  return 0;
}
```
