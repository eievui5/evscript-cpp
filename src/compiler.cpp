#include <fmt/format.h>
#include <unordered_set>
#include "exception.hpp"
#include "types.hpp"

struct variable {
	unsigned size = 0;
	bool internal;
	std::string name;
};

class variable_list {
	std::vector<variable> variables;

public:
	size_t alloc(unsigned size, bool internal, const std::string& name) {
		size_t i = 0;
		while (i <= variables.size() - size) {
			for (size_t j = i; j < i + size; j++) {
				if (variables[i].size) goto next;
			}
			goto found;
		next:
			i += variables[i].size;
		}
		err::fatal("Out of pool space.");

	found:
		variables[i].size = size;
		variables[i].internal = internal;
		variables[i].name = name;
		return i;
	}

	void free(const std::string& name) {
		for (auto& var : variables) if (var.name == name) {
			var.size = 0;
			return;
		}
		err::fatal("No variable named \"{}\"", name);
	}

	int lookup(const std::string& name) {
		for (size_t i = 0; i < variables.size(); i++) {
			if (variables[i].name == name) return i;
		}
		return -1;
	}

	variable * get(const std::string& name) {
		for (auto& i : variables) {
			if (i.name == name) return &i;
		}
		return nullptr;
	}

	variable_list(unsigned pool) { variables.resize(pool); }
};

typedef std::vector<std::string> string_table;
typedef std::unordered_set<std::string> label_table;

void script::compile(FILE * out, const std::string& name, environment& env) {
	variable_list varlist {env.pool};
	string_table s_table;
	label_table l_table;

	auto print_d = [&](size_t size) {
		switch (size) {
		case 1: fmt::print(out, "\tdb "); break;
		case 2: fmt::print(out, "\tdw "); break;
		case 4: fmt::print(out, "\tdl "); break;
		default:
			err::fatal("Cannot output value of size {}", size);
			break;
		}
	};

	auto print_value = [&](size_t size, unsigned value) {
		print_d(size);
		fmt::print(out, "{}\n", value);
	};

	auto print_argument = [&](const arg& argument) {
		switch (argument.type) {
		case argtype::VAR: {
			int var_index = varlist.lookup(argument.str);
			if (var_index != -1) {
				fmt::print(out, "{}", var_index);
			} else {
				if (l_table.contains(argument.str)) fmt::print(out, ".");
				fmt::print(out, "{}", argument.str);
			}
		} break;
		case argtype::NUM:
			fmt::print(out, "{}", argument.value);
			break;
		case argtype::STR:
			fmt::print(out, ".string_table{}", s_table.size());
			s_table.push_back(argument.str);
			break;
		case argtype::ARG:
			err::fatal("Variable arguments are only allowed in macro definitions");
			break;
		}
	};

	auto print_argument_d = [&](size_t size, const arg& argument) {
		if (argument.type == argtype::VAR) {
			int var_index = varlist.lookup(argument.str);
			if (var_index != -1) {
				fmt::print(out, "\tdb {}", var_index);
				return;
			}
		}

		print_d(size);
		print_argument(argument);
	};

	auto print_definition = [&](const std::string& name, const definition& def, const std::vector<arg>& args) {
		fmt::print(out, "\t; {}\n", name);

		switch (def.type) {
		case DEF: {
			if (def.parameters.size() > args.size()) {
				err::fatal("Not enough arguments to {}.", name);
			} else {
				int dif = args.size() - def.parameters.size();
				if (dif > 0) {
					err::warn("{} excess argument{} to {}", dif, dif == 1 ? "" : "s", name);
				}
			}
			print_value(1, def.bytecode);
			for (size_t i = 0; i < def.parameters.size(); i++) {
				print_argument_d(def.parameters[i].size, args[i]);
				fmt::print(out, "\n");
			}
		} break;
		case MAC: {
			definition& source_def = env.defines[def.alias];
			print_value(1, source_def.bytecode);
			for (size_t i = 0; i < source_def.parameters.size(); i++) {
				const arg& macarg = def.arguments[i];
				switch (macarg.type) {
				case argtype::STR:
					fmt::print(out, "\tdb \"{}\"", macarg.str);
					break;
				case argtype::ARG:
					print_argument_d(source_def.parameters[i].size, args[macarg.value - 1]);
					break;
				default:
					print_argument_d(source_def.parameters[i].size, macarg);
					break;
				}
				fmt::print(out, "\n");
			}
		} break;
		case ALIAS: {
			fmt::print(out, "\t{} ", def.alias);
			size_t i = 0;
			for (; i < def.parameters.size(); i++) {
				if (def.parameters[i].type == VARARGS) break;
				print_argument(args[i]);
				fmt::print(out, ", ");
			}
			for (; i < args.size(); i++) {
				// Special case for string literals
				if (args[i].type == argtype::STR) {
					fmt::print(out, "\"{}\"", args[i].str);
				} else {
					print_argument(args[i]);
				}
				fmt::print(out, ", ");
			}
			fmt::print(out, "\n");
		} break;
		}
	};

	auto print_standard = [&](const std::string& name, const std::vector<arg>& args) {
		definition * def = env.get_define(name);
		if (!def)
			err::fatal(
				"Definition of {0} not found.\n"
				"Please `use std;` in your environment or provide an implementation of {0}",
				name
			);
		print_definition(name, *def, args);
	};

	fmt::print(out, "SECTION \"{0} evscript section\", {1}\n{0}::\n", name, env.section);
	for (const auto& stmt : statements) {
		if (stmt.type == LABEL) l_table.emplace(stmt.identifier);
	}
	for (const auto& stmt : statements) {
		// ASSIGN, CONST_ADD, CONST_SUB, CONST_MULT, CONST_DIV, COPY, ADD, SUB, MULT, DIV
		switch (stmt.type) {
		case DECLARE_ASSIGN:
			fmt::print(out,
				"\t; Allocated {} at {}\n",
				stmt.identifier,
				varlist.alloc(stmt.size, false, stmt.identifier)
			);
		case ASSIGN: {
			std::vector<arg> args = {{argtype::VAR, stmt.identifier}, {argtype::NUM, .value=stmt.value}};
			variable * var = varlist.get(stmt.identifier);
			if (!var) err::fatal("{} is not defined", stmt.identifier);
			switch (var->size) {
			case 1:
				print_standard("copy_const", args);
				break;
			case 2:
				print_standard("copy_word_const", args);
				break;
			case 3: case 4:
				err::fatal("Variables of size {} are not yet supported", var->size);
			default:
				err::fatal("Variables of size {} are not supported", var->size);
			}
		} break;
		case DECLARE_COPY:
			fmt::print(out,
				"\t; Allocated {} at {}\n",
				stmt.identifier,
				varlist.alloc(stmt.size, false, stmt.identifier)
			);
		case COPY: {
			// TODO: This currently only works for global load/stores
			std::vector<arg> args = {{argtype::VAR, stmt.identifier}, {argtype::VAR, stmt.operand}};
			// Is the destination declared as a variable?
			if (varlist.lookup(stmt.identifier) != -1 && varlist.lookup(stmt.operand) != -1) {
				// Check its size
				unsigned size = varlist.get(stmt.identifier)->size;
				switch (size) {
				case 1:
					print_standard("copy", args);
					break;
				case 2:
					print_standard("copy_word", args);
					break;
				case 3: case 4:
					err::fatal("Variables of size {} are not yet supported", size);
				default:
					err::fatal("Variables of size {} are not supported", size);
				}
			} else if (varlist.lookup(stmt.identifier) != -1) {
				// Check its size
				unsigned size = varlist.get(stmt.identifier)->size;
				switch (size) {
				case 1:
					print_standard("load_const", args);
					break;
				case 2:
					print_standard("load_word_const", args);
					break;
				case 3: case 4:
					err::fatal("Variables of size {} are not yet supported", size);
				default:
					err::fatal("Variables of size {} are not supported", size);
				}
			} else if (varlist.lookup(stmt.operand) != -1) {
				// Check its size
				unsigned size = varlist.get(stmt.operand)->size;
				switch (size) {
				case 1:
					print_standard("store_const", args);
					break;
				case 2:
					print_standard("store_word_const", args);
					break;
				case 3: case 4:
					err::fatal("Variables of size {} are not yet supported", size);
				default:
					err::fatal("Variables of size {} are not supported", size);
				}
			} else {
				err::fatal("Cannot copy between two global vars, as no size is known");
			}
		} break;
		case CALL: {
			definition * def = env.get_define(stmt.identifier);
			if (!def)
				err::fatal("Definition of {} not found", stmt.identifier); 
			print_definition(stmt.identifier, *def, stmt.args);
			break;
		}
		case DECLARE:
			fmt::print(out,
				"\t; Allocated {} at {}\n",
				stmt.identifier,
				varlist.alloc(stmt.size, false, stmt.identifier)
			);
			break;
		case DROP:
			fmt::print(out, "\t; Dropped {}", stmt.identifier);
			varlist.free(stmt.identifier);
			break;
		case LABEL:
			fmt::print(out, ".{}\n", stmt.identifier);
			break;
		}
	}
	print_value(1, env.terminator);
	// Define constant strings
	for (size_t i = 0; i < s_table.size(); i++) {
		fmt::print(out, ".string_table{} db \"{}\", 0\n", i, s_table[i]);
	}
}
