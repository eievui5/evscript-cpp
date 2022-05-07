#pragma once

#include <fmt/format.h>
#include <stdint.h>
#include <stdlib.h>

namespace err {

inline bool color = false;
inline uintmax_t count = 0;

template <typename ...Args>
static inline void warn(fmt::string_view message, const Args& ... args) {
	if (color) fmt::print(stderr, "\033[1m\033[95mwarn: \033[0m");
	else fmt::print(stderr, "warn: ");
	fmt::vprint(stderr, message, fmt::make_format_args(args...));
	fmt::print(stderr, "\n");
}

template <typename ...Args>
static inline void error(fmt::string_view message, const Args& ... args) {
	if (color) fmt::print(stderr, "\033[1m\033[31merror: \033[0m");
	else fmt::print(stderr, "error: ");
	fmt::vprint(stderr, message, fmt::make_format_args(args...));
	fmt::print(stderr, "\n");
	count++;
}

template <typename ...Args>
[[noreturn]] static inline void fatal(fmt::string_view message, const Args& ... args) {
	if (color) fmt::print(stderr, "\033[1m\033[31mfatal: \033[0m");
	else fmt::print(stderr, "fatal: ");
	fmt::vprint(stderr, message, fmt::make_format_args(args...));
	fmt::print(stderr, "\n");
	exit(1);
}

static inline void check() {
	if (count > 0) {
		fatal("Failed with {} error{}", count, count != 1 ? "s" : "");
	}
}

}