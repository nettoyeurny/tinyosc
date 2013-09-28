.PHONY: clean

myosc_test: myosc_test.c myosc.h myosc.c pattern.h pattern.c
	gcc myosc_test.c myosc.c pattern.c -o myosc_test

clean:
	rm -f myosc_test
