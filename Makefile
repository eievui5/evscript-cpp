BIN := bin/evscript
OBJS := \
	$(patsubst src/%.ypp, obj/%.o, $(shell find src/ -name '*.ypp')) \
	$(patsubst src/%.lpp, obj/%.o, $(shell find src/ -name '*.lpp')) \
	$(patsubst src/%.cpp, obj/%.o, $(shell find src/ -name '*.cpp'))

CXXFLAGS := \
	-std=c++20 -MD \
	-Isrc/include -Isrc/ -Iobj/ -Isrc/libs \
	-Wno-unused-result -Wno-parentheses -Wno-switch 
RELEASEFLAGS := -Ofast -flto
DEBUGFLAGS := -Og -g
TESTFLAGS := -o bin/out.asm examples/spec.evs

CXXFLAGS += $(DEBUGFLAGS)

all:
	$(MAKE) $(BIN)

clean:
	rm -rf bin/ obj/

rebuild:
	$(MAKE) clean
	$(MAKE) all

test: all
	cd test/ && ./build.sh
	open test/bin/test.gb

memcheck: all
	valgrind --leak-check=full ./$(BIN) $(TESTFLAGS)

# Compile each source file.
obj/%.o: *%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj/%.cpp: *%.ypp
	@mkdir -p $(@D)
	bison -Wcounterexamples -o $@ $<

obj/%.cpp: *%.lpp
	@mkdir -p $(@D)
	flex -o $@ $<

# Link the output binary.
$(BIN): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^
