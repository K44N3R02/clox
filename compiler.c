#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
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
	PREC_EQUALITY,			// == !=
	PREC_COMPARISON,		// < > <= >=
	PREC_TERM,			// + -
	PREC_FACTOR,			// * /
	PREC_UNARY,			// - !
	PREC_CALL,			// . ()
	PREC_PRIMARY,
};
// clang-format on

typedef void (*parse_fn_t)(bool can_assign);

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

static bool check(enum token_type token_type)
{
	return parser.current.token_type == token_type;
}

static bool match(enum token_type token_type)
{
	if (!check(token_type))
		return false;
	advance();
	return true;
}

static void synchronize(void)
{
	parser.panic_mode = false;

	while (parser.current.token_type != TOKEN_EOF) {
		if (parser.previous.token_type == TOKEN_SEMICOLON)
			return;
		switch (parser.current.token_type) {
		case TOKEN_CLASS: /* fall through */
		case TOKEN_FUN: /* fall through */
		case TOKEN_VAR: /* fall through */
		case TOKEN_FOR: /* fall through */
		case TOKEN_IF: /* fall through */
		case TOKEN_WHILE: /* fall through */
		case TOKEN_PRINT: /* fall through */
		case TOKEN_RETURN: /* fall through */
			return;
		default:
			advance();
		}
	}
}

struct local {
	struct token name;
	int depth;
	bool is_captured;
};

struct upvalue {
	uint8_t index;
	bool is_local;
};

enum function_type {
	TYPE_FUNCTION,
	TYPE_SCRIPT,
};

struct compiler {
	struct compiler *enclosing;
	struct object_function *function;
	enum function_type type;
	struct local locals[256];
	int local_count;
	struct upvalue upvalues[256];
	int scope_depth;
};

struct compiler *current = NULL;

static void init_compiler(struct compiler *compiler, enum function_type type)
{
	struct local *local;

	compiler->enclosing = current;
	compiler->function = NULL;
	compiler->type = type;
	compiler->local_count = 0;
	compiler->scope_depth = 0;
	compiler->function = new_function();
	current = compiler;
	if (type != TYPE_SCRIPT)
		current->function->name = copy_string(parser.previous.start,
						      parser.previous.length);

	local = &current->locals[current->local_count++];
	local->depth = 0;
	local->name.length = 0;
	local->name.start = "";
	local->is_captured = false;
}

// byte emit helpers
static struct chunk *current_chunk(void)
{
	return &current->function->chunk;
}

static void emit_byte(uint8_t byte)
{
	write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
	emit_byte(byte1);
	emit_byte(byte2);
}

static void emit_constant(value_t value)
{
	write_constant(current_chunk(), OP_CONSTANT, value,
		       parser.previous.line);
}

static void emit_closure(value_t value)
{
	write_constant(current_chunk(), OP_CLOSURE, value,
		       parser.previous.line);
}

static void emit_return(void)
{
	emit_byte(OP_NIL);
	emit_byte(OP_RETURN);
}

static uint32_t emit_jump(uint8_t jump)
{
	emit_byte(jump);
	emit_bytes(0xff, 0xff);
	return current_chunk()->length - 2;
}

static void emit_loop(uint32_t loop_start)
{
	int32_t offset;

	emit_byte(OP_LOOP);

	offset = current_chunk()->length - loop_start + 2;
	if (offset > UINT16_MAX)
		error("Loop body too large.");
	emit_bytes((offset >> 8) & 0xff, offset & 0xff);
}

// compiler utilities
static void begin_scope(void)
{
	current->scope_depth++;
}

static void end_scope(void)
{
	current->scope_depth--;

	while (current->local_count > 0 &&
	       current->locals[current->local_count - 1].depth >
		       current->scope_depth) {
		if (current->locals[current->local_count - 1].is_captured)
			emit_byte(OP_CLOSE_UPVALUE);
		else
			emit_byte(OP_POP);
		current->local_count--;
	}
}

static struct object_function *end_compiler(void)
{
	struct object_function *function;

	emit_return();
	function = current->function;

#ifdef DEBUG_DUMP_CODE
	if (!parser.had_error)
		disassemble_chunk(current_chunk(),
				  function->name != NULL ?
					  function->name->characters :
					  "<script>");
#endif

	current = current->enclosing;

	return function;
}

// clox grammar
static void expression(void);
static void statement(void);
static void declaration(void);
static struct parse_rule *get_rule(enum token_type token_type);
static void parse_precedence(enum precedence precedence);

static void literal(bool can_assign)
{
	enum token_type literal = parser.previous.token_type;

	switch (literal) {
	case TOKEN_NIL:
		emit_byte(OP_NIL);
		break;
	case TOKEN_TRUE:
		emit_byte(OP_TRUE);
		break;
	case TOKEN_FALSE:
		emit_byte(OP_FALSE);
		break;
	default: // UNREACHABLE
		fprintf(stderr, "Unknown literal %d\n", literal);
		return;
	}
}

static void number(bool can_assign)
{
	double value = strtod(parser.previous.start, NULL);

	emit_constant(CONS_NUMBER(value));
}

static void string(bool can_assign)
{
	struct object_string *str = copy_string(parser.previous.start + 1,
						parser.previous.length - 2);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	emit_constant(CONS_OBJECT(str));
#pragma clang diagnostic pop
}

static uint32_t identifier_constant(struct token *token)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	return add_constant(
		current_chunk(),
		CONS_OBJECT(copy_string(token->start, token->length)));
#pragma clang diagnostic pop
}

static bool identifiers_equal(struct token *name1, struct token *name2)
{
	if (name1->length != name2->length)
		return false;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	return memcmp(name1->start, name2->start, name1->length) == 0;
#pragma clang diagnostic pop
}

static int32_t resolve_local(struct compiler *compiler, struct token *name)
{
	struct local *local;
	int32_t i;

	for (i = compiler->local_count - 1; i >= 0; i--) {
		local = &compiler->locals[i];
		if (identifiers_equal(&local->name, name)) {
			if (local->depth == -1)
				error("Can't read local variable in its own initializer.");
			else
				return i;
		}
	}

	return -1;
}

static int32_t add_upvalue(struct compiler *compiler, uint8_t index,
			   bool is_local)
{
	int32_t upvalue_count = compiler->function->upvalue_count, i;

	for (i = 0; i < upvalue_count; i++) {
		struct upvalue *upvalue = &compiler->upvalues[i];
		if (upvalue->is_local == is_local && upvalue->index == index)
			return i;
	}

	if (compiler->function->upvalue_count == 256) {
		error("Too many closure variables in a function.");
		return 0;
	}

	compiler->upvalues[upvalue_count].is_local = is_local;
	compiler->upvalues[upvalue_count].index = index;
	return compiler->function->upvalue_count++;
}

static int32_t resolve_upvalue(struct compiler *compiler, struct token *name)
{
	int32_t local;

	if (compiler->enclosing == NULL)
		return -1;

	local = resolve_local(compiler->enclosing, name);
	if (local != -1) {
		compiler->enclosing->locals[local].is_captured = true;
		return add_upvalue(compiler, (uint8_t)local, true);
	}

	local = resolve_upvalue(compiler->enclosing, name);
	if (local != -1)
		return add_upvalue(compiler, (uint8_t)local, false);

	return -1;
}

static void named_variable(struct token token, bool can_assign)
{
	// This fills the constant table with identical strings even for set
	// expressions. They point to same string but some of these entries are
	// still unnecessary.
	int32_t arg = resolve_local(current, &token);

	if (arg != -1) {
		if (can_assign && match(TOKEN_EQUAL)) {
			expression();
			emit_bytes(OP_SET_LOCAL, (uint8_t)arg);
		} else {
			emit_bytes(OP_GET_LOCAL, (uint8_t)arg);
		}
		return;
	}

	arg = resolve_upvalue(current, &token);

	if (arg != -1) {
		if (can_assign && match(TOKEN_EQUAL)) {
			expression();
			emit_bytes(OP_SET_UPVALUE, (uint8_t)arg);
		} else {
			emit_bytes(OP_GET_UPVALUE, (uint8_t)arg);
		}
		return;
	}

	arg = identifier_constant(&token);

	if (arg < 1 << 8) {
		if (can_assign && match(TOKEN_EQUAL)) {
			expression();
			emit_bytes(OP_SET_GLOBAL, (uint8_t)arg);
		} else {
			emit_bytes(OP_GET_GLOBAL, (uint8_t)arg);
		}
	} else if (arg < 1 << 24) {
		if (match(TOKEN_EQUAL)) {
			expression();
			emit_bytes(OP_SET_GLOBAL_LONG, (uint8_t)(arg >> 16));
			emit_bytes((uint8_t)(arg >> 8), (uint8_t)arg);
		} else {
			emit_bytes(OP_GET_GLOBAL_LONG, (uint8_t)(arg >> 16));
			emit_bytes((uint8_t)(arg >> 8), (uint8_t)arg);
		}
	} else {
		fprintf(stderr, "Too many arg variables.");
		exit(1);
	}
}

static void variable(bool can_assign)
{
	named_variable(parser.previous, can_assign);
}

static void grouping(bool can_assign)
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void unary(bool can_assign)
{
	enum token_type operator = parser.previous.token_type;

	parse_precedence(PREC_UNARY);

	switch (operator) {
	case TOKEN_BANG:
		emit_byte(OP_NOT);
		break;
	case TOKEN_MINUS:
		emit_byte(OP_NEGATE);
		break;
	default: // UNREACHABLE
		fprintf(stderr, "Unknown unary operator: %d", operator);
		return;
	}
}

static void binary(bool can_assign)
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
	case TOKEN_EQUAL_EQUAL:
		emit_byte(OP_EQUAL);
		break;
	case TOKEN_BANG_EQUAL:
		emit_bytes(OP_EQUAL, OP_NOT);
		break;
	case TOKEN_LESS:
		emit_byte(OP_LESS);
		break;
	case TOKEN_GREATER:
		emit_byte(OP_GREATER);
		break;
	case TOKEN_LESS_EQUAL:
		emit_bytes(OP_GREATER, OP_NOT);
		break;
	case TOKEN_GREATER_EQUAL:
		emit_bytes(OP_LESS, OP_NOT);
		break;
	default: // UNREACHABLE
		fprintf(stderr, "Unknown binary operator: %d", operator);
		return;
	}
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
	bool can_assign = precedence <= PREC_ASSIGNMENT;
	rule_fn(can_assign);

	while (precedence <= get_rule(parser.current.token_type)->precedence) {
		advance();
		rule_fn = get_rule(parser.previous.token_type)->infix;
		// Run infix rule
		rule_fn(can_assign);
	}

	if (can_assign && match(TOKEN_EQUAL))
		error("Invalid assignment target.");
}

static void print_statement(void)
{
	expression();
	consume(TOKEN_SEMICOLON, "Expected semicolon after print statement.");
	emit_byte(OP_PRINT);
}

static void expression_statement(void)
{
	expression();
	consume(TOKEN_SEMICOLON,
		"Expected semicolon after expression statement.");
	emit_byte(OP_POP);
}

static void expression(void)
{
	parse_precedence(PREC_ASSIGNMENT);
}

static void block(void)
{
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
		declaration();

	consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

static void patch_jump(uint32_t offset)
{
	int32_t relative = current_chunk()->length - offset - 2;

	if (relative > UINT16_MAX)
		error("Too much code to jump over.");

	current_chunk()->code[offset] = (relative >> 8) & 0xff;
	current_chunk()->code[offset + 1] = relative & 0xff;
}

static void if_statement(void)
{
	uint32_t then_jump, else_jump;

	consume(TOKEN_LEFT_PAREN, "Expected '(' before if condition.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after if condition.");

	then_jump = emit_jump(OP_JUMP_IF_FALSE);
	statement();
	else_jump = emit_jump(OP_JUMP);
	patch_jump(then_jump);

	if (match(TOKEN_ELSE))
		statement();

	patch_jump(else_jump);
	emit_byte(OP_POP);
}

static void and_(bool can_assign)
{
	uint32_t end_jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	parse_precedence(PREC_AND);
	patch_jump(end_jump);
}

static void or_(bool can_assign)
{
	uint32_t else_jump = emit_jump(OP_JUMP_IF_FALSE),
		 end_jump = emit_jump(OP_JUMP);
	patch_jump(else_jump);
	emit_byte(OP_POP);
	parse_precedence(PREC_OR);
	patch_jump(end_jump);
}

static void ternary(bool can_assign)
{
	uint32_t else_jump, end_jump;

	else_jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	parse_precedence(PREC_ASSIGNMENT);
	end_jump = emit_jump(OP_JUMP);
	consume(TOKEN_COLON,
		"Expected ':' for else branch of ternary operator.");
	patch_jump(else_jump);
	emit_byte(OP_POP);
	parse_precedence(PREC_TERNARY);
	patch_jump(end_jump);
}

static uint8_t argument_list(void)
{
	uint8_t arg_count = 0;

	if (!check(TOKEN_RIGHT_PAREN)) {
		do {
			expression();
			if (arg_count == 255)
				error("Cannot call a function with more than 255 arguments.");
			arg_count++;
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after argument list");

	return arg_count;
}

static void call(bool can_assign)
{
	uint8_t arg_count = argument_list();
	emit_bytes(OP_CALL, arg_count);
}

static void while_statement(void)
{
	uint16_t exit_jump;
	uint32_t loop_start = current_chunk()->length;

	consume(TOKEN_LEFT_PAREN, "Expected '(' before while condition.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after while condition.");

	exit_jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	statement();
	emit_loop(loop_start);

	patch_jump(exit_jump);
	emit_byte(OP_POP);
}

static void variable_declaration(void);

static void for_statement(void)
{
	uint16_t exit_jump;
	bool has_condition;
	uint32_t loop_start;

	begin_scope();

	consume(TOKEN_LEFT_PAREN, "Expected '(' before for clauses.");
	if (match(TOKEN_SEMICOLON))
		; // No initializer
	else if (match(TOKEN_VAR))
		variable_declaration();
	else
		expression_statement();

	loop_start = current_chunk()->length;
	has_condition = false;
	if (!match(TOKEN_SEMICOLON)) {
		expression();
		consume(TOKEN_SEMICOLON, "Expected ';' after for condition.");

		has_condition = true;
		exit_jump = emit_jump(OP_JUMP_IF_FALSE);
		emit_byte(OP_POP);
	}

	if (!match(TOKEN_RIGHT_PAREN)) {
		uint32_t body_jump = emit_jump(OP_JUMP),
			 increment_start = current_chunk()->length;
		expression();
		emit_byte(OP_POP);
		consume(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses.");

		emit_loop(loop_start);
		loop_start = increment_start;
		patch_jump(body_jump);
	}

	statement();
	emit_loop(loop_start);

	if (has_condition) {
		patch_jump(exit_jump);
		emit_byte(OP_POP); // condition
	}

	end_scope();
}

static void return_statement(void)
{
	if (current->type == TYPE_SCRIPT) {
		error("Cannot return from top-level code.");
	}

	if (match(TOKEN_SEMICOLON)) {
		emit_return();
	} else {
		expression();
		consume(TOKEN_SEMICOLON,
			"Expected semicolon after return expression.");
		emit_byte(OP_RETURN);
	}
}

static void statement(void)
{
	if (match(TOKEN_PRINT)) {
		print_statement();
	} else if (match(TOKEN_IF)) {
		if_statement();
	} else if (match(TOKEN_WHILE)) {
		while_statement();
	} else if (match(TOKEN_FOR)) {
		for_statement();
	} else if (match(TOKEN_LEFT_BRACE)) {
		begin_scope();
		block();
		end_scope();
	} else if (match(TOKEN_RETURN)) {
		return_statement();
	} else {
		expression_statement();
	}
}

static void add_local(struct token name)
{
	struct local *local;

	if (current->local_count >= 256) {
		error("Too many local variables in function.");
		return;
	}

	local = &current->locals[current->local_count++];
	local->name = name;
	local->depth = -1;
	local->is_captured = false;
}

static void mark_initialized(void)
{
	if (current->scope_depth == 0)
		return;
	current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void declare_variable(void)
{
	struct token *name;
	struct local *local;
	int32_t i;

	if (current->scope_depth == 0)
		return;

	name = &parser.previous;

	for (i = current->local_count - 1; i >= 0; i--) {
		local = &current->locals[i];
		if (local->depth != -1 && local->depth < current->scope_depth)
			break;

		if (identifiers_equal(&local->name, name))
			error("Already there is a local with same name.");
	}

	add_local(*name);
}

static uint32_t parse_variable(const char *msg)
{
	consume(TOKEN_IDENTIFIER, msg);

	declare_variable();
	if (current->scope_depth > 0)
		return 0;

	return identifier_constant(&parser.previous);
}

static void define_variable(uint32_t global)
{
	if (current->scope_depth > 0) {
		mark_initialized();
		return;
	}

	if (global < 1 << 8) {
		emit_bytes(OP_DEFINE_GLOBAL, global);
	} else if (global < 1 << 24) {
		emit_bytes(OP_DEFINE_GLOBAL_LONG, (uint8_t)(global >> 16));
		emit_bytes((uint8_t)(global >> 8), (uint8_t)global);
	} else {
		fprintf(stderr, "Too many global variables.");
		exit(1);
	}
}

static void variable_declaration(void)
{
	uint32_t global =
		parse_variable("Expected variable name after keyword var.");

	if (match(TOKEN_EQUAL))
		expression();
	else
		emit_byte(OP_NIL);

	consume(TOKEN_SEMICOLON, "Expected ; after variable declaration.");

	define_variable(global);
}

static void function(enum function_type type)
{
	struct compiler compiler;
	struct object_function *function;
	uint32_t constant;
	int32_t i;

	init_compiler(&compiler, type);
	begin_scope();

	consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
	if (!match(TOKEN_RIGHT_PAREN)) {
		do {
			current->function->arity++;
			if (current->function->arity > 255)
				error_at_current(
					"Function cannot have more than 255 parameters.");
			constant = parse_variable("Expected parameter name.");
			define_variable(constant);
		} while (match(TOKEN_COMMA));
		consume(TOKEN_RIGHT_PAREN, "Expected ')' after argument list.");
	}
	consume(TOKEN_LEFT_BRACE, "Expected '{' before function body.");

	block();

	function = end_compiler();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	emit_closure(CONS_OBJECT(function));
#pragma clang diagnostic pop
	for (i = 0; i < function->upvalue_count; i++)
		emit_bytes(compiler.upvalues[i].is_local ? 1 : 0,
			   compiler.upvalues[i].index);
}

static void function_declaration(void)
{
	uint8_t global = parse_variable("Expected function name.");
	mark_initialized();
	function(TYPE_FUNCTION);
	define_variable(global);
}

static void declaration(void)
{
	if (match(TOKEN_FUN))
		function_declaration();
	else if (match(TOKEN_VAR))
		variable_declaration();
	else
		statement();

	if (parser.panic_mode)
		synchronize();
}

// clang-format off
struct parse_rule rules[] = {
	[TOKEN_LEFT_PAREN]	= { grouping,	call,		PREC_CALL },
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
	[TOKEN_BANG]		= { unary,	NULL,		PREC_NONE },
	[TOKEN_BANG_EQUAL]	= { NULL,	binary,		PREC_EQUALITY },
	[TOKEN_EQUAL]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_EQUAL_EQUAL]	= { NULL,	binary,		PREC_EQUALITY },
	[TOKEN_GREATER]		= { NULL,	binary,		PREC_COMPARISON },
	[TOKEN_GREATER_EQUAL]	= { NULL,	binary,		PREC_COMPARISON },
	[TOKEN_LESS]		= { NULL,	binary,		PREC_COMPARISON },
	[TOKEN_LESS_EQUAL]	= { NULL,	binary,		PREC_COMPARISON },
	[TOKEN_IDENTIFIER]	= { variable,	NULL,		PREC_NONE },
	[TOKEN_STRING]		= { string,	NULL,		PREC_NONE },
	[TOKEN_NUMBER]		= { number,	NULL,		PREC_NONE },
	[TOKEN_AND]		= { NULL,	and_,		PREC_AND },
	[TOKEN_CLASS]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_ELSE]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_FALSE]		= { literal,	NULL,		PREC_NONE },
	[TOKEN_FOR]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_FUN]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_IF]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_NIL]		= { literal,	NULL,		PREC_NONE },
	[TOKEN_OR]		= { NULL,	or_,		PREC_OR },
	[TOKEN_PRINT]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_RETURN]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_SUPER]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_THIS]		= { NULL,	NULL,		PREC_NONE },
	[TOKEN_TRUE]		= { literal,	NULL,		PREC_NONE },
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

struct object_function *compile(char *source)
{
	struct compiler compiler;
	struct object_function *function;

	init_scanner(source);
	init_compiler(&compiler, TYPE_SCRIPT);

	parser.had_error = false;
	parser.panic_mode = false;

	advance();

	while (!match(TOKEN_EOF)) {
		declaration();
	}

	function = end_compiler();

	return !parser.had_error ? function : NULL;
}
