run:
	clear
	c++ -std=c++17 irc.cpp Client.cpp helpers.cpp src/*.cpp world_parser.cpp npcs.cpp -o server.out json_parser.cpp
	c++ -std=c++17 client_main.cpp -o client.out
	./server.out

serv:
	clear
	c++ -std=c++17 -Wall -Wextra -Werror irc.cpp Client.cpp helpers.cpp -o server.out
	c++ -std=c++17 -Wall -Wextra -Werror client_main.cpp -o client.out
	./server.out