CC=clang
VALGRIND=valgrind

CC_FLAGS=-Wall -Wextra
LD_FLAGS=-lstdc++ -lpthread -lz

GTEST_DIR=../../../third-party/.default/angle/third_party/googletest
INCLUDES=-I.. -isystem../../../third-party/.default/c-ares -isystem $(GTEST_DIR)/include/
GTEST_LIB=$(GTEST_DIR)/libgtest.a $(GTEST_DIR)/libgtest_main.a
LIBS=-L.. -lnativenetwork

TEST_OBJECTS=unittest_base64.o \
			unittest_sha1.o \
			unittest_hash.o \
			unittest_httpparser.o \
			unittest_common.o \
			unittest_buffer.o \
	
TEST_EXEC=run_unittests

all: $(TEST_EXEC)

$(TEST_EXEC): $(TEST_OBJECTS)
	@echo "LD\t" $@
	@$(CC) $(LD_FLAGS) -o $@ $^ $(GTEST_LIB) $(LIBS)
	@echo "Run\t" $@
	$(VALGRIND) ./$@

unittest_%.o: unittest_%.cpp
	@echo "CC\t" $@
	@$(CC) $(CC_FLAGS) -o $@ -c $< $(INCLUDES)

.PHONY: clean

clean:
	@echo "rm\t" $@
	@rm -rf $(TEST_EXEC) $(TEST_OBJECTS)

