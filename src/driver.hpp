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
	std::vector<std::string> assembly;

	int result;
	yy::location location;

	void load_std(environment& env);
	void load_std16(environment& env);
	int parse(const std::string & f);
	void scan_begin();
	void scan_pause();
	void scan_end();

	unsigned get_type(std::string name) {
		return typedefs[name].size;
	}

	void import(std::string import_name, environment& env) {
		if (!environments.contains(import_name)) {
			err::warn("Environment {} does not exist", import_name);
			return;
		}

		environment& import = environments[import_name];

		for (auto& [name, def] : import.defines) {
			env.defines[name] = def;
			env.bytecode_count++;
		}

		env.pool = import.pool;
		env.section = import.section;
		env.terminator = import.terminator;
	}

	driver() {
		load_std(environments["std"]);
		load_std16(environments["std16"]);

		typedefs["u8"].size = 1;
		typedefs["u16"].size = 2;
		typedefs["u24"].size = 3;
		typedefs["u32"].size = 4;
	}

	void merge(driver&);
};
