// #####################################################################
//
// Programmcode basiert auf einer Implementierung der Firma Foerst GmbH!
//
// #####################################################################


#ifndef LABCAR_CLIENT_H_INCLUDED
#define LABCAR_CLIENT_H_INCLUDED

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
#include <stdexcept> // runtime_error
#include <cstring>

#ifdef linux
	#include <sys/socket.h> // socket(), connect()
	#include <arpa/inet.h> // sockaddr_in
	#include <netdb.h> // gethostbyname(), hostent
	#include <errno.h> // errno
	#include <netinet/tcp.h>
#else
  #include <winsock2.h>
  #include <ws2tcpip.h>

  #pragma comment(lib, "ws2_32")
#endif


#pragma pack(push)
#pragma pack(1)

enum { MSGSIGNATURE = 0xcc };

struct MSGHDR
{
	unsigned char	signature;		// Identifies start of a message (MYSIGNATURE)
	unsigned char	flags;			// bitmask, 1: first of a sequence 2: last of a sequence
    unsigned short	length;			// size of message (littleendian)
    unsigned char	command;		// message command
	unsigned char	flagcheck;		// inverted flags byte
};

struct DATAMSG
{
	MSGHDR		hdr;
	char		data[0x8000];		// length does not really matter - never allocated
};

#pragma pack(pop)

enum {
	REGISTER_NAME				= 1,
	XFER_DATA					= 2,
	REQUEST_SESSION				= 3,
	SESSION_REQUEST_RESPONSE	= 4,
	SESSION_CLOSE				= 5,
	DEREGISTER_NAME				= 6,
	XFER_WORLDDATA				= 7,
	XFER_OUT_OF_BAND			= 8,
	REQUEST_SESSION2			= 9,	// send code page instead of name
};

class LabCarClient {
  private:
    int sock;
    std::vector<std::string> results;
    std::runtime_error CreateSocketError(std::string error_message);
    void fullrecv(int s, void* dest, int count);
    void fullsend(int sock, const void* src, int count);
    void SendPacket(int s, const MSGHDR* pack);
    void SendPacket(int s, const DATAMSG* pack);
    bool ReceivePacket(int s, std::vector<char>& resultData, int timeoutMs);
    void InitialProtocol(int s, const char* clientname);
    int OpenF12Socket(const char* address, const char* clientname);
    void CloseF12Socket(int sock);
    void SendCommand(int sock, const std::string& cmd);
    std::string ReceiveAnswer(int sock, size_t idx);
  public:
    LabCarClient(std::string address);
    ~LabCarClient(void);
    std::vector<std::string> sendCmd(std::vector<std::string> commands);
    void printResults(void);
};

#endif // LABCAR_CLIENT_H_INCLUDED
