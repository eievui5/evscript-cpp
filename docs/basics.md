# Basics of evscript

evscript is a bytecode-based scripting language made for the Nintendo Game Boy.
It's used for expressing complex logic while only having a small ROM footprint.
evscript is also fully re-entrant, meaning scripts can be used as coroutines which greatly simplifies many tasks, such as cutscenes, animations, and even actor logic.

evscript statements are very basic and usually map to just one instruction when compiled.
It currently does not support complex expressions, like `x = y * 200 + 5 - z`.
Instead, these must be written out line-by-line, with each line being one operation:

```c
x = y * 200;
x += 5;
x -= z;
```

While this may seem tedious, being able to use dedicated operators already a big improvment over the usual macro-based approach.

evscript also improves over macros with its support for control structures.
Rather than managing comparisons and jumps by hand, the compiler can manage these for you:

```c
if x == 1 {
	function();
}
```

These can be nested of course, and there are a few different structures available:

```c
loop {
	repeat 4 {
		move_forward();
	}
	turn_right();
}
```

Finally, evscript's re-entrant design makes it ideal for events, like NPC dialogue.
This is facilitated using `yield`, which effectively just exits script execution.
The script can be re-executed at a later point, for example on the next frame or after a previous event completes.

```c
repeat 8 {
	move_down();
}
say("Hello!");
wait(120);
say("Goodbye!");
repeat 8 {
	move_up();
}
```

The above snippet also highlights another strength of evscript: inline strings.
In macro-based scripts, you usually have to define a string seperately from where it's used, like this:

```c
Script:
    say .string
.string
    db "Hello!", 0
```

evscript can automatically handle this for you, making writing dialogue much more natural.
