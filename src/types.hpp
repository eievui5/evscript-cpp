#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "location.hh"

enum deftype { DEF, MAC, ALIAS };
enum partype { ARG, CON, VARARGS };
enum class argtype { VAR, NUM, STR, ARG, CON };

struct type_definition {
	unsigned size = 1;
	bool big_endian = false;
};

struct param {
	partype type;
	unsigned size;
};

struct arg {
	argtype type;
	std::string str;
	unsigned value;
};

struct definition {
	deftype type;
	unsigned bytecode;
	std::vector<param> parameters;
	std::string alias;
	std::vector<arg> arguments;
};

struct environment {
	std::unordered_map<std::string, definition> defines;
	std::string section = "ROMX";
	unsigned terminator = 0;
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
	IF, WHILE, DO, FOR, REPEAT, LOOP, BREAK, CONTINUE, GOTO, CALLASM, PURGE
};

struct statement {
	int type;

	std::string identifier;

	std::string lhs;
	std::string rhs;

	std::vector<arg> args;
	std::vector<statement> statements;
	std::vector<statement> conditions;
	std::vector<statement> else_statements;
	unsigned value;
	unsigned size;
	yy::location l;
};

struct script {
	std::string env;
	std::vector<statement> statements;
	
	void compile(FILE * out, const std::string& name, environment& env);

	void initialize() {
		statements.clear();
		env = "std";
	}
};