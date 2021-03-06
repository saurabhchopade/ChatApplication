#include "../model/server.h"
#include "../model/onlineUsers.h"
#include "../utility/serverUtility.h"
#include "../DBOperation/DBOperation.h"
#include <vector>
#include <sstream>

#define BUFFER_SIZE 2048
#define AUTHENTICATE "AUTHENTICATE"
#define REGISTER "REGISTER"
#define VALID "0"
#define INVALID "1"

Server server;
DBOperation db;

void sendDataToClient(string message, int sock_fd) {

	if(write(sock_fd, message.c_str(), message.size()) < 0) {
		perror("ERROR: write to descriptor failed");
	}
}

void onlineUsers(string name, int socket) {
	vector<string> onlineUsers = getOnlineUsers(name);
	string users = splitUsers(onlineUsers);

	sendDataToClient(users, socket);
}

void showAllMessages(string reciever_name, string buffer , int socket) {
	vector<string> data = split(buffer, ' ');
	vector<string> messages = db.getUserMessages(data[1], reciever_name);
	string users = splitUsers(messages);

	sendDataToClient(users, socket);
}

void showUnseenMessages(string reciever_name, string buffer , int socket) {
	vector<string> data = split(buffer, ' ');
	vector<string> messages = db.getUnseenMsg(data[1], reciever_name);
	string users = splitUsers(messages);

	sendDataToClient(users, socket);
}


void* handleTCPClient(void* arg) {
    char buffer[BUFFER_SIZE];
	char buffer_to_all[BUFFER_SIZE];
    char message[BUFFER_SIZE];
	char name[32];
    char password[10];
	int exit_flag = 0;
	size_t position;

	User* user = (User*)arg;

    while(1) {

        if(exit_flag) {
            break;
        }
	    if(recv(user->sock_fd, buffer, 32, 0) <= 0 ) {
		    exit_flag = 1;
	    } else {
		    cout << buffer << endl;
	    }

        vector<string> user_data = split(buffer, ' ');

		if(user_data[0] == REGISTER ) {
			if(!db.isUserAlreadyExists(user_data[1])) {
				db.addUserToDB(user_data[1], user_data[2]);
				sendDataToClient(INVALID, user->sock_fd);
			} else {
				sendDataToClient(VALID, user->sock_fd);
			}
		}

        if(user_data[0] == AUTHENTICATE ) {
			if(db.validateUserNameAndPassword(user_data[1], user_data[2])) {
				sendDataToClient(VALID, user->sock_fd);
                user->user_name = user_data[1];
                user->password = user_data[2];
                addUser(user);
				break;
			} else {
				sendDataToClient(INVALID, user->sock_fd);
			}
		}
    }
	bzero(buffer, BUFFER_SIZE);

	while(1) {
		if (exit_flag) {
			break;
		}
		int receive = recv(user->sock_fd, buffer, BUFFER_SIZE, 0);

		if (receive > 0) {
			if(strcmp(buffer, "online") == 0) {
				onlineUsers(user->user_name, user->sock_fd);	
			}
			else if((position = string(buffer).find("allmsg")) == 0) {
				showAllMessages(user->user_name, buffer, user->sock_fd);
			}
			else if((position = string(buffer).find("unseenmsg")) == 0) {
				showUnseenMessages(user->user_name, buffer, user->sock_fd);
			}
			else {
				strcpy(buffer_to_all, user->user_name.c_str());
				strcat(buffer_to_all, " : ");
				strcat(buffer_to_all, buffer);

				if(strlen(buffer) > 0) {
					parseCommand(buffer, message, name);
					bzero(buffer, BUFFER_SIZE);
					strcpy(buffer, user->user_name.c_str());
					strcat(buffer, " : ");
					strcat(buffer, message);

					if(name[0] == ' ') {
						sendMessage(buffer_to_all, user->user_id);
					} else {
						if (isUserOnline(name)) {
							sendMessageToParticularUser(buffer, name);
							db.addMessagesToDB(user->user_name, name, message, "seen");
						} else {
							sendMessageToParticularUser(buffer, name);
							db.addMessagesToDB(user->user_name, name, message, "unseen");
						}
					}
				}
			}
		} else if (receive == 0 || strcmp(buffer, "exit") == 0) {
			sprintf(buffer, "\033[1;41m%s has left\033[0m", user->user_name.c_str());
            cout << buffer << endl;
			sendMessage(buffer, user->user_id);
			exit_flag = 1;
		} else {
			printf("ERROR: -1\n");
			exit_flag = 1;
		}
		bzero(buffer, BUFFER_SIZE);
		bzero(message, BUFFER_SIZE);
	}

	close(user->sock_fd);
    removeUser(user->user_id);
    delete user;
    client_count--;
    pthread_detach(pthread_self());

	return NULL;
}

void startServer() {
    int socket = server.getClientSocket();
    pthread_t thread_id;

    while(true) {
        User* user = server.acceptNewConnection(socket);

        pthread_create(&thread_id, NULL, &handleTCPClient, (void*)user);

        sleep(1);
    }
    close(socket);
}

int main() {
	system("clear");
	startServer();

	return 0;
}