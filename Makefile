ALL_CFLAGS := -Wall -std=c99 -D_XOPEN_SOURCE=700 -g $(CFLAGS)

omark: benchmark.o main.o params.o prng.o
	$(CC) $(ALL_CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(ALL_CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f benchmark.o config.o main.o prng.o omark
