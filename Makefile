# SQL Database Engine Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -Iinc -Isrc
SRCDIR = src
OBJDIR = obj
DATADIR = data

# Source files
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/parser/tokenizer.cpp \
          $(SRCDIR)/parser/parser.cpp \
          $(SRCDIR)/storage/metadata.cpp \
          $(SRCDIR)/storage/table.cpp \
          $(SRCDIR)/executor/query_executor.cpp

# Object files
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Target executable
TARGET = sqldb

# Default target
all: $(TARGET)

# Create directories
$(OBJDIR):
	mkdir -p $(OBJDIR)/parser $(OBJDIR)/storage $(OBJDIR)/executor

$(DATADIR):
	mkdir -p $(DATADIR)

# Build target
$(TARGET): $(OBJDIR) $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

# Object file rules
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build
release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)

# Run the application
run: $(TARGET) $(DATADIR)
	./$(TARGET)

# Memory check (if valgrind is available)
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Clean all (including data)
clean-all: clean
	rm -rf $(DATADIR)

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Build the application (default)"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release version"
	@echo "  run       - Build and run the application"
	@echo "  memcheck  - Run with memory leak detection"
	@echo "  clean     - Remove build artifacts"
	@echo "  clean-all - Remove build artifacts and data files"
	@echo "  help      - Show this help message"

.PHONY: all debug release run memcheck clean clean-all help
