#include <fmt/format.h>
#include <getopt.h>
#include <stdio.h>
#include "driver.hpp"
#include "exception.hpp"

static bool printed_help = false;

static void print_help(const char * program_name) {
	if (!printed_help) {
		printed_help = true;
		fmt::print(
			"usage: {} -o <outfile> <infile>\n"
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
		if (!outfile) err::fatal("Failed to open {}: {}", path, strerror(errno));
	}
	return outfile;
}

int main(int argc, char ** argv) {
	// If stderr (fd 2) is a terminal, enable colored errors.
	err::color = isatty(2);

	// Options
	FILE * outfile = NULL;

	for (char c; (c = getopt_long_only(argc, argv, shortopts, longopts, NULL)) != -1;) {
		switch (c) {
		case 'h':
			print_help(argv[0]);
			break;
		case 'o':
			if (outfile) {
				err::warn("Multiple output files provided");
				fclose(outfile);
			}
			outfile = fopen_output(optarg);
			break;
		}
	}

	if (argc == optind) err::error("No input file");
	else if (argc != optind + 1) err::error("More than one input file given");
	if (!outfile) err::error("No output file");

	if (err::count > 0) {
		print_help(argv[0]);
		err::check();
	}

	// Parse input file.
	const char * input_path = argv[optind];
	driver drv;
	int result = drv.parse(input_path);
	if (result) {
		err::error("Compilation of {} failed", input_path);
		return result;
	}

	// Compile
	fmt::print(outfile, "; Generated by the evscript bytecode compiler, written by Eievui\n");
	for (auto& [name, script] : drv.scripts) {
		script.compile(outfile, name, drv.environments[script.env]);
	}
}
