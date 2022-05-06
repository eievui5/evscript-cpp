#pragma once

#include <stdio.h>
#include <fmt/format.h>
#include <string>
#include <vector>
#include <unordered_map>

enum deftype { DEF, MAC, ALIAS };
enum partype { ARG, CON, VARARGS };
enum class argtype { VAR, NUM, STR, ARG };

static inline const char * default_type(unsigned size) {
	switch (size) {
		default: return "byte";
		case 2: return "word";
		case 3: return "short";
		case 4: return "long";
	}
}

struct type_definition {
	unsigned size = 1;
	bool big_endian = false;

	void print(FILE * out, const std::string& name) {
		fmt::print(out,
			"typedef{} {} = {};\n",
			big_endian ? "_big" : "", name, size
		);
	}
};

struct param {
	partype type;
	unsigned size;

	void print(FILE * out) {
		switch (type) {
			case partype::ARG: fputs(default_type(size), out); break;
			case partype::CON: fmt::print(out, "const {}", default_type(size)); break;
			case partype::VARARGS: fputs("...", out); break;
		}
	}
};

struct arg {
	argtype type;
	std::string str;
	unsigned value;

	void print(FILE * out) {
		switch (type) {
			case argtype::VAR: fputs(str.c_str(), out); break;
			case argtype::NUM: fmt::print(out, "{}", value); break;
			case argtype::STR: fmt::print(out, "\"{}\"", str); break;
			case argtype::ARG: fmt::print(out, "${}", value); break;
		}
	}
};

struct definition {
	deftype type;
	unsigned bytecode;
	std::vector<param> parameters;
	std::string alias;
	std::vector<arg> arguments;

	void print(FILE * out, const std::string& name) {
		fmt::print(out,
			"\t{} {}(",
			type == deftype::DEF ? "def" : "mac",
			name
		);
		if (parameters.size()) {
			parameters[0].print(out);
			for (size_t i = 1; i < parameters.size(); i++) {
				fputs(", ", out);
				parameters[i].print(out);
			}
		}
		fputc(')', out);
		if (type != deftype::DEF) {
			fmt::print(out, " = {}", alias);
			if (type == deftype::MAC) {
				fputc('(', out);
				if (arguments.size()) {
					arguments[0].print(out);
					for (size_t i = 1; i < arguments.size(); i++) {
						fputs(", ", out);
						arguments[i].print(out);
					}
				}
				fputc(')', out);
			}
		}
		fputs(";\n", out);
	}
};

struct environment {
	std::unordered_map<std::string, definition> defines;
	std::string section = "ROMX";
	unsigned terminator = 0;
	unsigned pool = 0;
	unsigned bytecode_size = 1;
	unsigned bytecode_count = 0;

	void initialize() {
		defines.clear();
		section = "ROMX";
		terminator = 0;
		pool = 0;
		bytecode_size = 1;
		bytecode_count = 0;
	}

	void print(FILE * out, const std::string name) {
		fmt::print(out, "env {} ", name);
		if (bytecode_size != 1) fmt::print(out, "{} ", default_type(bytecode_size));
		fputs("{\n", out);
		for (auto& [def, def_info] : defines)
			def_info.print(out, def);
		fmt::print(out, "\tterminator = {};\n", terminator);
		fmt::print(out, "\tsection = {};\n", section);
		fmt::print(out, "\tpool = {};\n}\n", pool);
	}

	definition * get_define(std::string name) {
		if (!defines.contains(name)) return nullptr;
		return &defines[name];
	}
};

enum statement_type {
	ASSIGN, CONST_ADD, CONST_SUB, CONST_MULT, CONST_DIV,
	COPY, ADD, SUB, MULT, DIV,
	DECLARE, DROP, DECLARE_ASSIGN, DECLARE_COPY, LABEL, CALL
};

struct statement {
	statement_type type;
	std::string identifier;
	std::vector<arg> args;
	std::string operand;
	unsigned value;
	unsigned size;

	void print(FILE * out) {
		switch (type) {
		case ASSIGN: case CONST_ADD: case CONST_SUB: case CONST_MULT: case CONST_DIV:
			fmt::print(out, "\t{} ", identifier);
			switch (type) {
			case CONST_ADD: fputc('+', out); break;
			case CONST_SUB: fputc('-', out); break;
			case CONST_MULT: fputc('*', out); break;
			case CONST_DIV: fputc('/', out); break;
			}
			fmt::print(out, "= {};\n", value);
			break;
		case COPY: case ADD: case SUB: case MULT: case DIV:
			fmt::print(out, "\t{} ", identifier);
			switch (type) {
			case ADD: fputc('+', out); break;
			case SUB: fputc('-', out); break;
			case MULT: fputc('*', out); break;
			case DIV: fputc('/', out); break;
			}
			fmt::print(out, "= {};\n", operand);
			break;
		case DECLARE:
			fmt::print(out, "\t{} {};\n", default_type(size), identifier);
			break;
		case DECLARE_ASSIGN: case DECLARE_COPY:
			fmt::print(out, "\t{} {} = ", default_type(size), identifier);
			if (type == DECLARE_ASSIGN) {
				fmt::print(out, "{};\n", value);
			} else {
				fmt::print(out, "{};\n", operand);
			}
			break;
		case LABEL:
			fmt::print(out, "{}:\n", identifier);
			break;
		case CALL:
			fmt::print(out, "\t{}(", identifier);
			if (args.size()) {
				args[0].print(out);
				for (size_t i = 1; i < args.size(); i++) {
					fputs(", ", out);
					args[i].print(out);
				}
			}
			fputs(");\n", out);
			break;
		case DROP:
			fmt::print(out, "drop {};\n", identifier);
			break;
		}
	}
};

struct script {
	std::string env;
	std::vector<statement> statements;
	
	void compile(FILE * out, const std::string& name, environment& env);
	void print(FILE * out, const std::string& name) {
		fmt::print(out, "{} {} {\n", env, name);
		for (auto& statement : statements)
			statement.print(out);
		fputs("}\n", out);
	}
};