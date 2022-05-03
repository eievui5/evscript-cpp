#pragma once

#include <string>
#include <unordered_map>
#include "parser.hpp"
#include "types.hpp"

#define YY_DECL yy::parser::symbol_type yylex(driver& drv)
YY_DECL;

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
		typedefs["byte"].size = 1;
		typedefs["word"].size = 2;
		typedefs["ptr"].size = 2;
		typedefs["short"].size = 3;
		typedefs["farptr"].size = 3;
		typedefs["long"].size = 4;
	}
};
