all: WTF WTFserver

WTF:
	gcc client/client.c -lcrypto -lz -o WTF
	
WTFserver:
	gcc -lpthread -lcrypto server/server.c -o WTFserver

clean:
	rm ./WTF; rm ./WTFserver; rm ./WTFtest

test:
	gcc test.c -o WTFtest