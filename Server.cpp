#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string>
#include <sstream>

int playerOnline = 0;
bool start = 0;
int ID_player = 1;
const int PORT = 3008;
const int BUFFER_SIZE = 1024;

void setNonBlocking(int sockfd) {
    int nonBlocking = 1;
    if (ioctl(sockfd, FIONBIO, &nonBlocking) != 0) {
        perror("ioctl nonblocking faill");
        exit(EXIT_FAILURE);
    }
}

std::vector<std::string> splitByNewline(const std::string& input) {
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, '\n')) {
        result.push_back(item);
    }

    return result;
}

void handle_request(std::string msg, int sockfd) {
    std::vector<std::string> cmd = splitByNewline(msg);
    for (int e = 0; cmd.size() > e ; e++) {
        if (cmd[e] == "READY") {
            playerOnline++;
            if (playerOnline == 2) start = 1;
            std::cout << "send READY to" << sockfd << std::endl;

        }
        else if (cmd[e] == "ID") {
            char idd[2];
            idd[0] = '0' + ID_player;
            idd[1] = '\n';
            send(sockfd, idd, 2, 0);
            std::cout << "ID clien for " << sockfd << std::endl;
        } else if (cmd[e] == "MAP") {
            send (sockfd, "2 20 20 5\n", 5, 0);
            std::cout << "send MAP ofr : " << sockfd << std::endl;

        }
    }
}

int main() {
    int server_fd, new_socket, activity;
    int opt = 1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    fd_set readfds;
    std::vector<int> clients;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    setNonBlocking(server_fd);

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int client_socket : clients) {
            FD_SET(client_socket, &readfds);
            if (client_socket > max_sd) {
                max_sd = client_socket;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            setNonBlocking(new_socket);
            clients.push_back(new_socket);
            std::cout << "New connection: socket fd is " << new_socket << std::endl;
        }

        for (auto it = clients.begin(); it != clients.end(); ) {
            int client_socket = *it;
            if (FD_ISSET(client_socket, &readfds)) {
                int valread = read(client_socket, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    close(client_socket);
                    it = clients.erase(it);
                    std::cout << "Client disconnected: socket fd is " << client_socket << std::endl;
                } else if (valread > 0) {
                    buffer[valread] = '\0';
                    std::cout << "Received message: " << buffer << std::endl;
                    handle_request(buffer, client_socket);
                    // send(client_socket, buffer, valread, 0);
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    return 0;
}
