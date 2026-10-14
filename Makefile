
FLAGS = -Wall -Werror -Wextra -std=c++17

run:
	clear
	c++ $(FLAGS) irc.cpp Client.cpp helpers.cpp src/*.cpp world_parser.cpp npcs.cpp -o server.out json_parser.cpp
	c++ $(FLAGS) client_main.cpp -o client.out
	./server.out

server:
	clear
	c++ $(FLAGS) irc.cpp Client.cpp helpers.cpp src/*.cpp world_parser.cpp npcs.cpp -o server.out json_parser.cpp
	./server.out

client:
	clear
	c++ $(FLAGS) client_main.cpp -o client.out
	./client.out

client-gui:
	clear
	cd gui && rm -rf build && mkdir build && cd build && cmake .. && cmake --build . && ./RPGClient

clean:
	rm -f *.out
	rm -rf gui/build