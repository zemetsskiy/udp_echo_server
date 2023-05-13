#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <fstream>

// директива препроцессора, которая используется для указания компоновщику включить библиотеку winsock в процесс сборки программы
#pragma comment(lib, "ws2_32.lib")

#define BUFLEN 512

int get_config_data(std::string &server_port) {
    std::string filename = "../Debug/udp_server.cfg.TXT";

    // создание объекта класса ifstream для чтения данных из файла с заданным именем filename
    std::ifstream config(filename);
    if (!config.is_open()) {
        std::cerr << "Error: Unable to open config file " << filename << std::endl;
        return 1;
    }

    // config.peek() возвращает следующий символ в потоке config
    if (config.peek() == std::ifstream::traits_type::eof()) {
        std::cerr << "Error: Config file " << filename << " is empty" << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(config, line)) {
        if (line.find("server_port=") == 0) {
            // извлекаем подстроку, начиная с 12 символа
            server_port = line.substr(12);
            break;
        }
    }

    config.close();

    //std::cout << "Server port: " << server_port << std::endl;
    return 0;
}

int main(int argc, char** argv)
{
    // инициализация библиотеки сокетов


    // переменная типа WSADATA - структура, используется для хранения информации о версии и конфигурации библиотеки винсок
    WSADATA wsaData;
    // makeword определяет версию винсок, которую необходимо использовать
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // структура для представления адреса сокета
    struct addrinfo hints;
    struct addrinfo* result = NULL, * ptr = NULL;

    // заполняем структуру hints нулевыми байтами перед ее использованием (избежание ошибок из-за неопределенных значений в структуре)
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // дейтаграммный сокета
    hints.ai_protocol = IPPROTO_UDP; // протокол udp
    hints.ai_flags = AI_PASSIVE; // assign the address of the local host to the socket
    

    std::string port;
 
    if (argc == 1) {
        // нет аргументов
        get_config_data(port);
    }
    else {
        port = argv[1];
    }

    // переменная result будет содержать связанный список структур addrinfo
    iResult = getaddrinfo(NULL, port.c_str(), &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        // очищаем все ресурсы, связанные с инициализацией сокетов
        WSACleanup();
        return 1;
    }

    SOCKET sock = INVALID_SOCKET;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        //std::cout << "\nai_family: " << ptr->ai_family << " | "<< "ai_socktype: " << ptr->ai_socktype << " | " << "ai_protocol: " << ptr->ai_protocol << " | " << "ai_addr: " << ptr->ai_addr << std::endl;
        //char ip_str[INET_ADDRSTRLEN];
        //struct sockaddr_in* sa_in = (struct sockaddr_in*)ptr->ai_addr;
        //inet_ntop(AF_INET, &(sa_in->sin_addr), ip_str, INET_ADDRSTRLEN);
        //std::cout << "IP address: " << ip_str << std::endl;

        sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock == INVALID_SOCKET) {
            std::cerr << "socket creating failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return 1;
        }

        // связываем сокет с локальным адресом
        iResult = bind(sock, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(sock);
            sock = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result); 

    if (sock == INVALID_SOCKET) {
        std::cerr << "Unable to bind socket" << std::endl;
        WSACleanup();
        return 1;
    }

    char recvbuf[BUFLEN];
    int recvbuflen = BUFLEN;
    struct sockaddr_in clientaddr; // структура для хранения адреса клиента
    int clientaddrlen = sizeof(clientaddr);
    int iSendResult;


    printf("Server is running on %s port\n\n", port.c_str());
    std::cout << "Waiting for data..." << std::endl;

    while (true) {
        // принимаем данные из сокета
        iResult = recvfrom(sock, recvbuf, recvbuflen, 0, (struct sockaddr*)&clientaddr, &clientaddrlen);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "recvfrom failed: " << WSAGetLastError() << std::endl;
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        // network to ascii конвертирует Ipv4 бинарный адрес в строковое представление (*.*.*.* нотация)
        // ntohs преобразует 16-битное число (с обратным порядком байтов) в порядок байтов хоста (с прямым порядком байтов)
        std::cout << "Received data from " << inet_ntoa(clientaddr.sin_addr) << ":" << ntohs(clientaddr.sin_port) << std::endl;
        std::cout << "Message: " << recvbuf << std::endl;

        iSendResult = sendto(sock, recvbuf, iResult, 0, (struct sockaddr*)&clientaddr, clientaddrlen);
        if (iSendResult == SOCKET_ERROR) {
            std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
            closesocket(sock);
            WSACleanup();
            return 1;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}