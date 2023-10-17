CC=gcc
EXECUTABLES=scheduler.out
SOURCES=scheduler.c
OUTPUT=my_output_stcf.out my_output_stcf_a.out

.PHONY: all clean test

all: $(EXECUTABLES)

scheduler.out: scheduler.c
		$(CC) -o $@ $<

clean:
	rm -f $(EXECUTABLES)
	rm -f $(OUTPUT)

checkif0 = || (echo "Command Failed $$?";exit !0)

test:
	#./scheduler.out 0 STCF example_stcf.in 10 > my_output_stcf.out
	#diff my_output_stcf.out example_stcf_analysis.out
	./scheduler.out 1 STCF example_stcf.in 10 > my_output_stcf_a.out
	diff my_output_stcf_a.out example_stcf_analysis.out