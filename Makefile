all: other folders client server
client: bin/tracer
server: bin/monitor
other: obj/LinkedList.o
folders:
	@mkdir -p src obj bin tmp out
obj/LinkedList.o: src/LinkedList.c
	gcc -Wall -g -c src/LinkedList.c -o obj/LinkedList.o
bin/tracer: obj/tracer.o
	gcc -g obj/tracer.o -o bin/tracer
-o obj/tracer.o: src/tracer.c
	gcc -Wall -g -c src/tracer.c -o obj/tracer.o
bin/monitor: obj/monitor.o
	gcc -g obj/monitor.o obj/LinkedList.o -o bin/monitor
obj/monitor.o: src/monitor.c
	gcc -Wall -g -c src/monitor.c -o obj/monitor.o
clean:
	rm -f obj/* tmp/* bin/{tracer,monitor} out/*