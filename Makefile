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

DESTINATION := /usr/local/bin

all:
	${MAKE} $(BIN) "CXXFLAGS=$(CXXFLAGS) $(RELEASEFLAGS)"

debug:
	${MAKE} $(BIN) "CXXFLAGS=$(CXXFLAGS) $(DEBUGFLAGS)"

clean:
	rm -rf bin/ obj/

rebuild:
	$(MAKE) clean
	$(MAKE) all

test: all
	cd test/ && ./build.sh
	open test/bin/test.gb

install: all
	install -s -m 755 $(BIN) $(DESTINATION)/evscript

memcheck: all
	valgrind --leak-check=full ./$(BIN) $(TESTFLAGS)

# Compile each source file.
obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

obj/%.o: src/%.ypp
	@mkdir -p $(@D)
	bison -Wcounterexamples -o obj/$*.cpp $<
	$(CXX) $(CXXFLAGS) -c -o $@ obj/$*.cpp

obj/%.o: src/%.lpp
	@mkdir -p $(@D)
	flex -o obj/$*.cpp $<
	$(CXX) $(CXXFLAGS) -c -o $@ obj/$*.cpp

# Link the output binary.
$(BIN): $(OBJS)
	@mkdir -p $(@D)
	printf "const char * version = \"%s\";\n" `git describe --tags --always --dirty` \
	| $(CXX) $(CXXFLAGS) -c -x c++ -o obj/version.o -
	$(CXX) $(CXXFLAGS) -o $@ $^ obj/version.o
