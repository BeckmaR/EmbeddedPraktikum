/*
 /* server.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <sstream>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>



#include "driveSim.h"

#include "GPIO.h"

#define ZEDBOARD


/* Puffer für eingehende Nachrichten */
#define RCVBUFSIZE 1024

static void echo(int);

static void respond(int, std::string);

static void getRequest(int fd, char* buf, int maxlen);
static void getRequestedPage(char * in, int inlen, char * outBuf, int outlen);

static std::string readFile(std::string);

static void error_exit(char *errorMessage);

enum buttons {
	LEFT = 4,
	RIGHT = 8,
	UP = 16,
	DOWN = 2
};

static unsigned int last_button_state;

static unsigned int button_eval(int button_state);

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
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	fd_set readset;

	FD_ZERO(&readset);

	driveSim sim("1.1.1.1");
#ifdef ZEDBOARD	
	GPIO gpio(251);
#endif

	int PORT = 1234;


	int sock, fd;

	unsigned int len;

	int old_gpio_val;
	int gpio_val;


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
	if (bind(sock, (struct sockaddr*) &server, sizeof(server)) < 0) {
		error_exit("Kann das Socket nicht binden");
	}
	std::cout << "Connected to port " << PORT << "\n";
		

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
		int result = 0;
		do {
			int gpio_val = 0;
			FD_ZERO(&readset);
			FD_SET(sock, &readset);
#ifdef ZEDBOARD
			gpio_val = gpio.get_gpio_value();
			if(gpio_val & LEFT) {
				sim.steerLeft();
			} if(gpio_val & RIGHT) {
				sim.steerRight();
			} if(gpio_val & UP) {
				sim.accelerate();
			} if(gpio_val & DOWN) {
				sim.brake();
			}
#endif
			/*
			*/
			int result = select(sock + 1, &readset, NULL, NULL, &tv);
		} while (result == -1 && errno == EINTR);

		fd = accept(sock, (struct sockaddr*) &client, &len);

		int buflen = 4096;
		char buf[buflen];
		int pageNameLen = 128;
		char pageName[pageNameLen];
		memset(buf, 0, buflen);
		memset(pageName, 0, pageNameLen);
		

		getRequest(fd, buf, buflen);
		std::cout << buf << "\n";
		getRequestedPage(buf, buflen, pageName, pageNameLen);
		std::cout << pageName << "\n";

		std::string page(pageName);

		if(page == "1") {
			sim.steerLeft();
		} else if(page == "2") {
			sim.accelerate();
		} else if(page == "3") {
			sim.brake();
		} else if(page == "4") { 
			sim.steerRight();
		} else {
			respond(fd, std::string("index.html"));
		}
		
		if (fd < 0)
			error_exit("Fehler bei accept");
		std::cout << "Bearbeite den Client mit der Adresse: " << inet_ntoa(client.sin_addr) << "\n";

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

void getRequest(int fd, char* buf, int buflen)
{
	char * stop = "\r\n\r\n";
	int readBytesSum = 0;
	int readBytes = 0;
	while(readBytes = read(fd, buf, buflen - readBytesSum) && readBytesSum < buflen-1 && !strstr(buf, stop)) {
		readBytesSum += readBytes;
	}/*
std::string out(buf);
std::cout << out.length();
std::cout << "Header:\n";
	std::cout << out;
std::cout << "\n-> end";*/
}

void getRequestedPage(char * in, int inlen, char * outBuf, int outlen)
{
	std::cout << "Parsing header...\n";
	char * GETpos = 0;
	int outPos = 0;
	char * GET = "GET /";

	unsigned int offset = 0;

	if((GETpos = strstr(in, GET)) == NULL) {
		std::cout << "String not found!\n";
		return;
	}
	std::cout << "String found at " << (unsigned)GETpos << "\n";
	
	offset = ((unsigned int) GETpos - (unsigned int) in) / sizeof(char);
	
	offset += 5;

	

	for(int i = offset; i < inlen; i++) {
		if(in[i] != ' ' && in[i] != '\r' && in[i] != '\n' && outPos < outlen) {
			outBuf[outPos++] = in[i];
		} else return;
	}
}

