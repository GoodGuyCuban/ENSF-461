CC=gcc
EXECUTABLES=scheduler.out
SOURCES=scheduler.c
OUTPUT=my_output_fifo.out my_output_fifo_a.out my_output_sjf.out my_output_sjf_a.out

.PHONY: all clean test

all: $(EXECUTABLES)

scheduler.out: scheduler.c
		$(CC) -o $@ $<

clean:
	rm -f $(EXECUTABLES)
	rm -f $(OUTPUT)

checkif0 = || (echo "Command Failed $$?";exit !0)

test:
	./scheduler.out 0 FIFO example_fifo.in > my_output_fifo.out
	diff my_output_fifo.out example_fifo.out
	./scheduler.out 1 FIFO example_fifo.in > my_output_fifo_a.out
	diff my_output_fifo_a.out example_fifo_analysis.out
	./scheduler.out 0 SJF example_sjf.in > my_output_sjf.out
	diff my_output_sjf.out example_sjf.out
	./scheduler.out 1 SJF example_sjf.in > my_output_sjf_a.out
	diff my_output_sjf_a.out example_sjf_analysis.out