BIN := bin/evscript
OBJS := \
	$(patsubst src/%.ypp, obj/%.o, $(shell find src/ -name '*.ypp')) \
	$(patsubst src/%.lpp, obj/%.o, $(shell find src/ -name '*.lpp')) \
	$(patsubst src/%.cpp, obj/%.o, $(shell find src/ -name '*.cpp'))

CXXFLAGS := -Isrc/include -Isrc/ -Iobj/ -std=c++17 -Wno-unused-result -Wno-parentheses -Wno-switch -MD
RELEASEFLAGS := -Ofast -flto
DEBUGFLAGS := -Og -g
TESTFLAGS := examples/spec.evs

CXXFLAGS += $(RELEASEFLAGS)

all:
	$(MAKE) $(BIN)

clean:
	rm -rf bin/ obj/

rebuild:
	$(MAKE) clean
	$(MAKE) all

test: all
	./$(BIN) $(TESTFLAGS)

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
