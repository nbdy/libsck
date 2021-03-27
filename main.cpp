#include <iostream>
#include <cstring>
#include "sck.h"

class SenderSocket: public nbdy::SocketContainer {
public:
    SenderSocket(): nbdy::SocketContainer() {
        if(create()) std::cout << "Created SenderSocket" << std::endl;
        else std::cout << "Could not create SenderSocket" << std::endl;
    }
};

class ReceiverSocket: public nbdy::SocketContainer {
public:
    ReceiverSocket(): nbdy::SocketContainer() {
        if(create()) std::cout << "Created ReceiverSocket" << std::endl;
        else std::cout << "Could not create ReceiverSocket" << std::endl;
    }
};

int main() {
    std::thread receiverThread([] {
        ReceiverSocket rs;
        rs.listenOn(6666, [](const std::string& host, int port, int fd, void* ctx){
            auto s = (ReceiverSocket*) ctx;
            char buffer[1024] {};
            std::cout << "New connection from: " << host << ":" << std::to_string(port) << std::endl;
            while(read(fd, buffer, 1024) != 0 && s->getDoRun()) {
                std::string data(buffer);
                if(!data.empty()) std::cout << "\tRead: " << data << std::endl;
            }
            std::cout << "Done receiving data." << std::endl;
        });
    });

    std::thread senderThread([]{
        SenderSocket ss;
        ss.connectTo("127.0.0.1", 6666, [](const std::string& host, int port, int fd, void* ctx){
            auto s = (SenderSocket*) ctx;
            std::cout << "Connected to: " << host << ":" << std::to_string(port) << std::endl;
            int i = 0;
            std::string data;
            while(s->getDoRun()) {
                data = std::to_string(i);
                SenderSocket::writeTo(fd, data);
                sleep(1);
                i++;
            }
            std::cout << "Done sending data." << std::endl;
        });
    });

    while(senderThread.joinable() || receiverThread.joinable()) {
        usleep(50000);
    }

    return 0;
}
