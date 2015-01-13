ALL_CFLAGS := -Wall -std=gnu99 -ljansson $(CFLAGS)

omark: benchmark.o config.o main.o prng.o
	$(CC) $(ALL_CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(ALL_CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f benchmark.o config.o main.o prng.o omark
