#define _WIN32_WINNT 0x501

#include "stdafx.h"
#include "application.cpp"
#include <iostream>



using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#define IP_ADDR "127.0.0.1"
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512
#undef UNICODE

int main() {


    WSADATA wsaData;

    int iResult;

    // Initialize Winsock:
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // Creating socket:
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(IP_ADDR, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;
    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    ptr=result;

    // Create a SOCKET for connecting to server
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Connect to server.
    iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }
    
    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Exchanging messages:

    int recvbuflen = DEFAULT_BUFLEN;
    //char recvbuf[DEFAULT_BUFLEN];
    //const char* sendbuf = "Request message!";

    char buf[DEFAULT_BUFLEN];
    memset(buf, 0, DEFAULT_BUFLEN);

    while(1) {

        printMenu();

        while(1) {
            
            runApp(buf);
    
            iResult = send(ConnectSocket, buf, (int) strlen(buf), 0);

            if (iResult == SOCKET_ERROR) {
                printf("send failed: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
                break;
            }

            iResult = recv(ConnectSocket, buf, recvbuflen, 0);
            if (iResult > 0){
                printf("Estado do LED alternado.\n");
                //puts(buf);
            } else if (iResult == 0){
                puts(buf);
                break;
            }else{
                printf("recv failed: %d\n", WSAGetLastError());
                break;
            }
        }

        break;   
    }
    
    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}