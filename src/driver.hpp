#pragma once

#include <string>
#include <unordered_map>
#include "exception.hpp"
#include "parser.hpp"
#include "types.hpp"

#define YY_DECL yy::parser::symbol_type yylex(driver& drv)
YY_DECL;

struct driver {
	std::string file;
	bool trace_parsing = false;
	bool trace_scanning = false;

	std::unordered_map<std::string, type_definition> typedefs;
	std::unordered_map<std::string, environment> environments;
	std::unordered_map<std::string, script> scripts;

	int result;
	yy::location location;
	environment current_environment;
	script current_script;

	void load_standard_environment(environment& env);
	int parse(const std::string & f);
	void scan_begin();
	void scan_end();

	unsigned get_type(std::string name) {
		return typedefs[name].size;
	}

	void import(std::string env) {
		if (env == "std") {
			load_standard_environment(current_environment);
			return;
		}

		if (!environments.contains(env)) {
			err::warn("Environment {} does not exist", env);
			return;
		}

		for (auto& [name, def] : environments[env].defines) {
			current_environment.defines[name] = def;
			current_environment.bytecode_count++;
		}

		current_environment.pool = environments[env].pool;
		current_environment.section = environments[env].section;
		current_environment.terminator = environments[env].terminator;
		current_environment.bytecode_size = environments[env].bytecode_size;
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
