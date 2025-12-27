DAY := 04
PART := 1

all: build run

build:
	gcc day$(DAY)_$(PART).c -o day$(DAY)_$(PART) -Wall -Werror -Wpedantic -g -O0 -std=gnu23 -Wshadow

run: build
	./day$(DAY)_$(PART)  day$(DAY)_$(PART).txt

gdb:
	gdb --args ./day$(DAY)_$(PART)  day$(DAY)_$(PART).txt

clean:
	rm -f day*.o
