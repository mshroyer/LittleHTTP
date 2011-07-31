/* LittleHTTP.c
 *
 * A tiny, useless Winsock2 HTTP server.
 *
 * Mark Shroyer
 * Fri Jul 29 08:48:42 EDT 2011
 */

#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char html[] =
  "HTTP/1.0 200 OK\r\n"
  "Connection: close\r\n"
  "Server: LittleHTTP\r\n"
  "Content-Type: text/html\r\n"
  "\r\n"
  "<html>\r\n"
  "<head>\r\n"
  "<title>LittleHTTP</title>\r\n"
  "</head>\r\n"
  "<body>\r\n"
  "<h1>LittleHTTP</h1>\r\n"
  "<p>You got served!</p>\r\n"
  "</body>\r\n"
  "</html>\r\n";

WSADATA wsaData;
SOCKET sock;
int tcpPort = 80;
int verbose = 0;

void carp(wchar_t* msg)
{
  fwprintf(stderr, L"%s\n", msg);
}

void croak(wchar_t* msg)
{
  carp(msg);
  WSACleanup();
  exit(1);
}

void croakUsage()
{
  fwprintf(stderr,
           L"LittleHTTP - A tiny, useless Winsock2 HTTP server.\n"
           L"https://github.com/markshroyer/LittleHTTP\n"
           L"\n"
           L"Usage: LittleHTTP.exe [/P <port_num>] [/V] [/?]\n");
  exit(1);
}

int handleClient(SOCKET clientSock)
{
  char cCur = 0, cLast = 0, cBuf = 0;
  int headersEnded = 0;
  int count;
  char* reqHdr = calloc(2048, sizeof(char));
  int reqHdrLen = 0;

  // Wait until we've actually received the end of the HTTP request headers
  // before we start sending our response...
  do {
    count = recv(clientSock, &cBuf, 1, 0);
    if ( count == SOCKET_ERROR ) {
      carp(L"Could not recv from client");
      free(reqHdr);
      return 1;
    }

    if ( cBuf != '\r' && cBuf != '\b' ) {
      cLast = cCur;
      cCur = cBuf;
      if ( reqHdrLen < 2047 )
        reqHdr[reqHdrLen++] = cBuf;
    }

    if ( cCur == '\n' && cLast == '\n' )
      headersEnded = 1;
  } while ( ! headersEnded );
  
  if ( verbose )
    printf("\n%s", reqHdr);

  send(clientSock, html, strlen(html), 0);

  free(reqHdr);
  return 0;
}

int main(int argc, char* argv[])
{
  SOCKADDR_IN sockAddr;
  SOCKET clientSock;
  int i;
  char* p = NULL;

  for ( i=1; i<argc; i++ ) {
    if ( strcmp(argv[i], "/V") == 0 ) {
      verbose = 1;
    } else if ( strcmp(argv[i], "/P") == 0 ) {
      if ( ++i >= argc )
        croakUsage();
      tcpPort = strtol(argv[i], &p, 10);
      if ( ( p == argv[i] ) || ( tcpPort <= 0 ) || ( tcpPort > 65535 ) )
        croakUsage();
    } else {
      croakUsage();
    }
  }

  if ( WSAStartup(MAKEWORD(2,2), &wsaData) != 0 )
    croak(L"Winsock2 initialization failed");

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if ( sock == INVALID_SOCKET )
    croak(L"Socket creation failed");

  sockAddr.sin_port = htons(tcpPort);
  sockAddr.sin_family = AF_INET;
  sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ( bind(sock, (LPSOCKADDR)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR )
    croak(L"Unable to bind TCP socket on this port");

  if ( listen(sock, 10) == SOCKET_ERROR )
    croak(L"Unable to listen");

  wprintf(L"Listening on port %d...\n", tcpPort);

  while ( clientSock = accept(sock, NULL, 0) ) {
    if ( clientSock == INVALID_SOCKET ) {
      closesocket(clientSock);
      croak(L"Accept failed");
    }
    handleClient(clientSock);
    shutdown(clientSock, SD_BOTH);
    closesocket(clientSock);
  }

  return 0;
}
