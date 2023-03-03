# Guacamole Lang README

<img src="https://github.com/Gomez0015/guacamole_lang/blob/master/G_logo.png?raw=true" alt="Guacamole Logo" width="300" height="331" />

## Introduction

**Name :** Guacamole,
**File Extension :** .g,
**Author :** Raxo,
**Grammar :** PEG (lang.peg)

## Syntax Highlighting

if you would like a syntax highlighter for *Guacamole*, I have included a `.vsix` file which can easily be installed on VSCode for syntax highlighting.

## Usage

Compile Compiler:
```sh
> make clean && make && make compiler
```

Interpret Code:
```sh
> ./compiler code.g
```

## Definitions

### Primitive Operators

```c
+       // ADD
-       // SUBTRACT
*       // MULTIPLY
/       // DIVIDE
^       // RAISE TO POWER
%       // MODULO
```

### Unary Operators

```c
+       // STAY
-       // FLIP
!       // 0 IF BIGGER THAN 0 ELSE 1
```

### Comparisons

```c
==      // EQUAL TO
!=      // NOT EQUAL TO
<=      // LESS THAN OR EQUAL TO
<       // LESS THAN
>=      // MORE THAN OR EQUAL TO
>       // MORE THAN
```

### Logical Operators

```c
||      // OR
&&      // AND
```

### Variables

```c
a = 1;

b = a + 5;

c = (b ^ 4) * 4;
```

### Control Operators

```c
break    // BREAK LOOP
return   // RETURN FROM FUNK
continue // SKIP TO NEXT LOOP ITERATION
```

### Keywords

```c
if      // IF CONDITION
elif    // ELSE IF CONDITION
else    // ELSE CONDITION
while   // WHILE LOOP
```

### Conditionals

```c
// IF BLOCK
if (a == 1 || a == 2) {
    a = 4;
} 
// ELIF BLOCK
elif (a < 1 && a < 2) {
    a = 1;
}
// ELSE BLOCK
else {
    a = 2;
}

// WHILE BLOCK
while (a < 5) {
    a = a + 1;
}
```

### Functions

```c
funk add(a, b) {
    return a + b;
}
```

### Builtins

```c
print(a);   // PRINTS WITH 1 SPACE AFTER
println(a); // PRINTS WITH NEWLINE AFTER
```

