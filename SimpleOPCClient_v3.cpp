// Simple OPC Client
//
// This is a modified version of the "Simple OPC Client" originally
// developed by Philippe Gras (CERN) for demonstrating the basic techniques
// involved in the development of an OPC DA client.
//
// The modifications are the introduction of two C++ classes to allow the
// the client to ask for callback notifications from the OPC server, and
// the corresponding introduction of a message comsumption loop in the
// main program to allow the client to process those notifications. The
// C++ classes implement the OPC DA 1.0 IAdviseSink and the OPC DA 2.0
// IOPCDataCallback client interfaces, and in turn were adapted from the
// KEPWARE�s  OPC client sample code. A few wrapper functions to initiate
// and to cancel the notifications were also developed.
//
// The original Simple OPC Client code can still be found (as of this date)
// in
//        http://pgras.home.cern.ch/pgras/OPCClientTutorial/
//
//
// Luiz T. S. Mendes - DELT/UFMG - 15 Sept 2011
// luizt at cpdee.ufmg.br
//
#define _WIN32_WINNT 0x501

#pragma comment(lib, "Ws2_32.lib")

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#define DEFAULT_PORT "27015"
#define IP_ADDR "127.0.0.1"
#define DEFAULT_BUFLEN 512
#undef UNICODE



/* ----- AREA DE INCLUDE ----- */
#include "stdafx.h"
#include <atlbase.h>    // required for using the "_T" macro
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <ObjIdl.h>
#include "opcda.h"
#include "opcerror.h"
#include "SimpleOPCClient_v3.h"
#include "SOCAdviseSink.h"
#include "SOCDataCallback.h"
#include "SOCWrapperFunctions.h"

using namespace std;

/* ----- AREA DE DEFINE ----- */
#define OPC_SERVER_NAME L"ArduinoOPCServer.2"  //Nome do servidor
#define VT VT_R4

// Prototipo da função de chamada da Thread Secundaria: Server TCP/IP
DWORD WINAPI ServerTCPIP(LPVOID index);

/* ----- GLOBAL VARIABLES ----- */
// The OPC DA Spec requires that some constants be registered in order to use
// them. The one below refers to the OPC DA 1.0 IDataObject interface.
UINT OPC_DATA_TIME = RegisterClipboardFormat (_T("OPCSTMFORMATDATATIME"));

wchar_t ITEM_ID1[] = L"ArduinoSerial0.lr"; //ID do item a ser lido/escrito do servidor
wchar_t ITEM_ID2[] = L"ArduinoSerial0.lg"; //ID do item a ser lido/escrito do servidor
wchar_t ITEM_ID3[] = L"ArduinoSerial0.ly"; //ID do item a ser lido/escrito do servidor

//wchar_t ITEM_CLIENTID[] = L"ArduinoSerial0.cor"; //ID do item a ser escrito no servidor

IOPCServer* pIOPCServer = NULL;   //pointer to IOPServer interface
IOPCItemMgt* pIOPCItemMgt = NULL; //pointer to IOPCItemMgt interface

// Group and itens server
OPCHANDLE hServerGroup; // server handle to the group

OPCHANDLE hServerItem1;  // server handle to the item
OPCHANDLE hServerItem2;  // server handle to the item
OPCHANDLE hServerItem3;  // server handle to the item

// OPCHANDLE hClientItem;

VARIANT varOFF;
VARIANT varON;
VARIANT varCOR;

char command;

// HANDLE MUTEX
HANDLE hMutexStatus;
HANDLE hMutexSetup;

// HANDLE EVENT
HANDLE hEvent;

void printData();
void printAnswer(char recvbuf[]);

bool flag = 0;


void main() {

  /* ----- PRIMEIRA THREAD: OPC CLIENTE ----- */
	// Definicao do nome do terminal
	SetConsoleTitle("Terminal OPC Cliente");

	// Criacao da thread secundaria
	HANDLE hServerTCPIP, hEscritaSincrona;

	DWORD dwServerTCPIPId, dwEscritaSincronaId;
	DWORD dwExitCode = 0;

	int i = 1;
	int nTipoEvento;

	bool exit = false;

	/* Add: Valor1, Valor2, Valor3 para os LEDS, qual o tipo? */

	hServerTCPIP = CreateThread(NULL, 0, ServerTCPIP, (LPVOID)i, 0, &dwServerTCPIPId);
	if(hServerTCPIP)  printf("Thread %d criada com ID = %0d \n", i, dwServerTCPIPId);


	// Criacao objetos do tipo Mutex
	hMutexStatus = CreateMutex(NULL, FALSE, "MutexStatus");
	GetLastError();

	hMutexSetup = CreateMutex(NULL, FALSE, "MutexSetup");
	GetLastError();

	// Criacao objetos do tipo Eventos
	hEvent = CreateEvent(NULL, FALSE, FALSE, "Escrita");
	GetLastError();

	char buf[100];

	// Have to be done before using microsoft COM library:
	printf("Initializing the COM environment...\n");
	CoInitialize(NULL);

	// Let's instantiante the IOPCServer interface and get a pointer of it:
	printf("Intantiating the Arduino OPC Server...\n");
	pIOPCServer = InstantiateServer(OPC_SERVER_NAME);
	
	// Add the OPC group the OPC server and get an handle to the IOPCItemMgt
	//interface:
	printf("Adding a group in the INACTIVE state for the moment...\n");
	AddTheGroup(pIOPCServer, pIOPCItemMgt, hServerGroup);

	// Add the OPC item. First we have to convert from wchar_t* to char*
	// in order to print the item name in the console.
    size_t m;
	wcstombs_s(&m, buf, 100, ITEM_ID1, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);
	wcstombs_s(&m, buf, 100, ITEM_ID2, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);
	wcstombs_s(&m, buf, 100, ITEM_ID3, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);

    AddTheItems(pIOPCItemMgt, hServerItem1, hServerItem2, hServerItem3);

	//Synchronous read of the device�s item value.
	VARIANT varValue1; //to store the read value of red LED
	VariantInit(&varValue1);
	varValue1.vt = VT_R8;

	VARIANT varValue2; //to store the read value of green LED
	VariantInit(&varValue2);
	varValue2.vt = VT_R8;

	VARIANT varValue3; //to store the read value of yellow LED
	VariantInit(&varValue3);
	varValue3.vt = VT_R8;

	printf ("Reading synchronously...\n");
	printf("\n");


	while(!exit) {

		VariantInit(&varOFF);
		varOFF.vt = VT_R8;
		varOFF.boolVal = 0;
		varOFF.fltVal = 0;

		VariantInit(&varON);
		varON.vt = VT_R8;
		varON.boolVal = 1;
		varON.fltVal = 1;
	
		WaitForSingleObject(hMutexSetup, INFINITE); 

		WaitForSingleObject(hMutexStatus, INFINITE);

		ReadItem(pIOPCItemMgt, hServerItem1, varValue1);
		ReadItem(pIOPCItemMgt, hServerItem2, varValue2);
		ReadItem(pIOPCItemMgt, hServerItem3, varValue3);

		// Comando para dar exit 
		if (command == 'e' && flag == 0) {
			exit = true;
			flag = 1;
		}
		// Comando para leitura de todos os LEDS: ligar
		else if (command == 'a' && flag == 0){
			WriteItem(pIOPCItemMgt, hServerItem1, varON); 
			WriteItem(pIOPCItemMgt, hServerItem2, varON); 
			WriteItem(pIOPCItemMgt, hServerItem3, varON); 
			flag = 1;
			// Imprime dados atualizados
			printData();
		}
		// Comando para leitura de todos os LEDS: desligar
		else if (command == 'o' && flag == 0){
			WriteItem(pIOPCItemMgt, hServerItem1, varOFF); 
			WriteItem(pIOPCItemMgt, hServerItem2, varOFF); 
			WriteItem(pIOPCItemMgt, hServerItem3, varOFF); 
			flag = 1;
			// Imprime dados atualizados
			printData();
		}
		// Comando para leitura do LED VERMELHO
		else if( command == 'r' && flag == 0) {

			if (varValue1.fltVal == 0) {
				WriteItem(pIOPCItemMgt, hServerItem1, varON); 
			}
			else {
				WriteItem(pIOPCItemMgt, hServerItem1, varOFF); 
			}
			// Imprime dados atualizados
			printData();
			ReleaseSemaphore(hMutexStatus, 1, NULL);
			flag = 1;
		}
		// Comando para leitura do LED VERDE
		else if( command == 'g' && flag == 0) {

			if (varValue2.fltVal == 0) {
				WriteItem(pIOPCItemMgt, hServerItem2, varON); 
			}
			else {
				WriteItem(pIOPCItemMgt, hServerItem2, varOFF); 
			}
			// Imprime dados atualizados
			printData();
			ReleaseSemaphore(hMutexStatus, 1, NULL);
			flag = 1;
		}
		// Comando para leitura do LED AMARELO
		else if( command == 'y' && flag == 0) {

			if (varValue3.fltVal == 0) {
				WriteItem(pIOPCItemMgt, hServerItem3, varON); 
			}
			else {
				WriteItem(pIOPCItemMgt, hServerItem3, varOFF); 
			}
			// Imprime dados atualizados
			printData();
			ReleaseSemaphore(hMutexStatus, 1, NULL);
			flag = 1;
		}

		Sleep(500); // Espera 0.5 segundos
	}

	// Remove the OPC item:
	printf("Removing the OPC itens...\n");
	RemoveItems(pIOPCItemMgt, hServerItem1, hServerItem2, hServerItem3);

	// Remove the OPC group:
	printf("Removing the OPC group object...\n");
    pIOPCItemMgt->Release();
	RemoveGroup(pIOPCServer, hServerGroup);

	// release the interface references:
	printf("Removing the OPC server object...\n");
	pIOPCServer->Release();

	//close the COM library:
	printf ("Releasing the COM environment...\n");
	CoUninitialize();
}

// Escrita no terminal do OPC Client
void printData() {
	//system("cls"); // limpa tela
	string str = "-----------------------------\n";
	str += "Status dos LEDS: \n\n";

	VARIANT varValue;
	VariantInit(&varValue);

	WaitForSingleObject(hMutexStatus, INFINITE);

	ReadItem(pIOPCItemMgt, hServerItem1, varValue);
	if (varValue.fltVal != 0) str += "LED Vermelho: Ligado\n";
	else str += "LED Vermelho: Desligado\n";

	ReadItem(pIOPCItemMgt, hServerItem2, varValue);
	if (varValue.fltVal != 0) str += "LED Verde: Ligado\n";
	else str += "LED Verde: Desligado\n";

	ReadItem(pIOPCItemMgt, hServerItem3, varValue);
	if (varValue.fltVal != 0) str += "LED Amarelo: Ligado\n";
	else str += "LED Amarelo: Desligado\n";

	str += "-----------------------------\n\n";

	ReleaseSemaphore(hMutexStatus, 1, NULL);

	cout << str;
}


////////////////////////////////////////////////////////////////////
// Instantiate the IOPCServer interface of the OPCServer
// having the name ServerName. Return a pointer to this interface
//
IOPCServer* InstantiateServer(wchar_t ServerName[])
{
	CLSID CLSID_OPCServer;
	HRESULT hr;

	// get the CLSID from the OPC Server Name:
	hr = CLSIDFromString(ServerName, &CLSID_OPCServer);
	_ASSERT(!FAILED(hr));


	//queue of the class instances to create
	LONG cmq = 1; // nbr of class instance to create.
	MULTI_QI queue[1] =
		{{&IID_IOPCServer,
		NULL,
		0}};

	//Server info:
	//COSERVERINFO CoServerInfo =
    //{
	//	/*dwReserved1*/ 0,
	//	/*pwszName*/ REMOTE_SERVER_NAME,
	//	/*COAUTHINFO*/  NULL,
	//	/*dwReserved2*/ 0
    //}; 

	// create an instance of the IOPCServer
	hr = CoCreateInstanceEx(CLSID_OPCServer, NULL, CLSCTX_SERVER,
		/*&CoServerInfo*/NULL, cmq, queue);
	_ASSERT(!hr);

	// return a pointer to the IOPCServer interface:
	return(IOPCServer*) queue[0].pItf;
}


/////////////////////////////////////////////////////////////////////
// Add group "Group1" to the Server whose IOPCServer interface
// is pointed by pIOPCServer. 
// Returns a pointer to the IOPCItemMgt interface of the added group
// and a server opc handle to the added group.
//
void AddTheGroup(IOPCServer* pIOPCServer, IOPCItemMgt* &pIOPCItemMgt, 
				OPCHANDLE& hServerGroup)
{
	DWORD dwUpdateRate = 0;
	OPCHANDLE hClientGroup = 0;

	// Add an OPC group and get a pointer to the IUnknown I/F:
    HRESULT hr = pIOPCServer->AddGroup(/*szName*/ L"Group1",
		/*bActive*/ FALSE,
		/*dwRequestedUpdateRate*/ 1000,
		/*hClientGroup*/ hClientGroup,
		/*pTimeBias*/ 0,
		/*pPercentDeadband*/ 0,
		/*dwLCID*/0,
		/*phServerGroup*/&hServerGroup,
		&dwUpdateRate,
		/*riid*/ IID_IOPCItemMgt,
		/*ppUnk*/ (IUnknown**) &pIOPCItemMgt);
	_ASSERT(!FAILED(hr));
}



//////////////////////////////////////////////////////////////////
// Add the Item ITEM_ID to the group whose IOPCItemMgt interface
// is pointed by pIOPCItemMgt pointer. Return a server opc handle
// to the item.

void AddTheItems(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE& hServerItem1, OPCHANDLE& hServerItem2, OPCHANDLE& hServerItem3)
{
	HRESULT hr;

	// Array of items to add:
	OPCITEMDEF ItemArray[4] =
	{ {
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID1,
			/*bActive*/ TRUE,
			/*hClient*/ 0,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
			},
		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID2,
			/*bActive*/ TRUE,
			/*hClient*/ 1,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		},

		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_ID3,
			/*bActive*/ TRUE,
			/*hClient*/ 2,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		},
		};

	//Add Result:
	OPCITEMRESULT* pAddResult=NULL;
	HRESULT* pErrors = NULL;

	// Add an Item to the previous Group:
	hr = pIOPCItemMgt->AddItems(3, ItemArray, &pAddResult, &pErrors);
	if (hr != S_OK){
		printf("Failed call to AddItems function. Error code = %x\n", hr);
		exit(0);
	}

	// Server handle for the added item:
	hServerItem1 = pAddResult[0].hServer;
	hServerItem2 = pAddResult[1].hServer;
	hServerItem3 = pAddResult[2].hServer;

	//hClientItem = pAddResult[3].hServer;

	// release memory allocated by the server:
	CoTaskMemFree(pAddResult->pBlob);

	CoTaskMemFree(pAddResult);
	pAddResult = NULL;

	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Read from device the value of the item having the "hServerItem" server 
// handle and belonging to the group whose one interface is pointed by
// pGroupIUnknown. The value is put in varValue. 
//
void ReadItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT& varValue)
{
	// value of the item:
	OPCITEMSTATE* pValue = NULL;

	//get a pointer to the IOPCSyncIOInterface:
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**) &pIOPCSyncIO);

	// read the item value from the device:
	HRESULT* pErrors = NULL; //to store error code(s)
	HRESULT hr = pIOPCSyncIO->Read(OPC_DS_DEVICE, 1, &hServerItem, &pValue, &pErrors);
	_ASSERT(!hr);
	_ASSERT(pValue!=NULL);

	varValue = pValue[0].vDataValue;

	//Release memeory allocated by the OPC server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;

	CoTaskMemFree(pValue);
	pValue = NULL;

	// release the reference to the IOPCSyncIO interface:
	pIOPCSyncIO->Release();
}

//Write 
void WriteItem(IUnknown* pGroupIUnknown, OPCHANDLE hServerItem, VARIANT& varValue)
{
	//get a pointer to the IOPCSyncIOInterface:
	IOPCSyncIO* pIOPCSyncIO;
	pGroupIUnknown->QueryInterface(__uuidof(pIOPCSyncIO), (void**) &pIOPCSyncIO);

	// read the item value from the device:

	HRESULT* pErrors = NULL;

	HRESULT hr = pIOPCSyncIO->Write(1, &hServerItem, &varValue, &pErrors);
	if (hr != S_OK) { //  está retornando S_FALSE
		printf("Failed to send message %x.\n", hr);
		exit(0);
	}

	//Release memeory allocated by the OPC server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;

	// release the reference to the IOPCSyncIO interface:
	pIOPCSyncIO->Release();
}

///////////////////////////////////////////////////////////////////////////
// Remove the item whose server handle is hServerItem from the group
// whose IOPCItemMgt interface is pointed by pIOPCItemMgt
//
void RemoveItems(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE hServerItem1, OPCHANDLE hServerItem2, OPCHANDLE hServerItem3)
{
	// server handle of items to remove:
	OPCHANDLE hServerArray[4];
	hServerArray[0] = hServerItem1;
	hServerArray[1] = hServerItem2;
	hServerArray[2] = hServerItem3;
	//hServerArray[3] = hClientItem;
	
	//Remove the item:
	HRESULT* pErrors; // to store error code(s)
	HRESULT hr = pIOPCItemMgt->RemoveItems(3, hServerArray, &pErrors);
	_ASSERT(!hr);

	//release memory allocated by the server:
	CoTaskMemFree(pErrors);
	pErrors = NULL;
}

////////////////////////////////////////////////////////////////////////
// Remove the Group whose server handle is hServerGroup from the server
// whose IOPCServer interface is pointed by pIOPCServer
//
void RemoveGroup (IOPCServer* pIOPCServer, OPCHANDLE hServerGroup)
{
	// Remove the group:
	HRESULT hr = pIOPCServer->RemoveGroup(hServerGroup, FALSE);
	if (hr != S_OK){
		if (hr == OPC_S_INUSE)
			printf ("Failed to remove OPC group: object still has references to it.\n");
		else printf ("Failed to remove OPC group. Error code = %x\n", hr);
		exit(0);
	}
}


/* THREAD SECUNDARIA*/
DWORD WINAPI ServerTCPIP(LPVOID index) {
	
	WSADATA wsaData;
  	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(IP_ADDR, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	printf("Servidor TCP/IP: Aguardando conexões...\n");

	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);

	if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
		printf( "Listen failed with error: %ld\n", WSAGetLastError() );
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
		
	// Temporary socket for accepting connections:
	SOCKET ClientSocket;
	ClientSocket = INVALID_SOCKET;

	char recvbuf[DEFAULT_BUFLEN];
	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	while(1){
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		printf("New connection accepted..\n");

		while(true) {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);

		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			//puts(recvbuf);

			if( recvbuf[0] == 'e'){
			memset(recvbuf, 0, DEFAULT_BUFLEN);
			printf("Ending connection...\n");
			closesocket(ClientSocket);
			// iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			exit(0);
			} else {
			
			command = recvbuf[0];
			flag = 0;
			//fazer verificação de msg. se tudo certo, recvbuf = 'ok'
			//memset(recvbuf, 0, DEFAULT_BUFLEN);
			printAnswer(recvbuf);

			iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			
			if (iSendResult == SOCKET_ERROR) {

				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;

			}else if (iResult == 0){

				printf("Connection closing...\n");
				break;

			}
			}
		} else {
		printf("recv failed: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
		break;
		}
		}
	}
	closesocket(ClientSocket);
	WSACleanup();


}

void printAnswer(char recvbuf[]) {
	switch (recvbuf[0]) {
	case 'a':
		recvbuf = "Todos os LEDs foram ligados.";
		break;
	case 'y':
		recvbuf = "Estado do LED amarelo alternado.";
		break;
	case 'g':
		recvbuf = "Estado do LED verde alternado.";
		break;
	case 'r':
		recvbuf = "Estado do LED vermelho alternado.";
		break;
	case 'o':
		recvbuf = "Todos os leds foram desligados.";
		break;
	case 'e':
		recvbuf = "Conexao terminada.";
		break;
	default:
		recvbuf = "Dados invalidos recebidos.";
	};

}
