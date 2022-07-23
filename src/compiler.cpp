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

struct variable_list {
	std::vector<variable> variables;

	std::string alloc(unsigned size, bool internal, const std::string& name = "") {
		if (variables.size() == 0) {
			err::fatal("Cannot allocate memory, no pool is defined.");
		}
	
		size_t i = 0;
	retry:
		while (i <= variables.size() - size) {
			for (size_t j = i; j < i + size; j++) {
				if (variables[i].size) {
					i += variables[i].size;
					goto retry;
				}
			}
			// if we make it here, create a variable.
			variables[i].size = size;
			variables[i].internal = internal;
			if (internal) variables[i].name = fmt::format("__evstemp{}", i);
			else variables[i].name = name;
			return variables[i].name;
		}
		// In this case, show the user what is using up memory.
		std::string contents;
		for (auto& i : variables) {
			contents += fmt::format(
				"{}: size {}{}\n",
				i.name, i.size, i.internal ? "(internal)" : ""
			);
		}
		err::fatal("Out of pool space.\nActive variables:\n{}", contents);
	}

	// Free a variable.
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

	// Find the index of a variable.
	int lookup(const std::string& name) {
		for (size_t i = 0; i < variables.size(); i++) {
			if (variables[i].name == name) return i;
		}
		return -1;
	}

	// Get a variable by name. Returns a nullptr if the variable does not
	// exist.
	variable * get(const std::string& name) {
		for (auto& i : variables) {
			if (i.name == name) return &i;
		}
		return nullptr;
	}

	// Get a variable using its ID; always succeeds.
	variable& get(int id) {
		return variables[id];
	}

	// Get a variable by name. Throws a fatal error if the variable does not
	// exist.
	variable& required_get(const std::string& name) {
		for (auto& i : variables) {
			if (i.name == name) return i;
		}
		err::fatal("Variable {} not found", name);
	}

	variable_list(unsigned pool) { variables.resize(pool); }
};

// A list of constant strings found in the script.
// These will be placed after the script is compiled.
typedef std::vector<std::string> string_table;
// A list of previously defined labels. This tells the script when to append a .
// to a label name, as RGBASM's local labels are defined using a .
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

	// TODO: This needs to be removed as print_d cannot support u24s
	auto print_value = [&](size_t size, unsigned value) {
		print_d(size);
		fmt::print(out, "{}\n", value);
	};

	// Generates a name for an internal label used by the compiler.
	auto generate_label = [&](const std::string& l) {
		std::string label = fmt::format("__{}_{}", l, l_table.size());
		l_table.emplace(label);
		return label;
	};

	// Prints a label, appending a dot.
	auto print_label = [&](std::string label) {
		fmt::print(out, ".{}\n", label);
	};

	// Outputs the value of an argument to a function.
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
		case argtype::CON:
			fmt::print(out, "{}", argument.str);
			break;
		case argtype::STR:
			fmt::print(out, ".string_table{}", s_table.size());
			s_table.push_back(argument.str);
			break;
		case argtype::ARG:
			err::fatal("Reordered arguments are only allowed in macro definitions");
			break;
		}
	};

	// TODO: This needs to be removed as print_d cannot support u24s
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
					err::warn("{} excess argument{} to {}", dif, "s"[dif == 1], name);
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
			// I came up with this little hack and I'm very proud of it.
			// So here's a comment proclaiming such.
			// I'll leave figuring out how the condition works as a challenge to the reader.
			if (def.parameters.size()) do {
				if (def.parameters[i].type == VARARGS) break;
				print_argument(args[i++]);
			} while (i < def.parameters.size() && (fmt::print(out, ", "), 1));

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

	// Prints a function defined by the standard set of bytecode, printing
	// a unique message if it doesn't exist.
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

	// Automatically cast a variable and return the new name only if needed.
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

	// Convert an operation to be used as a conditional by assigning a unique
	// destination.
	auto conditional_operation = [&](statement& stmt) {
		if (stmt.type >= ASSIGN && stmt.type <= DIV) {
			if (stmt.identifier.length() == 0) {
				unsigned lhs_size = varlist.required_get(stmt.lhs).size;
				unsigned rhs_size = 0;
				if (stmt.rhs.length()) {
					variable * rhs_variable = varlist.get(stmt.rhs);
					if (rhs_variable) {
						rhs_size = rhs_variable->size;
					}
				}
				stmt.identifier = varlist.alloc(
					lhs_size > rhs_size ? lhs_size : rhs_size, true
				);
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
			{argtype::VAR, stmt.identifier}, {argtype::NUM, "", stmt.value}
		};
		variable& var = varlist.required_get(stmt.identifier);
		print_standard(command_table[var.size - 1], args);
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
		variable * destination = varlist.get(stmt.lhs);
		variable * source = varlist.get(stmt.rhs);
		std::string command;

		if (destination && source) {
			const char * table[] = {
				"copy", "copy16", "copy24", "copy32"
			};
			command = table[destination->size - 1];
		} else if (destination) {
			const char * table[] = {
				"load_const", "load16_const", "load24_const", "load32_const"
			};
			command = table[destination->size - 1];
		} else if (source) {
			const char * table[] = {
				"store_const", "store16_const", "store24_const", "store32_const"
			};
			command = table[source->size - 1];
		} else {
			err::fatal("Cannot copy between two global vars, as no size is known");
		}
		print_standard(command, args);
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
	};

	auto compile_LABEL = [&](statement& stmt) {
		print_label(stmt.identifier);
	};

	auto compile_GOTO = [&](statement& stmt) {
		print_standard("goto", {{argtype::VAR, stmt.identifier}});
	};

	auto compile_IF = [&](statement& stmt) {
		std::string end_label = generate_label("endif");

		// Convert and compile the conditional, then insert a jump for
		// when it is false.
		conditional_operation(stmt.conditions[0]);
		compile_statement(stmt.conditions[0]);
		print_standard("goto_conditional_not", {
			{argtype::VAR, stmt.conditions[0].identifier},
			{argtype::VAR, end_label}
		});

		// Compile the block of statements to be executed when the
		// condition is true.
		compile_statements(stmt.statements);

		// An else block has extra stipulations. If an else block is
		// present, compile it.
		if (stmt.else_statements.size()) {
			std::string else_label = generate_label("endelse");
			// Insert a jump to skip the else block when the
			// condition is true.
			print_standard("goto", {{argtype::VAR, else_label}});
			print_label(end_label);
			compile_statements(stmt.else_statements);
			print_label(else_label);
		} else {
			print_label(end_label);			
		}

		// Free any temporary variables generated for the condition.
		varlist.auto_free(stmt.conditions[0].identifier);
	};

	auto compile_WHILE = [&](statement& stmt) {
		std::string begin_label = generate_label("beginwhile");
		std::string end_label = generate_label("endwhile");
		std::string cond_label = generate_label("whilecondition");

		// Rather than jumping to the start each iteration to check the
		// condition, jump to the bottom and check it there each iterations
		print_standard("goto", {{argtype::VAR, cond_label}});
		print_label(begin_label);

		// Compile the main block of statements.
		compile_statements(stmt.statements);

		print_label(cond_label);
		// Convert and compile the conditional, then insert a jump for
		// when it is true.
		conditional_operation(stmt.conditions[0]);
		compile_statement(stmt.conditions[0]);
		print_standard("goto_conditional", {
			{argtype::VAR, stmt.conditions[0].identifier},
			{argtype::VAR, begin_label}
		});
		print_label(end_label);

		// Free any temporary variables generated for the condition.
		varlist.auto_free(stmt.conditions[0].identifier);
	};

	auto compile_DO = [&](statement& stmt) {
		std::string begin_label = generate_label("begindo");
		std::string end_label = generate_label("enddo");
		std::string cond_label = generate_label("docondition");

		print_label(begin_label);
		compile_statements(stmt.statements);
		print_label(cond_label);
		conditional_operation(stmt.conditions[0]);
		compile_statement(stmt.conditions[0]);
		print_standard("goto_conditional", {
			{argtype::VAR, stmt.conditions[0].identifier},
			{argtype::VAR, begin_label}
		});
		print_label(end_label);

		// Free any temporary variables generated for the condition.
		varlist.auto_free(stmt.conditions[0].identifier);
	};

	auto compile_FOR = [&](statement& stmt) {
		std::string begin_label = generate_label("beginfor");
		std::string end_label = generate_label("endfor");

		// Compile prologue to initialize the for loop.
		compile_statement(stmt.conditions[0]);

		print_label(begin_label);
		// Convert and compile the conditional, then insert a jump for
		// when it is false.
		conditional_operation(stmt.conditions[1]);
		compile_statement(stmt.conditions[1]);
		print_standard("goto_conditional_not", {
			{argtype::VAR, stmt.conditions[1].identifier},
			{argtype::VAR, end_label}
		});

		// Compile the main block of statements.
		compile_statements(stmt.statements);

		// Compile the epilogue, and then jump back to the condition.
		compile_statement(stmt.conditions[2]);
		print_standard("goto", {{argtype::VAR, begin_label}});

		print_label(end_label);

		// Free any temporary variables generated for the condition.
		varlist.auto_free(stmt.conditions[1].identifier);
	};

	auto compile_REPEAT = [&](statement& stmt) {
		// Special-case a loop that occurs 0 times.
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
			{{argtype::VAR, temp_var}, {argtype::NUM, "", stmt.value}}
		);

		print_label(begin_label);
		compile_statements(stmt.statements);

		print_label(cond_label);
		print_standard(
			i_size == 1 ? "sub_const" : fmt::format("sub{}_const", i_size * 8),
			{{argtype::VAR, temp_var}, {argtype::NUM, "", 1}, {argtype::VAR, temp_var}}
		);
		print_standard("goto_conditional", {
			{argtype::VAR, temp_var},
			{argtype::VAR, begin_label}
		});

		print_label(end_label);

		// Free the temporary counter variable.
		varlist.free(temp_var);
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
				{argtype::NUM, "", stmt.value},
				{argtype::VAR, stmt.identifier}
			};
		} else {
			variable * rhs_variable = varlist.get(stmt.rhs);
			if (rhs_variable) {
				rhs = auto_cast(*rhs_variable, dest);
				args = {
					{argtype::VAR, lhs},
					{argtype::VAR, rhs},
					{argtype::VAR, stmt.identifier}
				};
			} else {
				stmt.type -= EQU - CONST_EQU;
				is_const = true;
				args = {
					{argtype::VAR, lhs},
					{argtype::CON, stmt.rhs},
					{argtype::VAR, stmt.identifier}
				};
			}
		}

		const char * command_base[] = {"equ", "not", "lt", "lte", "gt", "gte", "add", "sub", "mul", "div", "band", "bor", "equ", "not", "and", "or"};
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
			case CONST_GT: case CONST_GTE: case CONST_ADD: case CONST_SUB:
			case CONST_MULT: case CONST_DIV: case CONST_BAND: case CONST_BOR:
			case EQU: case NOT: case LT: case LTE: case GT: case GTE: case ADD:
			case SUB: case MULT: case DIV: case BAND: case BOR:
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
			COMPILE(GOTO);
			COMPILE(LABEL);
			COMPILE(LOOP);
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

	// Special values to disable section creation.
	if (env.section != "" && env.section != "none") {
		fmt::print(out, "\nSECTION \"{} evscript section\", {}\n", name, env.section);
	}
	// Compile the contents of the script.
	fmt::print(out, "{}::\n", name);
	compile_statements(statements);
	// Print a terminator if the user has specified one.
	if (env.terminator >= 0) print_value(1, env.terminator);
	// Define constant strings
	for (size_t i = 0; i < s_table.size(); i++) {
		fmt::print(out, ".string_table{} db \"{}\", 0\n", i, s_table[i]);
	}
}
