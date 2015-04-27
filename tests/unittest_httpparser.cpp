#include <stdlib.h>
#include <string.h>

#include <gtest/gtest.h>

#include <ape_http_parser.h>
#include <ape_buffer.h>
#include <ape_array.h>

struct HttpClient{
	struct {
		int version;
		http_parser parser;
		http_method_t method;
		buffer *path;
		buffer *body;
		buffer *qs;
		struct {
			ape_array_t *list;
			buffer *tkey;
			buffer *tval;
		} headers;
	} http;
};

void httpclient_destroy(struct HttpClient * client )
{
	buffer_destroy( client->http.path );
	buffer_destroy( client->http.body );
	buffer_destroy( client->http.qs );
	buffer_destroy( client->http.headers.tkey );
	buffer_destroy( client->http.headers.tval );
	ape_array_destroy(client->http.headers.list );
	free( client );
}

struct HttpClient * httpclient_new( )
{
	struct HttpClient * client;

	client = (struct HttpClient*) malloc( sizeof( *client ) );
	client->http.method = HTTP_GET;
	client->http.version = 0;
	client->http.path         = buffer_new(256); memset(client->http.path->data, '\0', 256);
	client->http.body         = buffer_new(256); memset(client->http.body->data, '\0', 256);
	client->http.qs           = buffer_new(256); memset(client->http.qs->data, '\0', 256);
	client->http.headers.tkey = buffer_new(16); memset(client->http.headers.tkey->data, '\0', 16);
	client->http.headers.tval = buffer_new(64); memset(client->http.headers.tval->data, '\0', 64);
	client->http.headers.list = ape_array_new(16);

	return client;
}

static int requestToClient_cb(void **ctx, callback_type type, int value, uint32_t step)
{
	struct HttpClient *client = (struct HttpClient *) ctx[0];
	//ape_global *ape = (ape_global *)ctx[1];

switch(type) {
	case HTTP_METHOD:
		switch(value) {
			case HTTP_GET:
				client->http.method = HTTP_GET;
				break;
			case HTTP_POST:
				client->http.method = HTTP_POST;
				break;
			default:	//FT
				break;
		}
		break;
	case HTTP_PATH_END:
		buffer_append_char(client->http.path, '\0');
		break;
	case HTTP_PATH_CHAR:
		buffer_append_char(client->http.path, (unsigned char)value);
		break;
	case HTTP_VERSION_MAJOR:
		client->http.version = 10 * value;
		break;
	case HTTP_VERSION_MINOR:
		client->http.version += value;
		break;
	case HTTP_HEADER_KEY:
		break;
	case HTTP_HEADER_VAL:
		ape_array_add_b(client->http.headers.list, client->http.headers.tkey, client->http.headers.tval);
		client->http.headers.tkey   = buffer_new(16);
		client->http.headers.tval   = buffer_new(64);
		break;
	case HTTP_CL_VAL:
		client->http.parser.cl = value;
		break;
	case HTTP_HEADER_END:
		break;
	case HTTP_READY:
		buffer_append_char(client->http.body, '\0');
		buffer_destroy(client->http.headers.tkey);
		client->http.headers.tkey = NULL;
		buffer_destroy(client->http.headers.tval);
		client->http.headers.tval = NULL;
		break;
	case HTTP_BODY_CHAR:
		buffer_append_char(client->http.body, (unsigned char)value);
		break;
	case HTTP_PARSE_ERROR:
		break;
	case HTTP_QS_CHAR:
		buffer_append_char(client->http.qs, (unsigned char)value);
		break;
	case HTTP_HEADER_KEYC:
		buffer_append_char(client->http.headers.tkey, (unsigned char)value);
		break;
	case HTTP_HEADER_VALC:
		buffer_append_char(client->http.headers.tval, (unsigned char)value);
		break;
	default:
		break;
	}

	return 1;
}

struct HttpClient * ParseToClient( const unsigned char * httpMsg, HTTP_parser_callback cb, void * data)
{
	struct HttpClient * client;
	size_t length = 0, i;
	struct _http_parser *p;

	client = httpclient_new( );
	p = &client->http.parser;
	HTTP_PARSER_RESET(p);
	p->ctx[0] = client;
	p->ctx[1] = data;
	p->callback = cb;
	length = strlen( ( char *) httpMsg);
	//  @TODO: implement a "duff device" here
	for (i = 0; i < length; i++) {
		if (parse_http_char(p, httpMsg[i]) == 0) {
			printf("fail at %i\n", i);
			break;
		}
	}
	return client;
}

TEST(HttpParser, parseSimpleResponse)
{
	struct HttpClient * clientS;
	const unsigned char httpMsg[] = "POST /blab/la.php?action=just%20do%20it&when=now HTTP/0.9\nContent-Length: 8\n\n%40a%40b";

	clientS = ParseToClient( httpMsg, requestToClient_cb, NULL);
	EXPECT_TRUE(strcmp( (char *) clientS->http.path->data, "/blab/la.php" ) == 0);
	EXPECT_TRUE(strcmp( (char *) clientS->http.body->data, "@a@b" ) == 0);
	EXPECT_TRUE(strcmp( (char *) clientS->http.qs->data, "action=just do it&when=now" ) == 0);
	EXPECT_EQ(clientS->http.version, 9);
	//@FIXME: EXPECT_EQ(clientS->http.parser.cl, 8);
	httpclient_destroy(clientS);
}
TEST(HttpParser, parseSimpleRequest)
{
	struct HttpClient * clientS;
	const unsigned char httpMsg[] = "HTTP/1.1 200 OK\nfoo: bar\nContent-Length: 10\n\n1234567890";

	clientS = ParseToClient( httpMsg, requestToClient_cb, NULL);
	EXPECT_TRUE(strcmp( (char *) clientS->http.body->data, "1234567890" ) == 0);
	EXPECT_EQ(clientS->http.qs->used, 0);
	EXPECT_EQ(clientS->http.version, 11);
	EXPECT_EQ(clientS->http.parser.rcode, 200);
	//@FIXME: EXPECT_EQ(clientS->http.parser.cl, 10);
	httpclient_destroy(clientS);
}
