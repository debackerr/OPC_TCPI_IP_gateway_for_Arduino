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
#undef UNICODE

/* ----- AREA DE INCLUDE ----- */
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

wchar_t ITEM_ID1[] = L"ArduinoSerial0.lr"; //ID do item a ser lido do servidor
wchar_t ITEM_ID2[] = L"ArduinoSerial0.lg"; //ID do item a ser lido do servidor
wchar_t ITEM_ID3[] = L"ArduinoSerial0.ly"; //ID do item a ser lido do servidor

wchar_t ITEM_CLIENTID[] = L"ArduinoSerial0.cor"; //ID do item a ser escrito no servidor

IOPCServer* pIOPCServer = NULL;   //pointer to IOPServer interface
IOPCItemMgt* pIOPCItemMgt = NULL; //pointer to IOPCItemMgt interface

OPCHANDLE hClientItem;

VARIANT varOFF;
VARIANT varON;
VARIANT varCOR;

// HANDLE MUTEX
HANDLE hMutexStatus;
HANDLE hMutexSetup;

// HANDLE EVENT
HANDLE hEvent;

/* ----- PRIMEIRA THREAD: OPC CLIENTE ----- */
void main(void) {
	// Definicao do nome do terminal
	SetConsoleTitle(L"Terminal OPC Cliente");

	// Criacao da thread secundaria
	HANDLE hServerTCPIP, hEscritaSincrona;

	DWORD dwServerTCPIPId, dwEscritaSincronaId;
	DWORD dwExitCode = 0;

	DWORD ret;

	int i = 1;
	int nTipoEvento;

	bool exit = false;

	/* Add: Valor1, Valor2, Valor3 para os LEDS, qual o tipo? */

	hServerTCPIP = CreateThread(NULL, 0, ServerTCPIP, (LPVOID)i, 0, &dwServerTCPIPId);
	if(hServerTCPIP)  printf("Thread %d criada com ID = %0d \n", i, dwServerTCPIPId);


	// Criacao objetos do tipo Mutex
	hMutexStatus = CreateMutex(NULL, FALSE, L"MutexStatus");
	GetLastError();

	hMutexSetup = CreateMutex(NULL, FALSE, L"MutexSetup");
	GetLastError();

	// Criacao objetos do tipo Eventos
	hEvent = CreateEvent(NULL, FALSE, FALSE, L"Escrita");
	GetLastError();

	// Group and itens server
	OPCHANDLE hServerGroup; // server handle to the group

	OPCHANDLE hServerItem1;  // server handle to the item
	OPCHANDLE hServerItem2;  // server handle to the item
	OPCHANDLE hServerItem3;  // server handle to the item

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
	wcstombs_s(&m, buf, 100, ITEM_CLIENTID, _TRUNCATE);
	printf("Adding the item %s to the group...\n", buf);
    AddTheItems(pIOPCItemMgt, hServerItem1, hServerItem2, hServerItem3, hClientItem);

	int bRet;
	MSG msg;
	//Synchronous read of the device�s item value.
	VARIANT varValue; //to store the read value
	VariantInit(&varValue);
	varValue.vt = VT_R8;
	printf ("Reading synchronously...\n");
	printf("\n");


	while(!exit) {
		// Imprime dados atualizados
		printData();
	
		WaitForSingleObject(hMutexSetup, INFINITE); 

		// Comando para dar exit como vamos verificar ?
		if ( == 'e') {
			exit = true;
		}
		// Comando para leitura do LED VERMELHO
		else if( == 'r') {
			WaitForSingleObject(hMutexStatus, INFINITE);

			ReadItem(pIOPCItemMgt, hServerItem1, varValue);

			if (varValue.fltVal == -1) {
				cout << "The red led is ON" << endl;
				WriteItem(pIOPCItemMgt, hClientItem, varON); 
			}
			else {
				cout << "The red led is OFF" << endl;
				WriteItem(pIOPCItemMgt, hClientItem, varOFF); 
			}

			ReleaseSemaphore(hMutexStatus, 1, NULL);
		}
		// Comando para leitura do LED VERDE
		else if( == 'g') {
			WaitForSingleObject(hMutexStatus, INFINITE);

			ReadItem(pIOPCItemMgt, hServerItem2, varValue);

			if (varValue.fltVal == -1) {
				cout << "The green led is ON" << endl;
				WriteItem(pIOPCItemMgt, hClientItem, varON); 
			}
			else {
				cout << "The green led is OFF" << endl;
				WriteItem(pIOPCItemMgt, hClientItem, varOFF); 
			}

			ReleaseSemaphore(hMutexStatus, 1, NULL);
		}
		// Comando para leitura do LED AMARELO
		else if( == 'y') {
			WaitForSingleObject(hMutexStatus, INFINITE);

			ReadItem(pIOPCItemMgt, hServerItem2, varValue);

			if (varValue.fltVal == -1) {
				cout << "The red led is ON" << endl;
				WriteItem(pIOPCItemMgt, hClientItem, varON); 
			}
			else {
				cout << "The red led is OFF" << endl;
				WriteItem(pIOPCItemMgt, hClientItem, varOFF); 
			}

			ReleaseSemaphore(hMutexStatus, 1, NULL);
		}
		Sleep(100);
	}

	// for (i=0; i<10; i++) {

	// 	ReadItem(pIOPCItemMgt, hServerItem1, varValue);
	//   // print the read value:
	//   if (varValue.fltVal == -1) {
	// 	  cout << "The red led is ON" << endl;
	//   }
	//   else {
	// 	  cout << "The red led is OFF" << endl;
	//   }
	//   ReadItem(pIOPCItemMgt, hServerItem2, varValue);
	//   // print the read value:
	//   //cout << "Read value - GREEN LED:  " << varValue.fltVal << endl;
	//   if (varValue.fltVal == -1) {
	// 	  cout << "The green led is ON" << endl;
	//   }
	//   else {
	// 	  cout << "The green led is OFF" << endl;
	//   }
	//   ReadItem(pIOPCItemMgt, hServerItem3, varValue);
	//   // print the read value:
	//   //cout << "Read value - YELLOW LED: " << varValue.fltVal << endl;
	//   if (varValue.fltVal == -1) {
	// 	  cout << "The yellow led is ON" << endl;
	//   }
	//   else {
	// 	  cout << "The yellow led is OFF" << endl;
	//   }
	//   // wait 0.5 second
	//   Sleep(500); 
	//   printf("\n");

	// }
	// Inicialização das VARIANTs que representam "false" e "true" para escrever
	/*VariantInit(&varOFF);
	varOFF.vt = VT_R8;
	varOFF.boolVal = 0;
	varOFF.fltVal = 0;

	VariantInit(&varON);
	varON.vt = VT_R8;
	varON.boolVal = 1;
	varON.fltVal = 1;
	*/

	// // Ligando somente o led vermelho
	// char cor;
	// cout << "Escolha o LED: ";
	// cin >> cor;
	// cout << endl;

	// VariantInit(&varCOR);
	// varCOR.vt = VT_I1;
	// varCOR.bVal = cor;


	//WaitForSingleObject(hMutexSetup, INFINITE); 

	//manda 1 e depois manda 0
	// WriteItem(pIOPCItemMgt, hClientItem, varCOR); 
	// ReadItem(pIOPCItemMgt, hClientItem, varValue);
	// printf("Escreveu: %6.1f\n", varValue.fltVal);
	// Sleep(3000);

	//ReleaseSemaphore(hMutexSetup, 1, NULL);


	// Remove the OPC item:
	printf("Removing the OPC itens...\n");
	RemoveItems(pIOPCItemMgt, hServerItem1, hServerItem2, hServerItem3, hClientItem);

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
	string str = "Dados atualizados: ";

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

void AddTheItems(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE& hServerItem1, OPCHANDLE& hServerItem2, OPCHANDLE& hServerItem3, OPCHANDLE& hClientItem)
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
		{
			/*szAccessPath*/ L"",
			/*szItemID*/ ITEM_CLIENTID,
			/*bActive*/ TRUE,
			/*hClient*/ 3,
			/*dwBlobSize*/ 0,
			/*pBlob*/ NULL,
			/*vtRequestedDataType*/ VT,
			/*wReserved*/0
		} };

	//Add Result:
	OPCITEMRESULT* pAddResult=NULL;
	HRESULT* pErrors = NULL;

	// Add an Item to the previous Group:
	hr = pIOPCItemMgt->AddItems(4, ItemArray, &pAddResult, &pErrors);
	if (hr != S_OK){
		printf("Failed call to AddItems function. Error code = %x\n", hr);
		exit(0);
	}

	// Server handle for the added item:
	hServerItem1 = pAddResult[0].hServer;
	hServerItem2 = pAddResult[1].hServer;
	hServerItem3 = pAddResult[2].hServer;

	hClientItem = pAddResult[3].hServer;

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
void RemoveItems(IOPCItemMgt* pIOPCItemMgt, OPCHANDLE hServerItem1, OPCHANDLE hServerItem2, OPCHANDLE hServerItem3, OPCHANDLE hClientItem)
{
	// server handle of items to remove:
	OPCHANDLE hServerArray[4];
	hServerArray[0] = hServerItem1;
	hServerArray[1] = hServerItem2;
	hServerArray[2] = hServerItem3;
	hServerArray[3] = hClientItem;
	
	//Remove the item:
	HRESULT* pErrors; // to store error code(s)
	HRESULT hr = pIOPCItemMgt->RemoveItems(4, hServerArray, &pErrors);
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
	/*------------------------------------------------------------------------------*/
	/*Declarando variaveis locais*/
	WSADATA             wsaData;


}
