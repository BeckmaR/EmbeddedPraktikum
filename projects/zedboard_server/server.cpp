/*
 /* server.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <iostream>
#include <fstream>

#include "mjpeg.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

/* Portnummer */
#define PORT 1234

/* Puffer für eingehende Nachrichten */
#define RCVBUFSIZE 1024

static void echo(int);

static void respond(int, std::string);

static std::string readFile(std::string);

static void error_exit(char *errorMessage);

/* Die Funktion gibt Daten vom Client auf stdout aus,
 * die dieser mit der Kommandozeile übergibt. */
static void echo(int client_socket)
		{
	char echo_buffer[RCVBUFSIZE];
	int recv_size;
	time_t zeit;

	if ((recv_size = recv(client_socket, echo_buffer, RCVBUFSIZE, 0)) < 0)
		error_exit("Fehler bei recv()");
	echo_buffer[recv_size] = '\0';
	time(&zeit);
	printf("Nachrichten vom Client : %s \t%s", echo_buffer, ctime(&zeit));
}

/* Die Funktion gibt den aufgetretenen Fehler aus und
 * beendet die Anwendung. */
static void error_exit(char *error_message) {
	fprintf(stderr, "%s: %s\n", error_message, strerror(errno));

	exit(EXIT_FAILURE);
}

std::string intToString(int i) {
	std::stringstream ss;
	ss << i;
	return ss.str();
}

int main(int argc, char *argv[]) {
	struct sockaddr_in server, client;


	int sock, fd;

	unsigned int len;


	/* Erzeuge das Socket. */
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
		error_exit("Fehler beim Anlegen eines Sockets");

	/* Erzeuge die Socketadresse des Servers. */
	memset(&server, 0, sizeof(server));
	/* IPv4-Verbindung */
	server.sin_family = AF_INET;
	/* INADDR_ANY: jede IP-Adresse annehmen */
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	/* Portnummer */
	server.sin_port = htons(PORT);

	/* Erzeuge die Bindung an die Serveradresse
	 * (genauer: an einen bestimmten Port). */
	if (bind(sock, (struct sockaddr*) &server, sizeof(server)) < 0)
		error_exit("Kann das Socket nicht \"binden\"");

	/* Teile dem Socket mit, dass Verbindungswünsche
	 * von Clients entgegengenommen werden. */
	if (listen(sock, 5) == -1)
		error_exit("Fehler bei listen");

	printf("Server bereit - wartet auf Anfragen ...\n");
	/* Bearbeite die Verbindungswünsche von Clients
	 * in einer Endlosschleife.
	 * Der Aufruf von accept() blockiert so lange,
	 * bis ein Client Verbindung aufnimmt. */
	for (;;) {
		len = sizeof(client);
		fd = accept(sock, (struct sockaddr*) &client, &len);
		
		if (fd < 0)
			error_exit("Fehler bei accept");
		printf("Bearbeite den Client mit der Adresse: %s\n",
				inet_ntoa(client.sin_addr));
		respond(fd, std::string("index.html"));

		/* Schließe die Verbindung. */

		close(fd);

	}
	return EXIT_SUCCESS;
}

void respond(int fd, std::string htmlpath) 
{
	/* Daten vom Client auf dem Bildschirm ausgeben */

	std::string content = readFile(htmlpath);

	std::string response = "";
	response += "HTTP/1.1 200 OK\n";
	response += "Server: en.code-bude.net example server\n";
	response += "Content-Length: " + intToString(content.size()); 
	response += "Cache-Control: no-cache\n";
	response += "Cache-Control: private\n";
	response += "Content-Type: text/html; charset=UTF-8\n";
	response += "\n";
	response += content;
	

	usleep(200);
	unsigned int bytesSend = send(fd, response.c_str(), response.size(), 0);
	if(bytesSend != response.size()) {
		std::cout << "Sending error, aborting\n";
	} else {
		std::cout << "Sending successfully\n";
	}
}

std::string readFile(std::string path)
{
	std::string content = "";
	std::string line = "";
	std::fstream myfile(path.c_str());
	if(myfile.is_open()) {
		while(getline(myfile, line)) {
			content += line;
			content += "\n";
		}
	} else {
		std::cout << "file not open!\n";
	}

	return content;
}

