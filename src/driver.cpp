#include "driver.hpp"

int driver::parse(const std::string & f) {
	file = f;
	location.initialize(&file);
	scan_begin();
	yy::parser parser = {*this};
	parser.set_debug_level(trace_parsing);
	int result = parser();
	scan_end();
	return result;
}