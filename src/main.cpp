#include <stdio.h>
#include "driver.hpp"

int main(int argc, char ** argv) {
	int result = 0;
	driver drv;

	for (int i = 1; i < argc; i++) {
		if (!drv.parse(argv[i])) {
			puts("Driver complete");
			puts("Results:");
			for (auto& [name, info] : drv.typedefs)
				info.print(stdout, name);
			for (auto& [name, info] : drv.environments)
				info.print(stdout, name);
			for (auto& [name, info] : drv.scripts)
				info.print(stdout, name);
		} else {
			result = 1;
		}
	}
	
	return result;
}