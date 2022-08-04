#include <stdio.h>
#include "exception.hpp"
#include "langs.hpp"

language rgbasm = {
	.byte = "db",
	.str = "db \"{}\", 0",
	.number = "{}",
	.label = "{}::",
	.local_label = ".{}",
	.section = "SECTION \"{} evscript section\", {}",
	.comment = "; {}",
	.macro_open = "{} ",
	.macro_end = "",
};

std::unordered_map<std::string, language *> language_lookup = {
	{"rgbasm", &rgbasm},
};

language lang = rgbasm;

void readlang(std::string path) {
	FILE * langfile = fopen(path.c_str(), "r");
	char * line = NULL;
	size_t line_length = 0;
	while (getdelim(&line, &line_length, ',', langfile), !feof(langfile)) {
		size_t key_terminator = 0;
		for (; key_terminator < line_length; key_terminator++) {
			if (line[key_terminator] == ':') {
				line[key_terminator] = 0;
				break;
			}
		}
		for (size_t value_term = key_terminator; value_term < line_length; value_term++) {
			if (line[value_term] == ',') {
				line[value_term] = 0;
				break;
			}
		}
		if (key_terminator >= line_length - 1) {
			err::warn("Malformed language file. Line: {}", line);
			continue;
		}
		std::string key = line;
		std::string value = line + key_terminator + 1;
		if (key == "byte") lang.byte = value;
		else if (key == "str") lang.str = value;
		else if (key == "number") lang.number = value;
		else if (key == "label") lang.label = value;
		else if (key == "local_label") lang.local_label = value;
		else if (key == "section") lang.section = value;
		else if (key == "comment") lang.comment = value;
		else if (key == "macro_open") lang.macro_open = value;
		else if (key == "macro_end") lang.macro_end = value;
		else err::warn("{} is an invalid language spec", key);
	}
}
