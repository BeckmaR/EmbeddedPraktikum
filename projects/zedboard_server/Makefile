all: server

server: server.o
	g++ -o server server.o

server.o: server.cpp
	g++ -c server.cpp

.PHONY: clean

clean:
	rm -rf *.o server
