// #####################################################################
//
// Programmcode basiert auf einer Implementierung der Firma Foerst GmbH!
//
// #####################################################################


#include "LabCarClient.h"

using namespace std;


runtime_error LabCarClient::CreateSocketError(string error_message) {
	ostringstream temp;

    #ifdef linux
        temp << error_message << ": " << errno;
    #else
        temp << error_message << ": " << WSAGetLastError();
    #endif

    return runtime_error(temp.str());
}

void LabCarClient::fullrecv(int s, void* dest, int count) {
	char* d = (char*)dest;
	while (count > 0)
	{
		int r = recv(s, d, count, 0);
		if (r == 0)
			throw CreateSocketError("WSAECONNABORTED");
		else if (r < 0)
			throw CreateSocketError("");

		d += r;
		count -= r;
	}
}

void LabCarClient::fullsend(int sock, const void* src, int count) {
	const char* s = (const char*)src;
	while (count > 0)
	{
		int t = send(sock, s, count, 0);
		if (t == 0)
			throw CreateSocketError("WSAECONNABORTED");
		else if (t < 0)
			throw CreateSocketError("");

		s += t;
		count -= t;
	}
}

void LabCarClient::SendPacket(int s, const MSGHDR* pack) {
	assert(pack && pack->length >= sizeof(MSGHDR));
	fullsend(s, pack, pack->length);
}

void LabCarClient::SendPacket(int s, const DATAMSG* pack) {
	SendPacket(s, &pack->hdr);
}

bool LabCarClient::ReceivePacket(int s, vector<char>& resultData, int timeoutMs) {
	timeval tv;
	tv.tv_sec = timeoutMs / 1000;
	tv.tv_usec = 1000 * (timeoutMs % 1000);
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(s, &fds);
	int num = ::select((int)(s+1), &fds, NULL, NULL, &tv);
	if (!num)
		return false;

	// there was only one, so it must be ours.
	assert(FD_ISSET(s, &fds));

	MSGHDR head;
	fullrecv(s, &head, sizeof(head));
	if (head.signature != (unsigned char)MSGSIGNATURE || head.length < sizeof(head))
	    throw CreateSocketError("WSAECONNABORTED");
	if (head.flagcheck != (unsigned char)~head.flags)
	{
		// old style packet
		head.flags = 3;
		head.flagcheck = ~3;
	}
	resultData.resize(head.length);
	memcpy(&(resultData[0]), &head, sizeof(head));
	if (head.length > sizeof(head))
		fullrecv(s, &(resultData[sizeof(head)]), head.length-sizeof(head));
	return true;
}

void LabCarClient::InitialProtocol(int s, const char* clientname) {
	// we first wait for the server to identify itself.
	vector<char> data;
	if (!ReceivePacket(s, data, 5000))
	    throw CreateSocketError("Handshake WSAETIMEDOUT");
	const DATAMSG* msg = reinterpret_cast<const DATAMSG*>(&(data[0]));
	if (msg->hdr.command != (unsigned char)REGISTER_NAME)
	    throw CreateSocketError("Handshake -1");

	std::string simname(msg->data, msg->data + msg->hdr.length - sizeof(msg->hdr));
	cerr << "Connecting to simulator '" << simname << "'..." << std::endl;

	if (!clientname || !*clientname)
		clientname = "client";

	// the below wastes stack space, but this is a demo app.
	DATAMSG spack;
	spack.hdr.flags = 3;
	spack.hdr.flagcheck = ~3;
	spack.hdr.signature = MSGSIGNATURE;

	// we send our name.
	// the below addition sets the transfer encoding to utf-8 (otherwise: western)
	size_t namelen = strlen(clientname);
	memcpy(spack.data, clientname, namelen+1);
	strcpy(spack.data+namelen+1, "$CP65001");
	namelen = namelen + 2 + strlen(spack.data+namelen+1);
	spack.hdr.command = REGISTER_NAME;
	spack.hdr.length = (unsigned short)(sizeof(MSGHDR) + namelen);
	SendPacket(s, &spack);

	// the server now answers with REQUEST_SESSION2
	// (if we hadn't sent the $CP suffix, it wourld be REQUEST_SESSION)
	if (!ReceivePacket(s, data, 5000))
	    throw CreateSocketError("Handshake WSAETIMEDOUT");
	msg = reinterpret_cast<const DATAMSG*>(&(data[0]));
	if (msg->hdr.command != (unsigned char)REQUEST_SESSION2)
	    throw CreateSocketError("Handshake -1");

	// now complete it.
	spack.hdr.command = SESSION_REQUEST_RESPONSE;
	spack.data[0] = 1;
	spack.hdr.length = (unsigned short)(sizeof(MSGHDR) + 1);
	SendPacket(s, &spack);
}

int LabCarClient::OpenF12Socket(const char* address, const char* clientname) {
	// convert the string address to a sockaddr structure
	struct addrinfo* paddrinfo = NULL;
	int rc = ::getaddrinfo(address, "1365", NULL, &paddrinfo);
	if (rc != 0)
	    throw CreateSocketError("address resolution rc");
	assert(paddrinfo && paddrinfo->ai_addr);

	// open a tcp socket
	int sock = ::socket(AF_INET, SOCK_STREAM, 0);
	  if(sock == -1) throw CreateSocketError("");

	// disable nagle
	bool nonagle = 1;
	::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&nonagle, sizeof(bool));

	// connect to server
	rc = ::connect(sock, paddrinfo->ai_addr, (int)paddrinfo->ai_addrlen);
	if(rc != 0) throw CreateSocketError("");

	// perform the initial handshake
	try
	{
		InitialProtocol(sock, clientname);
	}
	catch(...)
	{
	  #ifdef linux
      ::close(sock);
	  #else
      ::closesocket(sock);
    #endif
		throw;
	}

	return sock;
}

void LabCarClient::CloseF12Socket(int sock) {
	// we are nice and say good-bye
	MSGHDR spack;
	spack.flags = 3;
	spack.flagcheck = ~3;
	spack.signature = MSGSIGNATURE;
	spack.command = SESSION_CLOSE;
	spack.length = (unsigned short)(sizeof(MSGHDR));
	SendPacket(sock, &spack);
	#ifdef linux
    close(sock);
	#else
    ::closesocket(sock);
  #endif
}

void LabCarClient::SendCommand(int sock, const string& cmd) {
	size_t cmdlen = cmd.size();
	std::vector<char> pack;
	pack.assign(sizeof(MSGHDR) + cmdlen + 1, (char)0);
	DATAMSG* msg = reinterpret_cast<DATAMSG*>(&(pack[0]));

	msg->hdr.flags = 3;
	msg->hdr.flagcheck = ~3;
	msg->hdr.signature = MSGSIGNATURE;
	msg->hdr.command = XFER_DATA;
	msg->hdr.length = (unsigned short)pack.size();
	memcpy(msg->data, cmd.c_str(), cmdlen);

	SendPacket(sock, msg);
}

string LabCarClient::ReceiveAnswer(int sock, size_t idx) {
	vector<char> data;
	const DATAMSG* msg = NULL;
	for (;;)
	{
		if (!ReceivePacket(sock, data, 5000))
			throw CreateSocketError("Reception WSAETIMEDOUT");

		msg = reinterpret_cast<const DATAMSG*>(&(data[0]));
		switch (msg->hdr.command)
		{
		case (unsigned char)SESSION_CLOSE:
			throw CreateSocketError("Reception 0");
			break;

		case (unsigned char)XFER_DATA:
			break;	// this is what we want!

		default:
			continue;	// do nothing with other messages.
		}
		break;
	}

	string answer;
	if (msg->hdr.length > sizeof(MSGHDR))
	{
		answer = string(msg->data, msg->data + msg->hdr.length - sizeof(MSGHDR));
	}

	// N.B.: The first reply token identifies the function, for which this is
	// the reply. There is no obvious rule correlating function names and reply
	// tokens (the latter are kept for backwards compatibility reasons).
	// Hence, one could as well delete the first token.

	return answer;
}

LabCarClient::LabCarClient(string address) {
  sock = OpenF12Socket(address.c_str(), "DID");
}

LabCarClient::~LabCarClient(void) {
  CloseF12Socket(sock);
}

vector<std::string> LabCarClient::sendCmd(std::vector<std::string> commands) {
  results.clear();

	for (size_t i = 0; i < commands.size(); ++i)
		SendCommand(sock, commands[i]);

	for (size_t i = 0; i < commands.size(); ++i) {
		results.push_back(ReceiveAnswer(sock, i));
	}

	return results;
}

void LabCarClient::printResults(void) {
  for(unsigned int i=0; i<results.size(); ++i) {
    string result = results.at(i);
    cout << i << ": " << result << endl;
  }
}
