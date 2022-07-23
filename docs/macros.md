# evscript macros

Macros in evscript are defined using the `mac` keyword, and are split into two types.

## Alias macros

The first type of macro is used to define an alias to another command.
This can be used to provide a more consise name for commands in certain environments.

For example:
```c
env script {
	use std;
	def npc_move(const u16, const u16);
}

env npc {
	use script;
	mac move(const u16, const u16) = npc_move($1, $2);
}
```

You may notice that the macro's arguments are ordered using ${number}.
This is used in case the order of arguments is potentially confusing because of the underlying implementation, for example if the Y coordinate must be placed before X.

```c
mac move(const u16, const u16) = npc_move($2, $1);
```

You can also provide a *default value* for one of the arguments.

```c
mac move_x(const u16) = npc($1, 0); // Notice that the constant 0 lacks a $.
```

# Assembly macros

The second type of macro is used to take advantage of the target assembler's macro support. This allows you to extend evscript using your assembler's capabilities.

evscript provides constants in the form `{environment}\_{command name}\_BYTECODE` which are crucial for writing macros.

The following macro allows you to write "player" as an alias for a value of 0.

```c
MACRO npc_move
	db script__npc_move_BYTECODE
	// Check if the argument is a string.
	IF STRSUB("\1", 1, 1) == "\""
		IF !STRCMP(\1, "player")
			db 0
		ELSE
			FAIL "Unrecongized alias \"\1\": possible values are: player"
		END
	ELSE
		db \1
	ENDC
	dw \2, \3
ENDM
```

Include this assembly code in evscript with `include asm "{name}";`.

Then you can define the command used by the macro, and an alias invoking it.
Note the the original command, `_npc_move`, does NOT invoke the macro, but it is necessary for defining the `script__npc_move_BYTECODE` constant.

To tell evscript that this macro is for assembly code, omit the parentheses after `_npc_move`.

```c
include asm "macro.inc"

env script {
	def _npc_move(const u8, const u16, const u16);
	mac npc_move(...) = _npc_move;
}
```

You may notice that this macro also uses `...` (variable arguments) rather than writing out its arguments. Since assembler macros may support a variable number of arguments, evscript allows you to pass in as many as you like.

Note that strings passed as variable arguments are passed as-is, and are NOT automatically placed in ROM by evscript. If the normal behavior is desired the string must be passed as a normal argument. Keep in mind that variable arguments must be last.

```c
// This macro creates a string and passes a pointer to it.
mac print(const u16, ...) = print;
// This macro does not create a string and passes it directly to the macro.
mac print(...) = print;
```
