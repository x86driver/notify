TARGET = notify daemon

all: $(TARGET)

notify:notify.c
	gcc -Wall -o $@ $<

daemon:daemon.c
	gcc -Wall -o $@ $<

clean:
	rm -rf $(TARGET)
