TARGET = notify

all: $(TARGET)

notify:notify.c
	gcc -Wall -o $@ $<

clean:
	rm -rf $(TARGET)
