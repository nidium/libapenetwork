CC=clang
CC_FLAGS=
LD_FLAGS=-lstdc++ -lpthread
GTEST_DIR=../../../third-party/.default/angle/third_party/googletest
INCLUDES=-I.. -isystem../../../third-party/.default/c-ares -isystem $(GTEST_DIR)/include/
GTEST_LIB=$(GTEST_DIR)/libgtest.a $(GTEST_DIR)/libgtest_main.a
LIBAPENETWORK=-L.. -lnativenetwork
VALGRIND=valgrind

TEST_OBJECTS= unittest_base64.o
TEST_EXEC=run_unittests

all: $(TEST_EXEC)

$(TEST_EXEC): $(TEST_OBJECTS)
	@echo "LD\t" $@
	@$(CC) $(LD_FLAGS) -o $@ $^ $(GTEST_LIB) $(LIBAPENETWORK)
	@echo "Run\t" $@
	$(VALGRIND) ./$@

unittest_%.o: unittest_%.cpp
	@echo "CC\t" $@
	@$(CC) $(CC_FLAGS) -o $@ -c $< $(INCLUDES)

.PHONY: clean

clean:
	@echo "rm\t" $@
	@rm -rf $(TEST_EXEC) $(TEST_OBJECTS)

