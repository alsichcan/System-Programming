CFLAGS=-O2 -fno-dce -fno-dse -fno-tree-dce -fno-tree-dse

targets := $(patsubst %.c,%,$(wildcard *.c))

% : %.c
	gcc $(CFLAGS) -o $@ $<

all: $(targets)

clean:
	rm -rf $(targets)
