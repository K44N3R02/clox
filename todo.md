# Challenges
## Chapter 16
- [ ] String interpolation
- [ ] Right shift vs generic type ambiguity
- [ ] Contextual keywords

## Chapter 19
- [ ] Flexible array members instead of double indirection for strings
- [ ] Different handling for owned strings and constant strings from source code
- [ ] Handling of addition with strings (my idea: probably user means the string version of other type, so use use a hidden .to_str() function)

# Design Notes
## Chapter 19
- [ ] Support UTF-8 encoding for strings (i.e. string indexing returns max 4 bytes for UTF-8 encoding) and let user select between ASCII and UTF-8

# Side Notes
## Chapter 18
- [ ] Exception handling
- [ ] Full stack trace when runtime error occurs
