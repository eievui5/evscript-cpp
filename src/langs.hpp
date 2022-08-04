#include <string>
#include <unordered_map>

struct language {
	std::string byte;
	std::string str; // {1}: text
	std::string number; // {1}: number
	std::string label; // {1}: label name
	std::string export_label; // {1}: label name
	std::string local_label; // {1}: label name
	std::string section; // {1}: name, {2}: optional type
	std::string comment; // {1}: comment text
	std::string macro_open; // {1}: macro name
	std::string macro_end;
};

extern language lang;
extern std::unordered_map<std::string, language *> language_lookup;

void readlang(std::string path);
