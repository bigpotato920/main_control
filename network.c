#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "network.h"

#define QLEN 1

/**
 * craete a server in unix domain
 * @param  path_name path name of the socket address
 * @return           server socket descriptor or -1 on failure
 */
int create_unix_server(const char* path_name) 
{
	struct sockaddr_un server_addr;
	size_t addr_len;
	int server_fd;

	if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("create socket");
		return -1;
	}
	unlink(path_name);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, path_name);
	addr_len = offsetof(struct sockaddr_un, sun_path) + strlen(path_name);

	if (bind(server_fd, (struct sockaddr*)&server_addr, addr_len) < 0) {
		perror("socket bind");
		return -1;
	}

	if (listen(server_fd, QLEN) < 0) {
		perror("socket listen");
		return -1;
	}

	return server_fd;
}

/**
 * Server accept a connection from the client
 * @param  server_fd server socket descriptor
 * @return           client socket descriptor
 */
int server_accept(int server_fd)
{
	int client_fd;
	socklen_t addr_len;
	struct sockaddr_un client_addr;

	addr_len = sizeof(client_addr);

	if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr
		, &addr_len) )< 0) {
		perror("socket accept");
	}

	return client_fd;
}