#include <fmt/format.h>
#include <unordered_set>
#include "exception.hpp"
#include "types.hpp"

struct variable {
	unsigned size = 0;
	bool internal = false;
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
		if (internal) variables[i].name = fmt::format("__evstemp{}", i);
		else variables[i].name = name;
		return i;
	}



	void free(const std::string& name) {
		for (auto& var : variables) if (var.name == name) {
			var.size = 0;
			return;
		}
		err::fatal("No variable named \"{}\"", name);
	}

	// Free a variable only if it is marked as internal.
	void auto_free(const std::string& name) {
		for (auto& var : variables) if (var.name == name) {
			if (!var.internal) return;
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

	variable& required_get(const std::string& name) {
		for (auto& i : variables) {
			if (i.name == name) return i;
		}
		err::fatal("Variable {} not found", name);
	}

	variable& get(int id) {
		return variables[id];
	}

	variable_list(unsigned pool) { variables.resize(pool); }
};

typedef std::vector<std::string> string_table;
typedef std::unordered_set<std::string> label_table;

void script::compile(FILE * out, const std::string& name, environment& env) {
	variable_list varlist {env.pool};
	string_table s_table;
	label_table l_table;

	// This needs to be removed as it cannot support u24s
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

	auto print_label = [&](size_t size, std::string label) {
		switch (size) {

		}
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
				fmt::print(out, "\tdb {} ; {}", var_index, argument.str);
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

	auto auto_cast = [&](variable& dest, variable& source) {
		std::string cast = dest.name;
		if (dest.size != source.size) {
			cast = varlist.get(varlist.alloc(dest.size, true, "")).name;
			print_standard(
				fmt::format("cast_{}to{}", source.size * 8, dest.size * 8),
				{{argtype::VAR, cast}, {argtype::VAR, source.name}}
			);
		}
		return cast;
	};

	auto compile_ASSIGN = [&](const statement& stmt) {
		const char * command_table[] = {
			"copy_const", "copy16_const", "copy24_const", "copy32_const"
		};
		std::vector<arg> args = {
			{argtype::VAR, stmt.identifier}, {argtype::NUM, .value = stmt.value}
		};
		variable * var = varlist.get(stmt.identifier);

		if (!var) err::fatal("{} is not defined", stmt.identifier);
		print_standard(command_table[var->size - 1], args);
	};

	auto compile_DECLARE = [&](const statement& stmt) {
		size_t index = varlist.alloc(stmt.size, false, stmt.identifier);
		fmt::print(out, "\t; Allocated {} at {}\n", stmt.identifier, index);
	};

	auto compile_DECLARE_ASSIGN = [&](const statement& stmt) {
		compile_DECLARE(stmt);
		compile_ASSIGN(stmt);
	};

	auto compile_COPY = [&](const statement& stmt) {
		std::vector<arg> args = {{argtype::VAR, stmt.identifier}, {argtype::VAR, stmt.operand}};
		int iden_index = varlist.lookup(stmt.identifier);
		int oper_index = varlist.lookup(stmt.operand);
		unsigned op_size;
		const char ** command_table;
		// Is the destination declared as a variable?
		if (iden_index != -1 && oper_index != -1) {
			const char * table[] = {
				"copy", "copy16", "copy24", "copy32"
			};
			op_size = varlist.get(iden_index).size;
			command_table = table;
		} else if (iden_index != -1) {
			const char * table[] = {
				"load_const", "load16_const",
				"load24_const", "load32_const"
			};
			op_size = varlist.get(iden_index).size;
			command_table = table;
		} else if (varlist.lookup(stmt.operand) != -1) {
			const char * table[] = {
				"store_const", "store16_const",
				"store24_const", "store32_const"
			};
			op_size = varlist.get(oper_index).size;
			command_table = table;
		} else {
			err::fatal("Cannot copy between two global vars, as no size is known");
		}
		print_standard(command_table[op_size - 1], args);
	};

	auto compile_DECLARE_COPY = [&](const statement& stmt) {
		compile_DECLARE(stmt);
		compile_COPY(stmt);
	};

	auto compile_CALL = [&](const statement& stmt) {
		definition * def = env.get_define(stmt.identifier);
		if (!def) err::fatal("Definition of {} not found", stmt.identifier); 
		print_definition(stmt.identifier, *def, stmt.args);
	};

	auto compile_DROP = [&](const statement& stmt) {
		varlist.free(stmt.identifier);
		fmt::print(out, "\t; Dropped {}", stmt.identifier);
	};

	auto compile_LABEL = [&](const statement& stmt) {
		fmt::print(out, ".{}\n", stmt.identifier);
	};

	auto compile_OPERATION = [&](const statement& stmt) {
		std::vector<arg> args;

		variable& dest = varlist.required_get(stmt.identifier);
		// TODO: This should use a seperate value to allow for a = x + y expressions.
		// Until then, this line is a no-op.
		std::string lhs = auto_cast(dest, varlist.required_get(stmt.identifier));
		bool is_const = stmt.type < ADD;
		std::string rhs;

		if (is_const) {
			args = {
				{argtype::VAR, lhs},
				{argtype::NUM, .value = stmt.value},
				{argtype::VAR, stmt.identifier}
			};
		} else {
			rhs = auto_cast(dest, varlist.required_get(stmt.operand));
			args = {
				{argtype::VAR, lhs},
				{argtype::VAR, rhs},
				{argtype::VAR, stmt.identifier}
			};
		}

		const char * command_base[] = {"add", "sub", "mul", "div", "equ", "not", "and", "or"};
		const char * command_type[] = {"", "16", "24", "32"};
		std::string command = command_base[is_const ? stmt.type - CONST_ADD : stmt.type - ADD];
		command += command_type[dest.size - 1];
		if (is_const) command += "_const";

		print_standard(command, args);

		varlist.auto_free(lhs);
		if (!is_const) varlist.auto_free(rhs);
	};
	if (env.section != "" && env.section != "none") {
		fmt::print(out, "\nSECTION \"{0} evscript section\", {1}\n{0}::\n", name, env.section);
	}
	for (const auto& stmt : statements) {
		if (stmt.type == LABEL) l_table.emplace(stmt.identifier);
	}
	for (const auto& stmt : statements) {
		#define COMPILE(type) case type: compile_##type(stmt); break
		switch (stmt.type) {
			case CONST_ADD: case CONST_SUB: case CONST_MULT: case CONST_DIV:
			case ADD: case SUB: case MULT: case DIV:
				compile_OPERATION(stmt);
				break;
			COMPILE(ASSIGN);
			COMPILE(CALL);
			COMPILE(COPY);
			COMPILE(DECLARE);
			COMPILE(DECLARE_ASSIGN);
			COMPILE(DECLARE_COPY);
			COMPILE(DROP);
			COMPILE(LABEL);
		}
		#undef COMPILE
	}
	print_value(1, env.terminator);
	// Define constant strings
	for (size_t i = 0; i < s_table.size(); i++) {
		fmt::print(out, ".string_table{} db \"{}\", 0\n", i, s_table[i]);
	}
}
