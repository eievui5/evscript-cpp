#include <stdio.h>
#include "exception.hpp"
#include "types.hpp"

struct variable {
	bool used;
	unsigned size;
	bool internal;
	std::string name;
};

class variable_list {
	std::vector<variable> variables;

public:
	size_t alloc(unsigned size, bool internal, const std::string& name) {
		size_t i = 0;
		while (i <= variables.size() - size) {
			if (!variables[i].used) goto found;
			i += variables[i].size;
		}
		fatal("Out of pool space.");

	found:
		variables[i].used = true;
		variables[i].size = size;
		variables[i].internal = internal;
		variables[i].name = name;
		return i;
	}

	void free(const std::string& name) {
		for (auto& var : variables) if (var.name == name) {
			var.used = false;
			return;
		}
		fatal("No variable named \"%s\"", name.c_str());
	}

	variable& lookup(const std::string& name) {
		for (auto& var : variables) if (var.name == name) return var;
		fatal("No variable named \"%s\"", name.c_str());
	}

	variable_list(unsigned pool) {
		variables.resize(pool);
	}
};

typedef std::vector<std::string> string_table;

static inline void fprint_d(FILE * out, size_t size) {
	switch (size) {
	case 1: fputs("\tdb ", out); break;
	case 2: fputs("\tdw ", out); break;
	case 4: fputs("\tdl", out); break;
	default:
		fatal("Cannot output value of size %zu", size);
		break;
	}
}

static inline void fprint_value(FILE * out, size_t size, unsigned value) {
	fprint_d(out, size);
	fprintf(out, "%u\n", value);
}

static void fprint_definition(
	FILE * out, const definition& def, const std::vector<arg>& args,
	string_table& s_table
) {
	if (def.parameters.size() > args.size()) fatal("Not enough arguments.");
	if (def.parameters.size() < args.size())
		warn("%lu excess arguments", args.size() - def.parameters.size());
	fprint_value(out, 1, def.bytecode);
	for (size_t i = 0; i < def.parameters.size(); i++) {
		fprint_d(out, def.parameters[i].size);
		switch (args[i].type) {
		case argtype::VAR:
			fprintf(out, "%s\n", args[i].str.c_str());
			break;
		case argtype::NUM:
			fprintf(out, "%u\n", args[i].value);
			break;
		case argtype::STR:
			fprintf(out, ".string_table%zu\n", s_table.size());
			s_table.push_back(args[i].str);
			break;
		case argtype::ARG:
			fatal("Variable arguments are only allowed in macro definitions");
			break;
		}
	}
}

static void fprint_standard(
	FILE * out, environment& env, const std::string& name,
	const std::vector<arg>& args, string_table& s_table
) {
	definition * def = env.get_define(name);
	if (!def)
		fatal(
			"Definition of %1$s not found.\n"
			"Please `use std;` in your environment or provide an implementation of %1$s",
			name.c_str()
		);
	fprint_definition(out, *def, args, s_table);
}

void script::compile(const std::string& name, environment& env, FILE * out) {
	variable_list varlist {env.pool};
	string_table s_table;

	fprintf(out, "%s::\n", name.c_str());
	for (const auto& stmt : statements) {
		switch (stmt.type) {
		case CALL: {
			definition * def = env.get_define(stmt.identifier);
			if (!def)
				fatal("Definition of %s not found", stmt.identifier.c_str()); 
			fprint_definition(out, *def, stmt.args, s_table);
		}
		case DECLARE:
			varlist.alloc(stmt.size, false, stmt.identifier);
			break;
		case DROP:
			varlist.free(stmt.identifier);
			break;
		}
	}
	fprint_value(out, 1, env.terminator);
	// Define constant strings
	for (size_t i = 0; i < s_table.size(); i++) {
		fprintf(out, ".string_table%zu\n\tdb \"%s\"\n", i, s_table[i].c_str());
	}
}