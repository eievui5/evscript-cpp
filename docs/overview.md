# evscript overview

## Environments

To begin writing a script, an *environment* must be provided, which provides
information about the context in which the script will run. This includes
which functions are available, how much space is provided for local variables,
and which section the script will be created in, for example. An environments
may also import functions from other environments, most notably to include the
standard functions which the language uses for operations and control flow.

Once an environment has been defined, a script can be written within it, where
the user may write their program.

### Defining an environment

An environment is defined using the `env` keyword, followed by an identifier and
a pair of brackets which will contain the environment's declarations.

```
env script {}
```

### use

Import another environment, including its functions, pool size, section info,
and terminator byte. Any functions imported are assigned bytecode sequentially,
preventing conflicts.

The `std` environment is automatically provided by the compiler and contains the
functions needed for control flow and 8-bit operations. `std16` provides 16-bit
operations.

```rs
env script {
	use std;
}
```

### def

Define a function with a given name and parameters. Its corresponding
bytecode is automatically determined by the order of definitions. Make sure the
script driver's jump table matches this order.

Parameters should be provided for the function, which may be either local
variables, or constant values or strings.

```c
env script {
	def print(const ptr);
}
```

### mac

Define either an alias to a function, or an RGBASM macro.

When used as an alias, arguments may be reordered or given default values. This
can be used to hide any unsightly implementation details, giving the programmer
a cleaner interface.

```c
env script {
	def move_actor(const ptr, const u8, const u8);
	mac move_player(const u8, const u8) = move_enemy(wPlayer, $1, $2);
}
```

When used as an RGBASM macro, varargs may be provided. However, arguments cannot
be given default values or reordered, as this should be done within the macro
itself.

```c
env script {
	def list(...) = list;
}
```

### pool

Define the number of bytes avaiable for allocating local variables.

```c
env script {
	pool = 16;
}
```

### section

Define the section info of the generated script.

```c
env script {
	section = "ROMX";
}
```

### terminator

Define a byte to automatically insert at the end of a script.

```c
env script {
	terminator = 0;
}
```
