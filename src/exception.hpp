#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

inline bool ansi_exceptions = false;
inline uintmax_t error_count = 0;

#define warn(...) \
	do { \
		if (ansi_exceptions) fputs("\033[1m\033[95mwarn: \033[0m", stderr); \
		else fputs("warn: ", stderr); \
		fprintf(stderr, __VA_ARGS__); \
		fputc('\n', stderr); \
	} while (0)

#define error(...) \
	do { \
		if (ansi_exceptions) fputs("\033[1m\033[31merror: \033[0m", stderr); \
		else fputs("error: ", stderr); \
		fprintf(stderr, __VA_ARGS__); \
		fputc('\n', stderr); \
		error_count++; \
	} while (0)

#define fatal(...) \
	do { \
		if (ansi_exceptions) fputs("\033[1m\033[31mfatal: \033[0m", stderr); \
		else fputs("fatal: ", stderr); \
		fprintf(stderr, __VA_ARGS__); \
		fputc('\n', stderr); \
		exit(1); \
	} while (0)

static inline void errcheck() {
	if (error_count > 0)
		fatal("Failed with %ju error%s.", error_count, error_count != 1 ? "s" : "");
}
