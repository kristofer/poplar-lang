# Simple Object Model (SOM) Language Specification

## Overview

SOM (Simple Object Model) is a minimalistic object-oriented programming language inspired by Smalltalk. It aims to provide a clean and concise syntax while maintaining powerful object-oriented features. SOM emphasizes simplicity, readability, and expressiveness.

## Core Principles

1. Everything is an object
2. All computation is performed by sending messages to objects
3. Classes are objects that create other objects
4. The system is based on a small set of core concepts
5. The language should be simple enough to be implemented in a few thousand lines of code

## Lexical Elements

### Comments

Comments begin with double quotes and end with double quotes:

```
"This is a comment"
```

### Identifiers

Identifiers start with a letter and can contain letters, digits, and underscores:

```
identifier
camelCaseIdentifier
_identifier
```

### Keywords

Keywords are used in method declarations:

- `self`: reference to the receiver object
- `super`: reference to the parent class
- `nil`: the null object
- `true`: the boolean true object
- `false`: the boolean false object

### Literals

#### Integer literals

```
42
-17
0
```

#### String literals

Strings are enclosed in single quotes:

```
'Hello, World!'
'String with ''escaped'' single quotes'
```

#### Symbol literals

Symbols are preceded by a hash character:

```
#symbol
#'symbol with spaces'
```

#### Array literals

Arrays are enclosed in parentheses with elements separated by spaces:

```
#(1 2 3)
#('one' 'two' 'three')
#(1 'two' #three)
```

## Object System

### Classes

Classes are the templates for objects. Each class defines the structure and behavior of its instances.

#### Class Definition

```
ClassName = SuperClass (
    "Class definition body"
    
    "Instance variable declarations"
    |var1 var2 var3|
    
    "Method definitions follow"
)
```

### Messages and Methods

SOM has three types of messages:

1. **Unary messages**: No arguments
   ```
   receiver message
   ```

2. **Binary messages**: One argument, operator-like syntax
   ```
   receiver + argument
   ```

3. **Keyword messages**: One or more arguments with keywords
   ```
   receiver keyword: argument1 anotherKeyword: argument2
   ```

#### Method Definition

```
"Unary method"
methodName = (
    "Method body"
    ^returnValue
)

"Binary method"
+ argument = (
    "Method body"
    ^returnValue
)

"Keyword method"
keyword: arg1 anotherKeyword: arg2 = (
    "Method body"
    ^returnValue
)
```

## Control Structures

SOM implements control structures as messages to objects, maintaining the principle that everything is done through message passing.

### Conditionals

Conditions are implemented as messages to boolean objects:

```
condition ifTrue: [ thenBlock ]
condition ifFalse: [ elseBlock ]
condition ifTrue: [ thenBlock ] ifFalse: [ elseBlock ]
```

### Loops

Loops are implemented as messages to objects:

```
"While loop"
[ condition ] whileTrue: [ body ]
[ condition ] whileFalse: [ body ]

"Iteration"
collection do: [ :each | body ]
1 to: 10 do: [ :i | body ]
```

### Blocks

Blocks are anonymous functions enclosed in square brackets:

```
[ :arg1 :arg2 | body ]
```

Blocks can be assigned to variables and passed as arguments:

```
block := [ :x | x * x ]
result := block value: 5  "Returns 25"
```

## Primitive Types

SOM includes these built-in primitive types:

1. **Object**: The root class of all objects
2. **Class**: The class of all classes
3. **Integer**: Fixed precision integers
4. **Double**: Double-precision floating-point numbers
5. **String**: Sequence of characters
6. **Symbol**: Interned strings used as identifiers
7. **Array**: Indexed collection of objects
8. **Block**: Executable code blocks with arguments
9. **Boolean**: With instances `true` and `false`
10. **Nil**: With single instance `nil`

## Standard Library

SOM includes a minimal standard library with essential functionality:

1. **Collection classes**: Array, Dictionary, Set
2. **Stream classes**: ReadStream, WriteStream
3. **System interface**: File, Directory
4. **Numeric classes**: Integer, Double, Number

## Execution Model

### Program Structure

A SOM program consists of class definitions. The entry point is the `main` method of a designated class.

### Message Lookup

1. When an object receives a message, the method is looked up in the object's class
2. If not found, the search continues in the superclass
3. If the method is not found in any superclass, the object receives the `doesNotUnderstand:` message

### Memory Management

SOM implementations typically use automatic memory management (garbage collection).

## Implementation Notes

The SOM language is designed to be simple enough for educational purposes while powerful enough for non-trivial programming. Reference implementations exist in various languages:

- SOM (Java): The original Simple Object Machine implementation
- CSOM (C): C implementation of SOM
- SOM++ (C++): C++ implementation with additional features
- TruffleSOM (Java): Implementation using the Truffle framework

## Examples

### Hello World

```
Hello = Object (
    "A simple hello world program"
    
    main: args = (
        'Hello, World!' println
    )
)
```

### Factorial Function

```
Factorial = Object (
    "Demonstrates recursion with the factorial function"
    
    factorial: n = (
        n <= 1 
            ifTrue: [ ^1 ]
            ifFalse: [ ^n * (self factorial: n - 1) ]
    )
    
    main: args = (
        (self factorial: 5) println "Prints 120"
    )
)
```

### List Implementation

```
List = Object (
    | first rest |
    
    first = ( ^first )
    
    rest = ( ^rest )
    
    first: anObject = ( first := anObject )
    
    rest: aList = ( rest := aList )
    
    isEmpty = ( ^false )
    
    size = (
        rest isEmpty
            ifTrue: [ ^1 ]
            ifFalse: [ ^1 + rest size ]
    )
    
    at: index = (
        index = 1 
            ifTrue: [ ^first ]
            ifFalse: [ ^rest at: index - 1 ]
    )
    
    append: anObject = (
        rest isEmpty
            ifTrue: [ rest := List new first: anObject ]
            ifFalse: [ rest append: anObject ]
    )
    
    do: aBlock = (
        aBlock value: first.
        rest isEmpty ifFalse: [ rest do: aBlock ]
    )
)

EmptyList = List (
    isEmpty = ( ^true )
    
    size = ( ^0 )
    
    at: index = ( self error: 'Index out of bounds' )
    
    append: anObject = (
        ^List new
            first: anObject;
            rest: EmptyList new
    )
    
    do: aBlock = ( "Do nothing" )
)
```

## Version History

- SOM 1.0: Initial specification
- SOM 1.1: Added support for advanced block features
- SOM 1.2: Enhanced standard library

---

This specification is based on the design principles of the original SOM (Simple Object Machine) language created for educational purposes. Implementations may vary in specific details while adhering to the core principles outlined in this document.
