Para correta execução da projeto enviado, siga como abaixo:

1 - Instalar o servidor OPC para Arduino e executá-lo;

2 - Carregar o arquivo arduino.ino na sua placa pela IDE do controlador;

3 - Rodar o aplicativo ArduinoOPCServer

4 - Com o Visual Studio Community, abrir o arquivo SimpleOPCClient.vcproj na pasta "gateway_clienteTCP"
Em seguida, compilar o mesmo arquivo e executar. A partir desse momento é estabelecida a conexão
OPC entre o Arduino e o gateway, que aguarda as instruções vindas do cliente TCP/IP.

5 - Em um terminal de comando separado do Windows, acessar novamente o diretório da pasta
 "gateway_clienteTCP" pelo comando:
	
	cd <diretorio>

Em seguida, compilar o programa cliente com a seguinte instrução:

	 g++ -o cliente client.cpp -lws2_32

Por fim, execute o código do programa cliente no mesmo terminal digitando:

 	cliente.exe
