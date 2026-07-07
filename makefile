

library: gcurses.c
	echo "fuck"

demo: gcurses.c demo.c
	gcc gcurses.c demo.c -o demo