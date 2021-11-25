all: cliente gestor verificador

cliente: cliente.o util.o mensagem.o
	gcc -o cliente cliente.o util.o mensagem.o -lncurses -lpthread

cliente.o: cliente.c
	gcc -o cliente.o -c cliente.c

gestor: gestor.o util.o mensagem.o
	gcc -o gestor gestor.o util.o mensagem.o -lpthread

gestor.o: gestor.c
	gcc -o gestor.o -c gestor.c

util.o: util.c
	gcc -o util.o -c util.c

verificador: verificador.o
	gcc -o verificador verificador.o

verificador.o:
	gcc -o verificador.o -c verificador.c

mensagem.o: mensagem.c
	gcc -o mensagem.o -c mensagem.c

clean:
	rm -rf *.o
