#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "location.hh"

enum deftype { DEF, MAC, ALIAS };
enum partype { ARG, CON, VARARGS };
enum class argtype { VAR, NUM, STR, ARG, CON };

// A user-defined type.
struct type_definition {
	unsigned size = 1;
	bool big_endian = false;
	// TODO: Add signedness.
};

// Used in a definition to define what arguments are expected.
struct param {
	partype type;
	unsigned size;
};

// Used when calling funtions or declaring macros to pass arguments.
struct arg {
	argtype type;
	std::string str;
	unsigned value;
};

// A definition of a function or macro.
struct definition {
	deftype type;
	unsigned bytecode;
	std::vector<param> parameters;
	std::string alias;
	std::vector<arg> arguments;
};

// Describes how to compile a script, such as what functions are available and
// how much memory is available.
struct environment {
	std::unordered_map<std::string, definition> defines;
	std::string section = "ROMX";
	int terminator = -1;
	unsigned pool = 0;
	unsigned bytecode_count = 0;

	definition * get_define(std::string name) {
		if (!defines.contains(name)) return nullptr;
		return &defines[name];
	}
};

enum statement_type {
	NOOP,
	ASSIGN, CONST_EQU, CONST_NOT, CONST_LT, CONST_LTE, CONST_GT, CONST_GTE,
	CONST_ADD, CONST_SUB, CONST_MULT, CONST_DIV, CONST_BAND, CONST_BOR,
	COPY, EQU, NOT, LT, LTE, GT, GTE, ADD, SUB, MULT, DIV, BAND, BOR,
	DECLARE, DROP, DECLARE_ASSIGN, DECLARE_COPY, LABEL, CALL,
	IF, WHILE, DO, FOR, REPEAT, LOOP, BREAK, CONTINUE, GOTO, CALLASM
};

// The code within a script.
struct statement {
	int type;
	// For operations, this is the destination of the operation. Other
	// statements may repurpose this.
	std::string identifier;
	// The operands of an operation.
	std::string lhs;
	std::string rhs;
	// Arguments passed by the user for a function call.
	std::vector<arg> args;
	// A block of sub-statements, for control structures like if and while.
	std::vector<statement> statements;
	// A block of 1 or 3 conditions. if and while use 1 condition, while
	// a for loop is made up of 3.
	std::vector<statement> conditions;
	// For an if/else statement, an additional block of statements is stored
	// for the `else`.
	std::vector<statement> else_statements;
	// For constant operations, this is the constant value of the rhs.
	// Other statements may repurpose this.
	unsigned value;
	unsigned size;
	yy::location l;
};

// A collection of statements that can be executed.
struct script {
	std::string env;
	std::vector<statement> statements;
	
	void compile(FILE * out, const std::string& name, environment& env);

	void initialize() {
		statements.clear();
		env = "std";
	}
};
