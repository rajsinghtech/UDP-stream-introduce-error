all:
	make shared
	make receiver
	make sender

shared:
	gcc -c utilities.c -o utilities.o -g -no-pie

receiver:
	gcc -c secondary.c -o secondary.o -g -no-pie
	gcc -o receiver receiver.o secondary.o utilities.o ccitt16.o -g -no-pie

sender:
	gcc -c introduceerror.c -g -o introduceerror.o -no-pie
	gcc -c primary.c -g -o primary.o -no-pie
	gcc -o sender sender.o primary.o utilities.o ccitt16.o introduceerror.o -g -no-pie

clean:
	- rm utilities.o
	- rm secondary.o
	- rm primary.o
	- rm receiver
	- rm sender
