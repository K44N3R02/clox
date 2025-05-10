#include <stdio.h>

#include "compiler.h"
#include "scanner.h"
#ifdef DEBUG_DUMP_CODE
#include "debug.h"
#endif

// parser utility
struct parser {
	struct token previous;
	struct token current;
	bool had_error;
	bool panic_mode;
};

// clang-format off
enum precedence {
	PREC_NONE,
	PREC_ASSIGNMENT,		// =
	PREC_TERNARY,			// ? :
	PREC_OR,			// or
	PREC_AND,			// and
	PREC_COMPARISON,		// == != < > <= >=
	PREC_TERM,			// + -
	PREC_FACTOR,			// * /
	PREC_UNARY,			// - !
	PREC_CALL,			// . ()
	PREC_PRIMARY,
};
// clang-format on

typedef void (*parse_fn_t)(void);

struct parse_rule {
	parse_fn_t prefix;
	parse_fn_t infix;
	enum precedence precedence;
};

struct parser parser;

static void error_at(struct token *token, const char *message)
{
	if (parser.panic_mode)
		return;

	parser.panic_mode = true;

	fprintf(stderr, "[line %d] Error", token->line);

	if (token->token_type == TOKEN_EOF)
		fprintf(stderr, " at end");
	else if (token->token_type != TOKEN_ERROR)
		fprintf(stderr, " at '%.*s'", token->length, token->start);

	fprintf(stderr, ": %s\n", message);
	parser.had_error = true;
}

static void error(const char *message)
{
	error_at(&parser.previous, message);
}

static void error_at_current(const char *message)
{
	error_at(&parser.current, message);
}

static void advance(void)
{
	parser.previous = parser.current;

	for (;;) {
		parser.current = scan_token();
		if (parser.current.token_type != TOKEN_ERROR)
			break;

		error_at_current(parser.current.start);
	}
}

static void consume(enum token_type token_type, const char *message)
{
	if (parser.current.token_type != token_type)
		error_at_current(message);

	advance();
}

// byte emit helpers
struct chunk *compiling_chunk;

static struct chunk *current_chunk(void)
{
	return compiling_chunk;
}

static void emit_byte(uint8_t byte)
{
	write_chunk(current_chunk(), byte, parser.previous.line);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
	emit_byte(byte1);
	emit_byte(byte2);
}
#pragma clang diagnostic pop

static void emit_constant(value_t value)
{
	write_constant(current_chunk(), value, parser.previous.line);
}

static void emit_return(void)
{
	emit_byte(OP_RETURN);
}

static void end_compiler(void)
{
	emit_return();
#ifdef DEBUG_DUMP_CODE
	if (!parser.had_error)
		disassemble_chunk(current_chunk(), "code");
#endif
}

// clox grammar
static void expression(void);
static struct parse_rule *get_rule(enum token_type token_type);
static void parse_precedence(enum precedence precedence);

static void number(void)
{
	double value = strtod(parser.previous.start, NULL);

	emit_constant(value);
}

static void grouping(void)
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void unary(void)
{
	enum token_type operator = parser.previous.token_type;

	parse_precedence(PREC_UNARY);

	switch (operator) {
	case TOKEN_MINUS:
		emit_byte(OP_NEGATE);
	default: // UNREACHABLE
		fprintf(stderr, "Unknown unary operator: %d", operator);
		return;
	}
}

static void binary(void)
{
	enum token_type operator = parser.previous.token_type;
	struct parse_rule *rule = get_rule(operator);

	parse_precedence(rule->precedence + 1);

	switch (operator) {
	case TOKEN_PLUS:
		emit_byte(OP_ADD);
		break;
	case TOKEN_MINUS:
		emit_byte(OP_SUB);
		break;
	case TOKEN_STAR:
		emit_byte(OP_MUL);
		break;
	case TOKEN_SLASH:
		emit_byte(OP_DIV);
		break;
	default: // UNREACHABLE
		fprintf(stderr, "Unknown binary operator: %d", operator);
		return;
	}
}

// WARNING: Incomplete functionality: Only for chapter 17 challenge
// Will be implemented properly when jump instructions are added.
static void ternary(void)
{
	printf("ternary\n");
	parse_precedence(PREC_ASSIGNMENT);
	printf("ternary\n");
	consume(TOKEN_COLON,
		"Expected ':' for else branch of ternary operator.");
	parse_precedence(PREC_TERNARY);
	printf("ternary\n");
}

static void parse_precedence(enum precedence precedence)
{
	parse_fn_t rule_fn;

	advance();
	rule_fn = get_rule(parser.previous.token_type)->prefix;
	if (rule_fn == NULL) {
		error("Expected expression.");
		return;
	}

	// Run prefix rule
	rule_fn();

	while (precedence <= get_rule(parser.current.token_type)->precedence) {
		advance();
		rule_fn = get_rule(parser.previous.token_type)->infix;
		// Run infix rule
		rule_fn();
	}
}

static void expression(void)
{
	parse_precedence(PREC_ASSIGNMENT);
}

// clang-format off
struct parse_rule rules[] = {
	[TOKEN_LEFT_PAREN]	= { grouping,	NULL,		PREC_NONE },
	[TOKEN_RIGHT_PAREN]	= { NULL,	NULL,		PREC_NONE },
	[TOKEN_LEFT_BRACE]	= { NULL,	NULL,		PREC_NONE },
	[TOKEN_RIGHT_BRACE]	= { NULL,	NULL,		PREC_NONE },
	[TOKEN_COMMA]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_COLON]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_DOT]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_MINUS]		= { unary,	binary,		PREC_TERM },
	[TOKEN_PLUS]		= { NULL,	binary,		PREC_TERM },
	[TOKEN_QUESTION]	= { NULL,	ternary,	PREC_TERNARY },
	[TOKEN_SEMICOLON]	= { NULL,	NULL,		PREC_NONE },
	[TOKEN_SLASH]		= { NULL,	binary,		PREC_FACTOR },
	[TOKEN_STAR]		= { NULL,	binary,		PREC_FACTOR },
	[TOKEN_BANG]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_BANG_EQUAL]	= { NULL,	NULL,		PREC_NONE },
	[TOKEN_EQUAL]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_EQUAL_EQUAL]	= { NULL,	NULL,		PREC_NONE },
	[TOKEN_GREATER]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_GREATER_EQUAL]	= { NULL,	NULL,		PREC_NONE },
	[TOKEN_LESS]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_LESS_EQUAL]	= { NULL,	NULL,		PREC_NONE },
	[TOKEN_IDENTIFIER]	= { NULL,	NULL,		PREC_NONE },
	[TOKEN_STRING]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_NUMBER]		= { number,	NULL,		PREC_NONE },
	[TOKEN_AND]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_CLASS]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_ELSE]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_FALSE]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_FOR]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_FUN]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_IF]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_NIL]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_OR]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_PRINT]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_RETURN]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_SUPER]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_THIS]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_TRUE]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_VAR]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_WHILE]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_ERROR]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_EOF]		= { NULL,	NULL,		PREC_NONE },
};
// clang-format on

static struct parse_rule *get_rule(enum token_type token_type)
{
	return rules + token_type;
}

bool compile(char *source, struct chunk *chunk)
{
	init_scanner(source);

	compiling_chunk = chunk;

	parser.had_error = false;
	parser.panic_mode = false;

	advance();
	expression();
	consume(TOKEN_EOF, "Expected end of expression.");
	end_compiler();

	return !parser.had_error;
}
