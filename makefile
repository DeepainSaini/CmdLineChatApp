all: compile run_server
compile: compile_server compile_client
keys: pri_key pub_key

run_server: server.out
	./server.out 6969
run_client1: client.out
	./client.out 127.0.0.1 6969 private_c1.pem public_c2.pem
run_client2: client.out
	./client.out 127.0.0.1 6969 private_c2.pem public_c1.pem
	
compile_client: client.c
	gcc client.c -lssl -lcrypto -lpthread -o client.out
# gcc-10 client.c -I/usr/local/Cellar/openssl@1.1/1.1.1k/include -L/usr/local/Cellar/openssl@1.1/1.1.1k/lib/ -lssl -lcrypto -lpthread -o client.out

compile_server: server.c
	gcc server.c -lssl -lcrypto -lpthread -o server.out
# gcc-10 server.c -I/usr/local/Cellar/openssl@1.1/1.1.1k/include -L/usr/local/Cellar/openssl@1.1/1.1.1k/lib/ -lssl -lcrypto -lpthread -o server.out

pri_key:
	openssl genrsa -out private_c1.pem 4096
	openssl genrsa -out private_c2.pem 4096
pub_key:
	openssl rsa -in private_c1.pem -outform PEM -pubout -out public_c1.pem
	openssl rsa -in private_c2.pem -outform PEM -pubout -out public_c2.pem

clean:
	rm *.out

delete_keys:
	rm *.pem
