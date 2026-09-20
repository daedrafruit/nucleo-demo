make:
	pio -f -c vim run

run:
	pio -f -c vim run --target upload

clean:
	pio -f -c vim run --target clean

program:
	pio -f -c vim run --target program

uploadfs:
	pio -f -c vim run --target uploadfs

update:
	pio -f -c vim update

db:
	pio run -t compiledb
	cp compile_commands.json ~/.platformio/packages/framework-stm32cubef4

