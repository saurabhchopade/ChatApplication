#include "server.h"

int Server::getClientSocket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if(setsockopt(server_fd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&opt,sizeof(opt)) < 0) {
		perror("ERROR: setsockopt failed");
        exit(EXIT_FAILURE);
	}

    if(bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR: Socket binding failed");
        exit(EXIT_FAILURE);
    }

     if (listen(server_fd, 10) < 0) {
        perror("ERROR: Socket listening failed");
        exit(EXIT_FAILURE);
	}
    printf("Server Started Listening........\n");

	return server_fd;
}

User* Server::acceptNewConnection(int sock_fd) {
    struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);

    User* user = new User();
	user->address = client_addr;
	user->sock_fd = client_fd;
	user->user_id = user_id++;

	return user;
}