# EVScript
## An extendable bytecode-based scripting engine

## Dependancies

- [Bison](https://www.gnu.org/software/bison/)
- [Flex](https://github.com/westes/flex)
- [GNU Make 4.3](https://www.gnu.org/software/make/)
- A C++20 compiler

## Building

Navigate to the project root and execute `make`.

## Credits

- [{fmt}](https://github.com/fmtlib/fmt) for formatting output.
- [poryscript](https://github.com/huderlem/poryscript) for inspiring this project.
- And everyone at gbdev who helped me along the way :)

## TODO (from testing)
- A pool of 0 segfaults. Give an error message instead.
- OOM seems to just return an empty variable name for temps?
- FOR doesn't properly generate temp vars
- asm {} is required to use constants; plz implement.
