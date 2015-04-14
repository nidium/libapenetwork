#CC=gcc
CC=/opt/clang/bin/clang
CFLAGS= -O2 -fvisibility=hidden -c -g -Wall -Wextra 
INC=-isystem../../third-party/c-ares/
AR=ar
ARFLAGS=rcs
EXECUTABLE=libnativenetwork.a
OBJECTS=native_netlib.o ape_pool.o ape_hash.o ape_http_parser.o ape_array.o ape_buffer.o ape_events.o ape_event_kqueue.o ape_event_epoll.o ape_event_select.o ape_events_loop.o ape_socket.o ape_dns.o ape_timers.o ape_timers_next.o ape_base64.o

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS) 

ape_array.o: ape_array.c ape_array.h ape_pool.h ape_buffer.h
ape_base64.o: ape_base64.c ape_base64.h
ape_buffer.o: ape_buffer.c ape_buffer.h common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h
ape_dns.o: ape_dns.c common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h ape_dns.h
ape_event_epoll.o: ape_event_epoll.c common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h ape_socket.h ape_buffer.h ape_pool.h
ape_event_kqueue.o: ape_event_kqueue.c common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h ape_socket.h ape_buffer.h ape_pool.h
ape_events.o: ape_events.c common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h ape_socket.h ape_buffer.h ape_pool.h
ape_event_select.o: ape_event_select.c common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h ape_socket.h ape_buffer.h ape_pool.h
ape_events_loop.o: ape_events_loop.c common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h ape_socket.h ape_buffer.h ape_pool.h ape_timers.h ape_events_loop.h
ape_hash.o: ape_hash.c ape_hash.h
ape_http_parser.o: ape_http_parser.c ape_http_parser.h
ape_pool.o: ape_pool.c ape_pool.h ape_buffer.h
ape_sha1.o: ape_sha1.c ape_sha1.h
ape_socket.o: ape_socket.c ape_socket.h common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h ape_buffer.h ape_pool.h ape_dns.h ape_ssl.h
ape_ssl.o: ape_ssl.c ape_ssl.h
ape_timers.o: ape_timers.c ape_timers.h common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h
ape_timers_next.o: ape_timers_next.c ape_timers_next.h common.h port/POSIX.h ape_events.h ape_hash.h
ape_websocket.o: ape_websocket.c ape_websocket.h common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h ape_socket.h ape_buffer.h ape_pool.h ape_sha1.h ape_base64.h
native_netlib.o: native_netlib.c native_netlib.h common.h port/POSIX.h ape_events.h ape_hash.h ape_timers_next.h ape_socket.h ape_buffer.h ape_pool.h ape_events_loop.h ape_timers.h ape_dns.h ape_ssl.h

.c.o:
	@echo "CC\t"$@
	@$(CC) $(INC) $(CFLAGS) $< -o $@
.PHONY: clean

clean:
	@echo "Cleaning.."
	@rm -f $(OBJECTS) $(EXECUTABLE)
