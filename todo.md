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

# Design Notes
## Chapter 14
- [ ] Preparing tests for language

## Chapter 19
- [ ] Support UTF-8 encoding for strings (i.e. string indexing returns max 4 bytes for UTF-8 encoding) and let user select between ASCII and UTF-8

# Side Notes
## Chapter 18
- [ ] Exception handling
- [ ] Full stack trace when runtime error occurs
