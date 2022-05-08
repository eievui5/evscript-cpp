printf "Building driver:          %s bytes\n" \
	`rgbasm "-Dswap_bank=ld [$2000], a" -o - driver.asm \
	| rgblink -xo - - \
	| wc -c`
printf "Building bytecode:        %s bytes\n" \
	`rgbasm "-Dswap_bank=ld [$2000], a" -o - evsbytecode.asm \
	| rgblink -xo - - \
	| wc -c`
printf "Building 16-bit bytecode: %s bytes\n" \
	`rgbasm "-Dswap_bank=ld [$2000], a" -o - evsbytecode16.asm \
	| rgblink -xo - - \
	| wc -c`
