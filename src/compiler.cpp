#include <fmt/format.h>
#include <functional>
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
	std::string alloc(unsigned size, bool internal, const std::string& name = "") {
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
		return variables[i].name;
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

	std::function<void(statement&)> compile_statement;
	std::function<void(std::vector<statement>&)> compile_statements;

	// TODO: This needs to be removed as it cannot support u24s
	auto print_d = [&](size_t size) {
		switch (size) {
		case 1:fmt::print(out, "\tdb "); break;
		case 2:fmt::print(out, "\tdw "); break;
		case 4:fmt::print(out, "\tdl "); break;
		default:
			err::fatal("Cannot output value of size {}", size);
			break;
		}
	};

	auto print_value = [&](size_t size, unsigned value) {
		print_d(size);
		fmt::print(out, "{}\n", value);
	};

	auto generate_label = [&](const std::string& l) {
		std::string label = fmt::format("__{}_{}", l, l_table.size());
		l_table.emplace(label);
		return label;
	};

	auto print_label = [&](std::string label) {
		fmt::print(out, ".{}\n", label);
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
			cast = varlist.alloc(dest.size, true);
			print_standard(
				fmt::format("cast_{}to{}", source.size * 8, dest.size * 8),
				{{argtype::VAR, cast}, {argtype::VAR, source.name}}
			);
		}
		return cast;
	};

	auto conditional_operation = [&](statement& stmt) {
		if (stmt.type >= ASSIGN && stmt.type <= DIV) {
			unsigned lhs_size = varlist.required_get(stmt.lhs).size;
			unsigned rhs_size = varlist.required_get(stmt.rhs).size;
			unsigned size = lhs_size > rhs_size ? lhs_size : rhs_size;
			if (stmt.identifier.length() == 0) {
				stmt.identifier = varlist.alloc(size, true);
			}
		} else {
			err::warn("Statement cannot be evaluated as condition");
		}
	};

	auto compile_ASSIGN = [&](statement& stmt) {
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

	auto compile_DECLARE = [&](statement& stmt) {
		varlist.alloc(stmt.size, false, stmt.identifier);
	};

	auto compile_DECLARE_ASSIGN = [&](statement& stmt) {
		compile_DECLARE(stmt);
		compile_ASSIGN(stmt);
	};

	auto compile_COPY = [&](statement& stmt) {
		std::vector<arg> args = {{argtype::VAR, stmt.lhs}, {argtype::VAR, stmt.rhs}};
		int iden_index = varlist.lookup(stmt.lhs);
		int oper_index = varlist.lookup(stmt.rhs);
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
		} else if (oper_index != -1) {
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

	auto compile_DECLARE_COPY = [&](statement& stmt) {
		compile_DECLARE(stmt);
		compile_COPY(stmt);
	};

	auto compile_CALL = [&](statement& stmt) {
		definition * def = env.get_define(stmt.identifier);
		if (!def) err::fatal("Definition of {} not found", stmt.identifier); 
		print_definition(stmt.identifier, *def, stmt.args);
	};

	auto compile_DROP = [&](statement& stmt) {
		varlist.free(stmt.identifier);
		fmt::print(out, "\t; Dropped {}", stmt.identifier);
	};

	auto compile_LABEL = [&](statement& stmt) {
		print_label(stmt.identifier);
	};

	auto compile_GOTO = [&](statement& stmt) {
		print_standard("goto", {{argtype::VAR, stmt.identifier}});
	};

	auto compile_IF = [&](statement& stmt) {
		std::string end_label = generate_label("endif");
		std::string else_label = generate_label("endelse");
		bool has_else = stmt.else_statements.size();

		conditional_operation(stmt.conditions[0]);
		compile_statement(stmt.conditions[0]);
		print_standard("goto_conditional_not", {
			{argtype::VAR, stmt.conditions[0].identifier},
			{argtype::VAR, end_label}
		}); 
		compile_statements(stmt.statements);
		if (has_else) print_standard("goto", {{argtype::VAR, else_label}});
		print_label(end_label);
		if (has_else) {
			compile_statements(stmt.else_statements);
			print_label(else_label);
		}
	};

	auto compile_WHILE = [&](statement& stmt) {
		std::string begin_label = generate_label("beginwhile");
		std::string end_label = generate_label("endwhile");
		std::string cond_label = generate_label("whilecondition");

		// Rather than jumping to the start each iteration to check the
		// condition, jump to the bottom and check it there each iterations
		print_standard("goto", {{argtype::VAR, cond_label}});
		print_label(begin_label);
		compile_statements(stmt.statements);
		print_label(cond_label);
		compile_statement(stmt.conditions[0]);
		print_standard("goto_conditional", {
			{argtype::VAR, stmt.conditions[0].identifier},
			{argtype::VAR, begin_label}
		});
		print_label(end_label);
	};

	auto compile_DO = [&](statement& stmt) {
		std::string begin_label = generate_label("begindo");
		std::string end_label = generate_label("enddo");
		std::string cond_label = generate_label("docondition");

		print_label(begin_label);
		compile_statements(stmt.statements);
		print_label(cond_label);
		compile_statement(stmt.conditions[0]);
		print_standard("goto_conditional", {
			{argtype::VAR, stmt.conditions[0].identifier},
			{argtype::VAR, begin_label}
		});
		print_label(end_label);
	};

	auto compile_FOR = [&](statement& stmt) {
		std::string begin_label = generate_label("beginfor");
		std::string end_label = generate_label("endfor");

		// compile prologue.
		compile_statement(stmt.conditions[0]);
		print_label(begin_label);
		// Condition
		compile_statement(stmt.conditions[1]);
		print_standard("goto_conditional_not", {
			{argtype::VAR, stmt.conditions[1].identifier},
			{argtype::VAR, end_label}
		});

		compile_statements(stmt.statements);
		compile_statement(stmt.conditions[2]);
		print_standard("goto", {{argtype::VAR, begin_label}});

		print_label(end_label);
	};

	auto compile_REPEAT = [&](statement& stmt) {
		if (stmt.value == 0) return;

		std::string begin_label = generate_label("beginrepeat");
		std::string cond_label = generate_label("repeatcondition");
		std::string end_label = generate_label("endrepeat");
		size_t i_size;

		if (stmt.value < 256) i_size = 1;
		else if (stmt.value < 65536) i_size = 2;
		else err::fatal("Repeat loops are limited to 65536 iterations");

		// compile prologue.
		std::string temp_var = varlist.alloc(i_size, true);
		print_standard(
			i_size == 1 ? "copy_const" : fmt::format("copy{}_const", i_size * 8),
			{{argtype::VAR, temp_var}, {argtype::NUM, .value = stmt.value}}
		);
		print_label(begin_label);
		compile_statements(stmt.statements);
		print_label(cond_label);
		print_standard(
			i_size == 1 ? "sub_const" : fmt::format("sub{}_const", i_size * 8),
			{{argtype::VAR, temp_var}, {argtype::NUM, .value = 1}, {argtype::VAR, temp_var}}
		);
		print_standard("goto_conditional", {
			{argtype::VAR, temp_var},
			{argtype::VAR, begin_label}
		});
		print_label(end_label);
	};

	auto compile_LOOP = [&](statement& stmt) {
		std::string begin_label = generate_label("beginloop");
		std::string end_label = generate_label("endloop");
		print_label(begin_label);
		compile_statements(stmt.statements);
		print_standard("goto", {{argtype::VAR, begin_label}});
		print_label(end_label);
	};


	auto compile_OPERATION = [&](statement& stmt) {
		if (stmt.identifier.length() == 0) return;
		std::vector<arg> args;

		variable& dest = varlist.required_get(stmt.identifier);
		std::string lhs = auto_cast(varlist.required_get(stmt.lhs), dest);
		bool is_const = stmt.type < EQU;
		std::string rhs;

		if (is_const) {
			args = {
				{argtype::VAR, lhs},
				{argtype::NUM, .value = stmt.value},
				{argtype::VAR, stmt.identifier}
			};
		} else {
			rhs = auto_cast(varlist.required_get(stmt.rhs), dest);
			args = {
				{argtype::VAR, lhs},
				{argtype::VAR, rhs},
				{argtype::VAR, stmt.identifier}
			};
		}

		const char * command_base[] = {"equ", "not", "lt", "lte", "gt", "gte", "add", "sub", "mul", "div", "equ", "not", "and", "or"};
		const char * command_type[] = {"", "16", "24", "32"};
		std::string command = command_base[is_const ? stmt.type - CONST_EQU : stmt.type - EQU];
		command += command_type[dest.size - 1];
		if (is_const) command += "_const";

		print_standard(command, args);

		varlist.auto_free(lhs);
		if (!is_const) varlist.auto_free(rhs);
	};

	compile_statement = [&](statement& stmt) {
		#define COMPILE(type) case type: compile_##type(stmt); break
		switch (stmt.type) {
			case CONST_EQU: case CONST_NOT: case CONST_LT: case CONST_LTE:
			case CONST_GT: case CONST_GTE:
			case CONST_ADD: case CONST_SUB: case CONST_MULT: case CONST_DIV:
			case EQU: case NOT: case LT: case LTE: case GT: case GTE: case ADD:
			case SUB: case MULT: case DIV:
				compile_OPERATION(stmt);
				break;
			COMPILE(ASSIGN);
			COMPILE(CALL);
			COMPILE(COPY);
			COMPILE(DECLARE);
			COMPILE(DECLARE_ASSIGN);
			COMPILE(DECLARE_COPY);
			COMPILE(DO);
			COMPILE(DROP);
			COMPILE(FOR);
			COMPILE(LABEL);
			COMPILE(LOOP);
			COMPILE(GOTO);
			COMPILE(IF);
			COMPILE(REPEAT);
			COMPILE(WHILE);
		}
		#undef COMPILE
	};

	compile_statements = [&](std::vector<statement>& list) {
		for (auto& stmt : list) {
			if (stmt.type == LABEL) l_table.emplace(stmt.identifier);
		}
		for (auto& stmt : list) {
			compile_statement(stmt);
		}
	};

	if (env.section != "" && env.section != "none") {
		fmt::print(out, "\nSECTION \"{} evscript section\", {}\n", name, env.section);
	}
	fmt::print(out, "{}::\n", name);
	compile_statements(statements);
	print_value(1, env.terminator);
	// Define constant strings
	for (size_t i = 0; i < s_table.size(); i++) {
		fmt::print(out, ".string_table{} db \"{}\", 0\n", i, s_table[i]);
	}
}
