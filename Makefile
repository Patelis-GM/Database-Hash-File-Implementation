ht:
	@echo "Compiling HASH-FILE Main!";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/record.c ./src/ht_table.c -lbf -o ./build/ht_main -O2

sht:
	@echo "Compiling SECONDARY-HASH-FILE Main!";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/sht_main.c ./src/record.c ./src/sht_table.c ./src/ht_table.c -lbf -o ./build/sht_main -O2


st:
	@echo "Compiling STATISTICS Main!";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/st_main.c ./src/record.c ./src/sht_table.c ./src/ht_table.c -lbf -o ./build/st_main -O2