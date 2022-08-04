rm -r bin/
mkdir bin/
../bin/evscript -o bin/script.asm script.evs
../bin/evscript -l ../examples/example.evslang -o bin/script.s script.evs
rgbgfx -c embedded -o bin/font.2bpp font.png
rgbasm -h -o bin/test.o test.asm
rgblink -n bin/test.sym -m bin/test.map -o bin/test.gb bin/test.o
rgbfix -flhg bin/test.gb
echo test/bin/test.gb
