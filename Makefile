CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm

TARGET = compact_rational
CANON_TARGET = canonicalize
OPTIMAL_TARGET = optimal_encoding
VERIFY_TARGET = verify_bug

TARGETS = $(TARGET) $(CANON_TARGET) $(OPTIMAL_TARGET) $(VERIFY_TARGET)

.PHONY: all clean test canon optimal verify

all: $(TARGETS)

$(TARGET): compact_rational.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(CANON_TARGET): canonicalize.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(OPTIMAL_TARGET): optimal_encoding.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(VERIFY_TARGET): verify_bug.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test: $(TARGET)
	./$(TARGET)

canon: $(CANON_TARGET)
	./$(CANON_TARGET)

optimal: $(OPTIMAL_TARGET)
	./$(OPTIMAL_TARGET)

verify: $(VERIFY_TARGET)
	./$(VERIFY_TARGET)

clean:
	rm -f $(TARGETS)

run: $(TARGET)
	./$(TARGET)
