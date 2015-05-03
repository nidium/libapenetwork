CC=clang
VALGRIND=valgrind --leak-check=full # --show-reachable=yes

CC_FLAGS=-Wall -Wextra -g -O3 -D_HAVE_SSL_SUPPORT=1
LD_FLAGS=-g -lstdc++ -lpthread -lz -lrt -O3

GTEST_DIR=../../../third-party/.default/angle/third_party/googletest
INCLUDES=-I.. -isystem../../../third-party/.default/c-ares -isystem $(GTEST_DIR)/include/
GTEST_LIB=$(GTEST_DIR)/libgtest.a $(GTEST_DIR)/libgtest_main.a
LIBNATIVENETWORK=../libnativenetwork.a
LIBS=-L.. -lnativenetwork -L../../../third-party/c-ares/.libs -lcares -lssl -lcrypto

TEST_OBJECTS= \
	unittest_0.o \
	unittest_common.o \
	unittest_hash.o \
	unittest_sha1.o \
	unittest_buffer.o \
	unittest_pool.o \
	unittest_array.o \
	unittest_socket.o \
	unittest_dns.o \
	unittest_ssl.o \
	unittest_events.o \
	unittest_netlib.o \
	unittest_httpparser.o \
	unittest_websocket.o \
	unittest_timersng.o \
	unittest_timers.o \

TEST_EXEC=run_unittests
BENCHMARKS = run_benchmark_new_pool run_benchmark_http_server run_benchmark_timers_next

all: $(TEST_EXEC)

benchmarks: $(BENCHMARKS)

$(TEST_EXEC): $(TEST_OBJECTS) $(LIBNATIVENETWORK)
	@echo "LD\t" $@
	@$(CC) $(LD_FLAGS) -o $@ $^ $(GTEST_LIB) $(LIBS)
	@echo "Run\t" $@
	$(VALGRIND) ./$@

unittest%.o: unittest%.cpp unittest.h
	@echo "CC\t" $@
	@$(CC) $(CC_FLAGS) -o $@ -c $< $(INCLUDES)

run_benchmark_%: benchmark_%.c $(LIBNATIVENETWORK)
	@echo "CCLD\t" $@
	@$(CC) $(CC_FLAGS) $(LD_FLAGS) -o $@ $^ $(GTEST_LIB) $(LIBS) $(INCLUDES)
	
.PHONY: clean

clean:
	@echo "rm\t" $@
	@rm -rf $(TEST_EXEC) $(TEST_OBJECTS)$(BENCHMARKS)

