rm -r bin/
mkdir bin/
../bin/evscript -o bin/script.asm script.evs
rgbgfx -o bin/font.2bpp font.png
rgbasm -o bin/test.o test.asm
rgblink -n bin/test.sym -m bin/test.map -o bin/test.gb bin/test.o
rgbfix -flhg bin/test.gb
echo bin/test.gb
