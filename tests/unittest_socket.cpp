#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <native_netlib.h>
#include <ape_socket.h>

//@TODO: int APE_socket_listen(ape_socket *socket, uint16_t port,
//@TODO: int APE_socket_connect(ape_socket *socket, uint16_t port,
//@TODO: int APE_socket_write(ape_socket *socket, void *data,
//@TODO: int APE_socket_writev(ape_socket *socket, const struct iovec *iov, int iovcnt);
//@TODO: void APE_socket_shutdown(ape_socket *socket);
//@TODO: void APE_socket_shutdown_now(ape_socket *socket);
//@TODO: int APE_sendfile(ape_socket *socket, const char *file);
//@TODO: char *APE_socket_ipv4(ape_socket *socket);
//@TODO: int APE_socket_is_online(ape_socket *socket)
//@TODO: int ape_socket_do_jobs(ape_socket *socket);
//@TODO: int ape_socket_accept(ape_socket *socket);
//@TODO: int ape_socket_read(ape_socket *socket);
//@TODO: int ape_socket_read_udp(ape_socket *socket);
//@TODO: int ape_socket_connected(void *arf);
//@TODO: int ape_socket_write_udp(ape_socket *from, const char *data,
//@TODO: int ape_socket_write_file(ape_socket *socket, const char *file,

TEST(Socket, Simple)
{
	ape_global * g_ape;
	ape_socket * socket;
	int ret;
	int proto;

	proto = APE_SOCKET_PT_TCP;
	g_ape = native_netlib_init();
	ape_running = g_ape->is_running = 0;
	socket = NULL;

	socket = APE_socket_new(proto, 0, g_ape);
	EXPECT_TRUE(socket != NULL);
	EXPECT_EQ(socket->ape, g_ape);
	EXPECT_TRUE(socket->s.fd != 0);
	EXPECT_EQ(socket->s.type, APE_EVENT_SOCKET);
	EXPECT_EQ(socket->states.type, APE_SOCKET_TP_UNKNOWN);
	EXPECT_EQ(socket->states.state, APE_SOCKET_ST_PENDING);
	EXPECT_EQ(socket->states.proto, proto);
	EXPECT_TRUE(socket->ctx == NULL);
	EXPECT_TRUE(socket->parent == NULL);
	EXPECT_TRUE(socket->dns_state == NULL);
	EXPECT_TRUE(socket->callbacks.on_read == NULL);
	EXPECT_TRUE(socket->callbacks.on_disconnect == NULL);
	EXPECT_TRUE(socket->callbacks.on_connect == NULL);
	EXPECT_TRUE(socket->callbacks.on_connected == NULL);
	EXPECT_TRUE(socket->callbacks.on_message == NULL);
	EXPECT_TRUE(socket->callbacks.on_drain == NULL);
	EXPECT_TRUE(socket->callbacks.arg == NULL);
	EXPECT_EQ(socket->remote_port, 0);
	EXPECT_EQ(socket->local_port, 0);
	
	ret = ape_socket_destroy(socket);
	EXPECT_EQ(ret, 0);

	//native_netlib_destroy(g_ape);
}

