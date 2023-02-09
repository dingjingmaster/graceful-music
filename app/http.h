#ifndef CMUS_HTTP_H
#define CMUS_HTTP_H

#include "keyval.h"

#include <stddef.h> /* size_t */

/*
 * 1xx indicates an informational message only
 * 2xx indicates success of some kind
 * 3xx redirects the client to another URL
 * 4xx indicates an error on the client's part
 * 5xx indicates an error on the server's part
 */

struct http_uri {
	char *uri;
	char *user;
	char *pass;
	char *host;
	char *path;
	int port;
};

struct http_get {
	struct http_uri uri;
	struct http_uri *proxy;
	int fd;
	struct keyval *headers;
	char *reason;
	int code;
};

int http_parse_uri(const char *uri, struct http_uri *u);

/* frees contents of @u, not @u itself */
void http_free_uri(struct http_uri *u);

int http_open(struct http_get *hg, int timeout_ms);

/*
 * returns:  0 success
 *          -1 check errno
 *          -2 parse error
 */
int http_get(struct http_get *hg, struct keyval *headers, int timeout_ms);
void http_get_free(struct http_get *hg);

char *http_read_body(int fd, size_t *size, int timeout_ms);
char *base64_encode(const char *str);

#endif
