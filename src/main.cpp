#include <getopt.h>
#include <stdio.h>
#include "driver.hpp"
#include "exception.hpp"

static bool printed_help = false;

static void print_help(const char * program_name) {
	if (!printed_help) {
		printed_help = true;
		printf(
			"usage: %s -o <outfile> <infile>\n"
			"\t-h --help   Show this message.\n"
			"\t-o --output Path to output file.\n",
			program_name
		);
	}
}

static const char shortopts[] = "ho:";
static struct option const longopts[] = {
	{"help",   no_argument,       NULL, 'h'},
	{"output", required_argument, NULL, 'o'}
};

static FILE * fopen_output(const char * path) {
	FILE * outfile;
	if (path[0] == '-' && path[1] == 0) {
		outfile = stdout;
	} else {
		outfile = fopen(path, "w");
		if (!outfile) fatal("Failed to open %s: %s", path, strerror(errno));
	}
	return outfile;
}

int main(int argc, char ** argv) {
	// Initialize program
	ansi_exceptions = isatty(2); // Check if fd 2 (stderr) is a terminal.

	// Options
	FILE * outfile = NULL;

	for (char c; (c = getopt_long_only(argc, argv, shortopts, longopts, NULL)) != -1;) {
		switch (c) {
		case 'h': print_help(argv[0]); break;
		case 'o':
			if (outfile) warn("multiple output files provided");
			outfile = fopen_output(optarg);
			break;
		}
	}

	// Argument parsing error check.
	if (argc == optind) error("No input file");
	else if (argc != optind + 1) error("More than one input file given");
	if (!outfile) error("No output file");


	if (error_count > 0) {
		print_help(argv[0]);
		errcheck();
	}

	// Parse input file.
	driver drv;

	{
		int result = drv.parse(argv[optind]);
		if (result) return result;
	}

	// Reproduce file after parsing
	// for debugging purposes; commented out of actual builds.
	//for (auto& [name, info] : drv.typedefs)
	//	info.print(stdout, name);
	//for (auto& [name, info] : drv.environments)
	//	info.print(stdout, name);
	//for (auto& [name, info] : drv.scripts)
	//	info.print(stdout, name);


	// Compile
	for (auto& [name, script] : drv.scripts) {
		script.compile(name, drv.environments[script.env], outfile);
	}
}
