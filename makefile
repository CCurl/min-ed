ARCH ?= -m64
app := min-ed

CFLAGS := $(ARCH) -O3 -D IS_LINUX

srcfiles := $(shell find . -name "*.c")
incfiles := $(shell find . -name "*.h")
# LDLIBS   := -lm

all: $(app)

$(app): $(srcfiles) $(incfiles)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(app) $(srcfiles) $(LDLIBS)
	ls -l $(app)

clean:
	rm -f $(app)

test: $(app)
	./$(app) editor.c

bin: $(app)
	cp -u -p $(app) ~/bin/$(app)
