#include "application.h"
#include <iostream>

void printMenu() {

    std::cout << "Insira um dos comandos a seguir: " << std::endl;
    std::cout << "---------------------------------" << std::endl;
    std::cout << "a - ligar todos os LEDS." << std::endl;
    std::cout << "o - desligar todos os LEDS." << std::endl;
    std::cout << "r - ligar o LED vermelho." << std::endl;
    std::cout << "g - ligar o LED verde." << std::endl;
    std::cout << "y - ligar o LED amarelo." << std::endl;
    std::cout << "e - encerrar a comunicacao. " << std::endl;
    std::cout << "---------------------------------" << std::endl;
}

void runApp(char buf[]){
    //Gets commands from user
    bool invalid = true;
    printf(">>");

    while(invalid) {
        fgets(buf,512, stdin);

        switch (buf[0]){
        case 'a':
            invalid = false;
            break;
        case 'y':
            invalid = false;
            break;
        case 'g':
            invalid = false;
            break;
        case 'r':
            invalid = false;
            break;
        case 'o':
            invalid = false;
            break;
        case 'e':
            invalid = false;
            break;
        default:
            printf("Comando Inválido. Digite novamente.\n");
        };
        
    }
}
