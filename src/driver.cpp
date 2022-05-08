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
		// dest
		{ "callasm",              {DEF, i++, {{CON, 2}}}},
		{ "callasm_far",          {DEF, i++, {{CON, 3}}}},
		// lhs, rhs, dest
		{ "add",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "sub",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "mul",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "div",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "equ",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "not",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "land",                 {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "lor",                  {DEF, i++, {{ARG, 1}, {ARG, 1}, {ARG, 1}}}},
		{ "add_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "sub_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "mul_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "div_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "equ_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		{ "not_const",            {DEF, i++, {{ARG, 1}, {CON, 1}, {ARG, 1}}}},
		// dest, source
		{ "copy",                 {DEF, i++, {{ARG, 1}, {ARG, 1}}}},
		{ "load",                 {DEF, i++, {{ARG, 1}, {ARG, 2}}}},
		{ "store",                {DEF, i++, {{ARG, 2}, {ARG, 1}}}},
		{ "copy_const",           {DEF, i++, {{ARG, 1}, {CON, 1}}}},
		{ "load_const",           {DEF, i++, {{ARG, 1}, {CON, 2}}}},
		{ "store_const",          {DEF, i++, {{CON, 2}, {ARG, 1}}}},
		// lhs, rhs, dest
		{ "add16",                {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "sub16",                {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "mul16",                {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "div16",                {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "equ16",                {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "not16",                {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "land16",               {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "lor16",                {DEF, i++, {{ARG, 2}, {ARG, 2}, {ARG, 2}}}},
		{ "add16_const",          {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "sub16_const",          {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "mul16_const",          {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "div16_const",          {DEF, i++, {{ARG, 2}, {ARG, 2}, {CON, 2}}}},
		{ "equ16_const",          {DEF, i++, {{ARG, 2}, {CON, 2}, {ARG, 2}}}},
		{ "not16_const",          {DEF, i++, {{ARG, 2}, {CON, 2}, {ARG, 2}}}},
		// dest, source
		{ "copy16",               {DEF, i++, {{ARG, 2}, {ARG, 2}}}},
		{ "load16",               {DEF, i++, {{ARG, 2}, {ARG, 2}}}},
		{ "store16",              {DEF, i++, {{ARG, 2}, {ARG, 2}}}},
		{ "copy16_const",         {DEF, i++, {{ARG, 2}, {CON, 2}}}},
		{ "load16_const",         {DEF, i++, {{ARG, 2}, {CON, 2}}}},
		{ "store16_const",        {DEF, i++, {{CON, 2}, {ARG, 2}}}},
		// dest, source
		{ "cast_8to16",           {DEF, i++, {{ARG, 2}, {ARG, 1}}}},
		{ "cast_16to8",           {DEF, i++, {{ARG, 1}, {ARG, 2}}}},
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