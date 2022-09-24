# EVScript
## An extendable bytecode-based scripting engine

## Dependancies

- [Bison](https://www.gnu.org/software/bison/)
- [Flex](https://github.com/westes/flex)
- [GNU Make 4.3](https://www.gnu.org/software/make/)
- A C++20 compiler

## Building

Navigate to the project root and execute `make`.

## Installing

Navigate to the project root and execute `sudo make install`.

## Roadmap

evscript is lacking a few crucial features before it is production-ready:

1. Return values were omitted for technical reasons (the Game Boy doesn't have enough registers for them) but could be mimicked with syntactical sugar.
2. Complex expressions (anything more than just `x = y + z`).
3. Scope/automatic freeing of locals. `drop` will be removed.
4. Structures.

## Credits

- [{fmt}](https://github.com/fmtlib/fmt) for formatting output.
- [poryscript](https://github.com/huderlem/poryscript) for inspiring this project.
- And everyone at gbdev who helped me along the way :)

## TODO (from testing)
- OOM seems to just return an empty variable name for temps?
- u8 var = CONST fails?
- `drop` is annoying. Add automatic lifetime detection.
