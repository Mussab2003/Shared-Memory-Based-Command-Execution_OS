target:
	gcc -o Producer Producer.c
	gcc -o Standby Standby.c
	gcc -o Consumer Consumer.c
	./Producer
	./Standby & ./Consumer
