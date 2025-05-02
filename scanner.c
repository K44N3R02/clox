#include <ctype.h>
#include <string.h>

#include "scanner.h"

struct scanner {
	char *start;
	char *current;
	int32_t line;
};

struct scanner scanner;

void init_scanner(char *source)
{
	scanner.start = source;
	scanner.current = source;
	scanner.line = 1;
}

static bool is_at_end(void)
{
	return *scanner.current == '\0';
}

static struct token make_token(enum token_type token_type)
{
	return (struct token){
		.token_type = token_type,
		.start = scanner.start,
		.length = scanner.current - scanner.start,
		.line = scanner.line,
	};
}

static struct token error_token(const char *msg)
{
	return (struct token){
		.token_type = TOKEN_ERROR,
		.start = msg,
		.length = strlen(msg),
		.line = scanner.line,
	};
}

static char advance(void)
{
	scanner.current++;
	return scanner.current[-1];
}

static bool match(char c)
{
	if (is_at_end())
		return false;

	if (*scanner.current != c)
		return false;

	scanner.current++;
	return true;
}

static char peek(void)
{
	return *scanner.current;
}

static char peek_next(void)
{
	if (is_at_end())
		return '\0';
	return scanner.current[1];
}

static void skip_whitespace(void)
{
	char c;

	for (;;)
		switch (c = peek()) {
		case '\n':
			scanner.line++;
			/* fall through */
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		case '/':
			if (peek_next() == '/')
				while (peek() != '\n' && !is_at_end())
					advance();
			/* fall through */
		default:
			return;
		}
}

static struct token string(void)
{
	char c;
	bool b;

	while ((c = peek()) != '"' && !(b = is_at_end())) {
		if (c == '\n')
			scanner.line++;
		advance();
	}

	if (b)
		return error_token("Unterminated string.");

	advance(); // consume closing '"'
	return make_token(TOKEN_STRING);
}

static struct token number(void)
{
	while (isdigit(peek()))
		advance();

	if (peek() == '.' && isdigit(peek_next())) {
		// consume '.'
		advance();
		while (isdigit(peek()))
			advance();
	}

	return make_token(TOKEN_NUMBER);
}

static enum token_type check_keyword(int32_t start, int32_t length,
				     const char *rest,
				     enum token_type token_type)
{
	if (scanner.current - scanner.start != start + length)
		return TOKEN_IDENTIFIER;

	if (memcmp(scanner.start + start, rest, length) != 0)
		return TOKEN_IDENTIFIER;

	return token_type;
}

static enum token_type identifier_type(void)
{
	switch (*scanner.start) {
	case 'a':
		return check_keyword(1, 2, "nd", TOKEN_AND);
	case 'c':
		return check_keyword(1, 4, "lass", TOKEN_CLASS);
	case 'e':
		return check_keyword(1, 3, "lse", TOKEN_ELSE);
	case 'f':
		if (scanner.current - scanner.start < 2)
			break;
		switch (scanner.start[1]) {
		case 'a':
			return check_keyword(2, 3, "lse", TOKEN_FALSE);
		case 'o':
			return check_keyword(2, 1, "r", TOKEN_FOR);
		case 'u':
			return check_keyword(2, 1, "n", TOKEN_FUN);
		default:
			break;
		}
	case 'i':
		return check_keyword(1, 1, "f", TOKEN_IF);
	case 'n':
		return check_keyword(1, 2, "il", TOKEN_NIL);
	case 'o':
		return check_keyword(1, 1, "r", TOKEN_OR);
	case 'p':
		return check_keyword(1, 4, "rint", TOKEN_PRINT);
	case 'r':
		return check_keyword(1, 5, "eturn", TOKEN_RETURN);
	case 's':
		return check_keyword(1, 4, "uper", TOKEN_SUPER);
	case 't':
		if (scanner.current - scanner.start < 2)
			break;
		switch (scanner.start[1]) {
		case 'h':
			return check_keyword(2, 2, "is", TOKEN_THIS);
		case 'r':
			return check_keyword(2, 2, "ue", TOKEN_TRUE);
		default:
			break;
		}
	case 'v':
		return check_keyword(1, 2, "ar", TOKEN_VAR);
	case 'w':
		return check_keyword(1, 4, "hile", TOKEN_WHILE);
	}
	return TOKEN_IDENTIFIER;
}

static struct token identifier(void)
{
	char c;

	for (c = peek(); isalnum(c) || c == '_'; c = peek())
		advance();

	return make_token(identifier_type());
}

struct token scan_token(void)
{
	char c;

	skip_whitespace();
	scanner.start = scanner.current;

	if (is_at_end())
		return make_token(TOKEN_EOF);

	c = advance();

	if (isdigit(c))
		return number();
	if (isalpha(c) || c == '_')
		return identifier();

	switch (c) {
	case '(':
		return make_token(TOKEN_LEFT_PAREN);
	case ')':
		return make_token(TOKEN_RIGHT_PAREN);
	case '{':
		return make_token(TOKEN_LEFT_BRACE);
	case '}':
		return make_token(TOKEN_RIGHT_BRACE);
	case ',':
		return make_token(TOKEN_COMMA);
	case '.':
		return make_token(TOKEN_DOT);
	case '-':
		return make_token(TOKEN_MINUS);
	case '+':
		return make_token(TOKEN_PLUS);
	case ';':
		return make_token(TOKEN_SEMICOLON);
	case '/':
		return make_token(TOKEN_SLASH);
	case '!':
		return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
	case '=':
		return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
	case '>':
		return make_token(match('=') ? TOKEN_GREATER_EQUAL :
					       TOKEN_GREATER);
	case '<':
		return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
	case '"':
		return string();
	}

	return error_token("Unexpected character.");
}
