#include "driver.hpp"

void driver::load_standard_environment(environment& env) {
	unsigned i = 0;
	const struct {const char * name; definition def;} stddefs[] = {
		// The purpose of each argument is provided in a comment before the
		// bytecode
		{ "return",               {DEF, i++, {}}},
		{ "yield",                {DEF, i++, {}}},
		// dest
		{ "goto",                 {DEF, i++, {{CON, 2}}}},
		{ "goto_far",             {DEF, i++, {{CON, 3}}}},
		// test, dest
		{ "goto_conditional",     {DEF, i++, {{ARG, 1}, {CON, 2}}}},
		{ "goto_conditional_far", {DEF, i++, {{ARG, 1}, {CON, 3}}}},
		// lhs, rhs, dest
		{ "add",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "sub",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "mul",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "div",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "equ",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "not",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "and",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "or",                   {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},

		{ "add_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "sub_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "mul_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "div_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "equ_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "not_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},

		{ "add_word",             {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "sub_word",             {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "mul_word",             {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "div_word",             {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "equ_word",             {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "not_word",             {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "and_word",             {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "or_word",              {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},

		{ "add_word_const",       {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "sub_word_const",       {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "mul_word_const",       {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "div_word_const",       {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "equ_word_const",       {DEF, i++, {{ARG, 2}, {CON, 2}, {ARG, 2}}}},
		{ "not_word_const",       {DEF, i++, {{ARG, 2}, {CON, 2}, {ARG, 2}}}},

		// dest, source
		{ "copy",                 {DEF, i++, {{ARG, 1}, {ARG, 1}}}},
		{ "load",                 {DEF, i++, {{ARG, 1}, {ARG, 2}}}},
		{ "store",                {DEF, i++, {{ARG, 2}, {ARG, 1}}}},

		{ "copy_const",           {DEF, i++, {{ARG, 1}, {CON, 1}}}},
		{ "load_const",           {DEF, i++, {{ARG, 1}, {CON, 2}}}},
		{ "store_const",          {DEF, i++, {{CON, 2}, {ARG, 1}}}},

		{ "copy_word",            {DEF, i++, {{ARG, 2}, {ARG, 2}}}},
		{ "load_word",            {DEF, i++, {{ARG, 2}, {ARG, 2}}}},
		{ "store_word",           {DEF, i++, {{ARG, 2}, {ARG, 2}}}},

		{ "copy_word_const",      {DEF, i++, {{ARG, 2}, {CON, 2}}}},
		{ "load_word_const",      {DEF, i++, {{ARG, 2}, {CON, 2}}}},
		{ "store_word_const",     {DEF, i++, {{CON, 2}, {ARG, 2}}}},

		{ "cast_8to16",           {DEF, i++, {{ARG, 2}, {ARG, 1}}}},
		{ "cast_16to8",           {DEF, i++, {{ARG, 1}, {ARG, 2}}}},

		// dest
		{ "callasm",              {DEF, i++, {{CON, 2}}}},
		{ "callasm_far",          {DEF, i++, {{CON, 3}}}},
	};

	for (size_t i = 0; i < sizeof(stddefs) / sizeof(*stddefs); i++) {
		env.defines[stddefs[i].name] = stddefs[i].def;
		env.bytecode_count++;
	}

	env.section = "ROMX";
	env.terminator = 0;
	env.pool = 0;
}

int driver::parse(const std::string & f) {
	file = f;
	location.initialize(&file);
	scan_begin();
	yy::parser parser = {*this};
	parser.set_debug_level(trace_parsing);
	int result = parser();
	scan_end();
	return result;
}

void driver::merge(driver& source) {
	typedefs.insert(source.typedefs.begin(), source.typedefs.end());
	environments.insert(source.environments.begin(), source.environments.end());
	scripts.insert(source.scripts.begin(), source.scripts.end());
}