#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/resource.h>

#include <native_netlib.h>
#include <ape_array.h>
#include <ape_http_parser.h>

#define APE_CLIENT(socket) ((ape_client *)socket->_ctx)
#define REQUEST_HEADER(header) ape_array_lookup(client->http.headers.list, CONST_STR_LEN(header))

#define PORT 8080
#define IP "127.0.0.1"
#define MAX_FDS 4000

int ape_running;
ape_global * g_ape;

struct HttpServer{
	ape_socket *socket;
	uint16_t port;
	char * ip;
};

static void signal_handler(int signal)
{
	if (signal == SIGINT || signal == SIGTERM) {
		ape_running = 0;
	}
}


typedef struct _ape_client {
	char                       ip[16];
	ape_socket *               socket;
	ape_socket *               server;
	struct {
		http_parser            parser;
		http_method_t          method;
		buffer *               path;
		buffer *               qs;
		struct {
			ape_array_t *      list;
			buffer *           tkey;
			buffer *           tval;
		}                      headers;
	}                          http;
} ape_client;

static int ape_server_http_ready(ape_client *client, ape_global *ape);

static int ape_http_callback(void **ctx, callback_type type, int value, uint32_t step) {
	ape_client *client = (ape_client *)ctx[0];
	ape_global *ape = (ape_global *)ctx[1];

	switch(type) {
	case HTTP_METHOD:
		switch(value) {
			default:  //  FT
			case HTTP_GET:
				client->http.method = HTTP_GET;
				break;
			case HTTP_POST:
				client->http.method = HTTP_POST;
				break;
		}
		client->http.headers.list = ape_array_new(16);

		client->http.path		  = buffer_new(256); 
		memset(client->http.path->data, '\0', 256);

		client->http.headers.tkey = buffer_new(16);
		memset(client->http.headers.tkey->data, '\0', 16);

		client->http.headers.tval = buffer_new(64);
		memset(client->http.headers.tval->data, '\0', 64);
		break;
	case HTTP_PATH_CHAR:
		buffer_append_char(client->http.path, (unsigned char) value);
		break;
	case HTTP_QS_CHAR:
		if (client->http.method == HTTP_GET) {
			//  @TODO: bufferize
		}
		break;
	case HTTP_HEADER_KEYC:
		buffer_append_char(client->http.headers.tkey, (unsigned char) value);
		break;
	case HTTP_HEADER_VALC:
		buffer_append_char(client->http.headers.tval, (unsigned char) value);
		break;
	case HTTP_BODY_CHAR:
		if (client->http.method == HTTP_POST) {
		//  @TODO: bufferize
		}
		break;
	case HTTP_PATH_END:
		buffer_append_char(client->http.path, '\0');
		break;
	case HTTP_VERSION_MINOR:
		/* fall through */
	case HTTP_VERSION_MAJOR:
		break;
	case HTTP_HEADER_KEY:
		break;
	case HTTP_HEADER_VAL:
		ape_array_add_b(client->http.headers.list, client->http.headers.tkey, client->http.headers.tval);

		client->http.headers.tkey = buffer_new(16);
		memset(client->http.headers.tkey->data, '\0', 16);

		client->http.headers.tval = buffer_new(64);
		memset(client->http.headers.tval->data, '\0', 64);
		break;
	case HTTP_CL_VAL:
		break;
	case HTTP_HEADER_END:
		//break;
	case HTTP_READY:
		buffer_destroy(client->http.headers.tkey); client->http.headers.tkey = NULL;
		buffer_destroy(client->http.headers.tval); client->http.headers.tval = NULL;
		ape_server_http_ready(client, ape);
		break;
	case HTTP_PARSE_ERROR:  	//  FT
	default:
		break;
	}
	return 1;
}

static int ape_server_http_ready(ape_client *client, ape_global *ape)
{
	const buffer *connection;
	struct HttpServer *server;

	server = (struct HttpServer*) client->server;
	//APE_socket_write(client->socket, (char *) CONST_STR_LEN("HTTP/1.1 200 OK\nServer: libNative\nContent-Type: text/html; charset=UTF-8\nConnection: keep-alive;\nContent-Length: 644\n\n<!DOCTYPE html><html><head><meta charset='UTF-8'><title>LibApeNetwork</title><style type='text/css'>body{background-color:#D3D3EE;text-align:center;float:left;} .gim{border-radius:2px 4px 2px;padding:10px;margin:15px;} .title{margin:0px auto;background-color:#A2A2CC;color:#D3D3EE;} .button{background-color:#D3D3EE;color:#A2A2CC;width:30%;}</style></head>\n<body><h1 class='title gim'>LibApeNetwork</h1><div><a class='button gim' href='http://www.ape-project.org'>Where we came from</a><a class='button gim' href='http://www.nidium.com'>Where we will go to</a></div><h2 class='title gim'>Async, non blocking, fast, robust, ..</h2></body></html>"), APE_DATA_STATIC);
	APE_socket_write(client->socket, (char *) CONST_STR_LEN("HTTP/1.1 200 OK\nConnection: keep-alive\n\n"), APE_DATA_STATIC);
	
	connection = REQUEST_HEADER("Connection");
	if (connection != NULL && strcmp((char*)connection->data, "Close") == 0) {
		APE_socket_shutdown(client->socket);
	} else {
		HTTP_PARSER_RESET(&client->http.parser);
		client->http.parser.callback = ape_http_callback;
		client->http.parser.ctx[0]   = client;
		client->http.parser.ctx[1]   = ape;
		client->http.method          = HTTP_GET;
	}
	ape_array_destroy(client->http.headers.list); client->http.headers.list = NULL;
	buffer_destroy(client->http.headers.tkey); client->http.headers.tkey = NULL;
	buffer_destroy(client->http.headers.tval); client->http.headers.tval = NULL;
	buffer_destroy(client->http.path);	client->http.path = NULL;
	
	return 0;
}
static void OnConnect(ape_socket *socket_server, ape_socket *socket_client, ape_global *ape, void *arg)
{
	ape_client *client;

	client = (ape_client*)malloc(sizeof(*client));
	client->socket  = socket_client;
	client->server  = socket_server;
	socket_client->_ctx = (void *) client; /* link the socket to the client struct */
	HTTP_PARSER_RESET(&client->http.parser);
	client->http.parser.callback = ape_http_callback;
	client->http.parser.ctx[0]   = client;
	client->http.parser.ctx[1]   = ape;
	client->http.method          = HTTP_GET;
	client->http.path            = NULL;
	client->http.headers.list    = NULL;
	client->http.headers.tkey = NULL;
	client->http.headers.tval = NULL;
}
static void OnRead(ape_socket *socket_client, ape_global *ape, void *arg)
{
	size_t i;
	struct _http_parser *httpParser;

	httpParser = &APE_CLIENT(socket_client)->http.parser;
	for (i = 0; i < socket_client->data_in.used; i++) {
		if (!parse_http_char(httpParser, socket_client->data_in.data[i])) {
			shutdown(socket_client->s.fd, SHUT_RDWR);
			break;
		}
		if (socket_client->states.state != APE_SOCKET_ST_ONLINE) {
			break;
		}
	}
}

static void OnDisconnect(ape_socket *socket_client, ape_global *ape, void *arg)
{
	ape_client *client;

	client = APE_CLIENT(socket_client);
	if (client) {
		buffer_destroy(client->http.path); client->http.path = NULL;
		ape_array_destroy(client->http.headers.list);
		buffer_destroy(client->http.headers.tkey); client->http.headers.tkey = NULL;
		buffer_destroy(client->http.headers.tval); client->http.headers.tval = NULL;
		ape_dispatch_async(free, socket_client->_ctx);
		APE_socket_shutdown_now(socket_client);
		free(client);client = NULL;
	}
}

struct HttpServer *HttpServer_New(const uint16_t port, const char * ip)
{
	struct HttpServer *server;

	server = NULL;
	if (port == 0 || ip == NULL) {
		return NULL;
	}
	server = malloc(sizeof(*server));
	server->port = port;
	server->ip = strdup(ip);
	server->socket = APE_socket_new(APE_SOCKET_PT_TCP, 0, g_ape);
	server->socket->callbacks.on_connect = OnConnect;
	server->socket->callbacks.on_read = OnRead;
	server->socket->callbacks.on_disconnect = OnDisconnect;
	server->socket->ctx = (void*) server;

	return server;
}

void HttpServer_Run(struct HttpServer *server)
{
	printf("Listening as %s:%d\n", server->ip, server->port);
	APE_socket_listen(server->socket, server->port, server->ip, 1, 1);
	events_loop(g_ape);
}

void HttpServer_Delete(struct HttpServer * server)
{
	APE_socket_shutdown_now(server->socket);
	//ape_socket_destroy(server->socket); server->socket = NULL;
	free(server->ip); server->ip = NULL;
	server->port = 0;
	free(server); server = NULL;
}
static void daemonize()
{
	struct rlimit limit;
	int fds;

	fds = MAX_FDS;
	limit.rlim_cur = (rlim_t) fds;
	limit.rlim_max = (rlim_t) fds;
	setrlimit(RLIMIT_NOFILE, &limit);
	signal(SIGINT, &signal_handler);
	signal(SIGTERM, &signal_handler);
}

int main(const int argc, const char **argv)
{
	struct HttpServer *server;

	g_ape = native_netlib_init();
	g_ape->is_running = ape_running = 1;

	server = NULL;
	if (argc == 1) {
		server = HttpServer_New(PORT, IP);
	} else if (argc == 2) {
		server = HttpServer_New(atoi(argv[1]), IP);
	} else if (argc == 3) {
		server = HttpServer_New(atoi(argv[1]), argv[2]);
	} else {
		printf("Usage: %s [port] [ip]\n\tdefault port: %d\n\tdefault ip: %s\n", argv[0], PORT, IP);
		exit(1);
	}

	if (server) {
		daemonize();
		HttpServer_Run(server);
		
		HttpServer_Delete(server); server = NULL;
	}

	//native_netlib_destroy(g_ape);

	return 0;
}
