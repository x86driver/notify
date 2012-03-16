TARGET = notify daemon

all: $(TARGET)

notify:notify.c
	gcc -Wall -o $@ $<

daemon:daemon.c
	gcc -g -Wall -o $@ $< -lpthread

clean:
	rm -rf $(TARGET)
