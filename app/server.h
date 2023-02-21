#ifndef CMUS_SERVER_H
#define CMUS_SERVER_H

#include "list.h"

struct client {
	struct list_head node;
	int fd;
	unsigned int authenticated : 1;
};

extern int gServerSocket;
extern struct list_head client_head;

void server_init(char *address);
void server_exit(void);
void server_accept(void);
void server_serve(struct client *client);

#endif
