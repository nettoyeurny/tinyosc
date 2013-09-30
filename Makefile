.PHONY: test

check:
	gcc tinyosc_test.c tinyosc.c pattern.c && ./a.out && rm a.out
