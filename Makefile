include_dirs = -Iinc -I../Sha1Algorithm/inc -I../BaseEncode/inc
CPPFLAGS = $(include_dirs) -std=c++14 -g -lpthread

all: WebSocketTest run

out/sha1algorithm.o:
	cd ../Sha1Algorithm/ && $(MAKE) out/sha1algorithm.o

out/baseencode.o:
	cd ../BaseEncode/ && $(MAKE) out/baseencode.o

out/websocket.o: src/WebSocket.cpp inc/WebSocket.hpp
	mkdir -p out
	g++ $(CPPFLAGS) -c src/WebSocket.cpp -o out/websocket.o

WebSocketTest: out/websocket.o out/sha1algorithm.o out/baseencode.o
	g++ $(CPPFLAGS) out/websocket.o ../Sha1Algorithm/out/sha1algorithm.o ../BaseEncode/out/baseencode.o tests/WebSocketTest.cpp -o out/test

run: out/test
	./out/test

clean: 
	rm -rf out/