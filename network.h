#ifndef NETWORK_H
#define NETWORK_H

#define SERVER_PATH "/var/tmp/server_path"

int create_unix_server(const char* path_name);
int server_accept(int server_fd);
#endif