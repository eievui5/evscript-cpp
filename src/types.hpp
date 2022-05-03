#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <unordered_map>

enum class deftype { DEF, MAC, ALIAS };
enum class partype { ARG, CON, VARARGS };
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
		fprintf(out,
			"typedef%s %s = %u;\n",
			big_endian ? "_big" : "", name.c_str(), size
		);
	}
};

struct param {
	partype type;
	unsigned size;
};

struct arg {
	argtype type;
	std::string str;
	unsigned value;

	void print(FILE * out) {
		switch (type) {
			case argtype::VAR: fputs(str.c_str(), out); break;
			case argtype::NUM: fprintf(out, "%u", value); break;
			case argtype::STR: fprintf(out, "\"%s\"", str.c_str()); break;
			case argtype::ARG: fprintf(out, "$%u", value); break;
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
		fprintf(out,
			"\t%s %s(",
			type == deftype::DEF ? "def" : "mac",
			name.c_str()
		);
		if (parameters.size()) {
			fprintf(out, "%s", default_type(parameters[0].size));
			for (size_t i = 1; i < parameters.size(); i++) {
				fprintf(out, ", %s", default_type(parameters[i].size));
			}
		}
		fputc(')', out);
		if (type != deftype::DEF) {
			fprintf(out, " = %s", alias.c_str());
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
		fprintf(out, "env %s ", name.c_str());
		if (bytecode_size != 1) fprintf(out, "%s ", default_type(bytecode_size));
		fputs("{\n", out);
		for (auto& [def, def_info] : defines)
			def_info.print(out, def);
		fprintf(out, "\tterminator = %u;\n", terminator);
		fprintf(out, "\tsection = %s;\n", section.c_str());
		fprintf(out, "\tpool = %u;\n}\n", pool);
	}
};

enum statement_type {
	ASSIGN, CONST_ADD, CONST_SUB, CONST_MULT, CONST_DIV,
	COPY, ADD, SUB, MULT, DIV,
	DECLARE, DECLARE_ASSIGN, DECLARE_COPY, LABEL, CALL
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
			fprintf(out, "\t%s ", identifier.c_str());
			switch (type) {
			case CONST_ADD: fputc('+', out); break;
			case CONST_SUB: fputc('-', out); break;
			case CONST_MULT: fputc('*', out); break;
			case CONST_DIV: fputc('/', out); break;
			}
			fprintf(out, "= %u;\n", value);
			break;
		case COPY: case ADD: case SUB: case MULT: case DIV:
			fprintf(out, "\t%s ", identifier.c_str());
			switch (type) {
			case ADD: fputc('+', out); break;
			case SUB: fputc('-', out); break;
			case MULT: fputc('*', out); break;
			case DIV: fputc('/', out); break;
			}
			fprintf(out, "= %s;\n", operand.c_str());
			break;
		case DECLARE:
			fprintf(out, "\t%s %s;\n", default_type(size), identifier.c_str());
			break;
		case DECLARE_ASSIGN: case DECLARE_COPY:
			fprintf(out, "\t%s %s = ", default_type(size), identifier.c_str());
			if (type == DECLARE_ASSIGN) {
				fprintf(out, "%u;\n", value);
			} else {
				fprintf(out, "%s;\n", operand.c_str());
			}
			break;
		case LABEL:
			fprintf(out, "%s:\n", identifier.c_str());
			break;
		case CALL:
			fprintf(out, "\t%s(", identifier.c_str());
			if (args.size()) {
				args[0].print(out);
				for (size_t i = 1; i < args.size(); i++) {
					fputs(", ", out);
					args[i].print(out);
				}
			}
			fputs(");\n", out);
			break;
		}
	}
};

struct script {
	std::string env;
	std::vector<statement> statements;

	void print(FILE * out, const std::string& name) {
		fprintf(out, "%s %s {\n", env.c_str(), name.c_str());
		for (auto& statement : statements)
			statement.print(out);
		fputs("}\n", out);
	}
};