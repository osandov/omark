ALL_CFLAGS := -Wall -std=gnu99 -ljansson $(CFLAGS)

omark: config.o omark.o
	$(CC) $(ALL_CFLAGS) -o $@ $^

%.o: %.c config.h
	$(CC) $(ALL_CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f omark.o omark
