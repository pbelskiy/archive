all: bin/usage

obj_dir=@mkdir -p obj

bin/usage: obj/usage.o obj/cgp.o obj/lde.o obj/input.o
	@gcc -o bin/usage obj/usage.o obj/cgp.o obj/lde.o obj/input.o

obj/usage.o: src/usage.c
	$(obj_dir)
	@gcc -c src/usage.c -o obj/usage.o

obj/cgp.o: src/cgp.c src/cgp.c
	$(obj_dir)
	@gcc -std=c99 -c src/cgp.c -o obj/cgp.o

obj/lde.o: src/lde.c
	$(obj_dir)
	@gcc -c src/lde.c -o obj/lde.o

obj/input.o: src/input.s
	@gcc -masm=intel -c src/input.s -o obj/input.o

clean:
	@rm -rf obj \
	@rm -f bin/usage bin/graph_in.gdl bin/graph_out.gdl
	@rm -f bin/graph_in.png bin/graph_out.png
