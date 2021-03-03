server: socket_server
	./socket_server

client: socket_client
	./socket_client

socket_server: socket_server.cpp
	g++ -o socket_server -pthread socket_server.cpp common.cpp

socket_client: socket_client.cpp
	g++ -o socket_client -pthread socket_client.cpp common.cpp
