#pragma once

#include <string>
#include <unordered_map>
#include "parser.hpp"
#include "types.hpp"

#define YY_DECL yy::parser::symbol_type yylex(driver& drv)
YY_DECL;

inline static void load_standard_environment(environment& env) {
	unsigned i = 0;
	const struct {const char * name; definition def;} stddefs[] = {
		// The purpose of each argument is provided in a comment before the
		// bytecode
		{ "return", {DEF, i++, {}}},
		{ "yield",  {DEF, i++, {}}},
		// dest
		{ "goto",     {DEF, i++, {{CON, 2}}}},
		{ "goto_far", {DEF, i++, {{CON, 3}}}},
		// test, dest
		{ "goto_conditional",     {DEF, i++, {{ARG, 1}, {CON, 2}}}},
		{ "goto_conditional_far", {DEF, i++, {{ARG, 1}, {CON, 3}}}},
		// lhs, rhs, dest
		{ "add", {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "sub", {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "mul", {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "div", {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "add_const", {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "sub_const", {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "mul_const", {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "div_const", {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "equ", {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "not", {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "and", {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "or",  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "equ_const", {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "not_const", {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},

		{ "add_word", {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "sub_word", {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "mul_word", {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "div_word", {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "add_word_const", {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "sub_word_const", {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "mul_word_const", {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "div_word_const", {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},

		// { "name", {DEF, i++, {}},
		// { "name", {DEF, i++, {}},
		// { "name", {DEF, i++, {}},
		// { "name", {DEF, i++, {}},
		// { "name", {DEF, i++, {}},
		// { "name", {DEF, i++, {}},
		// { "name", {DEF, i++, {}},
		// { "name", {DEF, i++, {}},
		// { "name", {DEF, i++, {}},
		// { "name", {DEF, i++, {}},
	};

	for (size_t i = 0; i < sizeof(stddefs) / sizeof(*stddefs); i++) {
		env.defines[stddefs[i].name] = stddefs[i].def;
	}

	env.section = "ROMX";
	env.terminator = 0;
	env.pool = 0;
}

struct driver {
	std::string file;
	bool trace_parsing = false;
	bool trace_scanning = false;

	std::unordered_map<std::string, int> variables;
	std::unordered_map<std::string, type_definition> typedefs;
	std::unordered_map<std::string, environment> environments;
	std::unordered_map<std::string, script> scripts;

	int result;
	yy::location location;
	environment current_environment;
	script current_script;

	int parse(const std::string & f);
	void scan_begin();
	void scan_end();

	unsigned get_type(std::string name) {
		return typedefs[name].size;
	}

	driver() {
		load_standard_environment(environments["std"]);

		typedefs["byte"].size = 1;
		typedefs["word"].size = 2;
		typedefs["ptr"].size = 2;
		typedefs["short"].size = 3;
		typedefs["farptr"].size = 3;
		typedefs["long"].size = 4;
	}
};
