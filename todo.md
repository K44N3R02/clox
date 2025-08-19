# Challenges
## Chapter 14
- [x] Run-length encoding
- [x] OP_CONSTANT_LONG and write_constant()
- [ ] Implementation of malloc and free
- [ ] Manual heap management

## Chapter 15
- [ ] Growing stack as needed (a better solution would be in my opinion, going through the chunk to determine max size required for the chunk like done in JVM)
- [x] In-place negation
- [ ] Consider only poping the first argument and change the second argument in place

## Chapter 16
- [ ] String interpolation
- [ ] Right shift vs generic type ambiguity
- [ ] Contextual keywords

## Chapter 17
- [x] Ternary operator

## Chapter 18
- [ ] Test increasing and decreasing number of op_codes and test performance

## Chapter 19
- [x] Flexible array members instead of double indirection for strings
- [ ] Different handling for owned strings and constant strings from source code
- [ ] Handling of addition with strings (my idea: probably user means the string version of other type, so use use a hidden .to_str() function)

## Chapter 20
- [ ] Add support for all value types (number, boolean, nil) to be keys of a hash table
- [ ] Add support for user defined class instances to be keys of a hash table
- [ ] Add benchmark for hash tables and try some alternative hash tables

## Chapter 21
- [ ] Optimize identifiers usage of constant table. Every time an identifier is encountered, the name is added to constant table even if it already exist.
        My idea: in table strings, give identifiers an id and get them by id
- [ ] Hash table lookup is slow, come up with a faster solution
        My idea: using the id, create an array and store the values (or pointers) and access from there
- [ ] How can we report the issue in the following code? Function is never executed, so no runtime error will occur.
        ```
        fun useVar() {
          print oops;
        }

        var ooops = "too many o's!";
        ```

## Chapter 22
- [ ] Think of a faster `resolve_local()` than O(n)
- [ ] `var a = a;` What other languages do for this situation, what would you do?
- [ ] Add support for constant variables, give compile time error if assign is attempted. `let a = 1; a = 2; // compile error`
- [ ] Handle more than 256 locals

## Chapter 23
- [ ] Add `switch` statement with with O(n) complexity, with automatic break, don't support fallthrough
- [ ] Add `continue` and `break`, think about local variables, ensure they are in loop bodies

## Chapter 24
- [ ] Try to keep `ip` in a register by holding it in a local variable and write to `call_frame` only when necessary. Benchmark to see if added complexity is worth it
- [ ] Arity check for native functions
- [ ] Let native functions signal runtime error
- [ ] Add some more useful native functions

## Chapter 25
- [ ] Only wrap functions using upvalues with object_closure
- [ ] Add an option to capture values instead of variables `fun x() capture {}`

# Design Notes
## Chapter 14
- [ ] Preparing tests for language

## Chapter 19
- [ ] Support UTF-8 encoding for strings (i.e. string indexing returns max 4 bytes for UTF-8 encoding) and let user select between ASCII and UTF-8

# Side Notes
## Chapter 18
- [ ] Exception handling
- [ ] Full stack trace when runtime error occurs

## Chapter 22
- [ ] Track the names of the locals for debugger

## Chapter 24
- [ ] Allow `return` at top-level code for early exiting and use it as exit code

# My Todos
- [ ] Go through the chunk and resize the stack size of vm accordingly
- [ ] Support `var x = 1, y, z="asd";`
- [ ] Think about supporting shadow declarations `var x = 1; var x = x.to_str();`, may be useful for static typing
- [ ] Implement `switch` with O(1) complexity
- [ ] Implement `goto`
- [ ] String escape sequences
- [x] Support expression functions `fun a(x) = x + 1 // fun a(x) { return x + 1; }`
- [x] Add lambda functions `(fun (x) { print x; })(2); (fun (x) = x * 2)(3);`
- [ ] Add documentation for each function

