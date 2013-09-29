.PHONY: test

test:
	gcc myosc_test.c myosc.c pattern.c && ./a.out && rm a.out
