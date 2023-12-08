CXX      := g++
CXXFLAGS := -ggdb -std=c++17
CPPFLAGS := -MMD
SRCDIR   := .

COMPILE  := $(CXX) $(CXXFLAGS) $(CPPFLAGS)

SRCS     := $(wildcard $(SRCDIR)/*.cpp)
OBJS     := $(SRCS:.cpp=.o)
DEPS     := $(SRCS:.cpp=.d)

clox: $(OBJS)
	@$(COMPILE) $(OBJS) -o $@

.PHONY: clean
clean:
	rm -f $(DEPS) $(OBJS) clox

# Include dependencies
-include $(DEPS)

OUTPUT_FILE := test_output.txt
TEST_DIR := tests

define make_test
.PHONY: $(1)

$(1):
	@echo "Testing clox with $(1)..."
	@echo "========================================" >> $(OUTPUT_FILE)
	@echo "Test: $(1)" >> $(OUTPUT_FILE)
	@./clox $(TEST_DIR)/$(1) >> $(OUTPUT_FILE);
	@echo "========================================" >> $(OUTPUT_FILE)
	@echo >> $(OUTPUT_FILE)  # Add a newline for better readability
endef

TESTS := $(patsubst $(TEST_DIR)/%,%,$(wildcard $(TEST_DIR)/**/*.lox))

$(foreach test, $(TESTS), $(eval $(call make_test,$(test))))

.PHONY: test-all
test-all:
	@for test in $(TESTS); do make $$test; done
