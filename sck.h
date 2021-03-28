//
// Created by nbdy on 23.03.21.
//
/*
 * MIT License
 *
 * Copyright libsck 2021 Pascal Eberlein
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LIBTCPSH_TCPSH_H
#define LIBTCPSH_TCPSH_H

#define DEFAULT_BUFFER_SIZE 4096

#include <map>
#include <cerrno>
#include <thread>
#include <utility>
#include <unistd.h>
#include <functional>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <ohlog.h>

namespace nbdy {
    typedef void (*tNewConnectionCallback)(const std::string& host, int port, int fd, void* ctx);

    enum SocketContainerState {
        NONE=0, LISTENING, CONNECTING, CONNECTED
    };

    class SocketContainer {
        bool doRun = false;

        int descriptor = 0;

        struct sockaddr_in address {};
        int addressLength{};

        uint8_t domain = AF_INET;
        uint8_t type = SOCK_STREAM;

        int socketOptions = 1;

        int fd = 0;

        char incomingBuffer[DEFAULT_BUFFER_SIZE] {};

        SocketContainerState state = NONE;

        std::string exitReason;

        bool setExitReason(const std::string& reason) {
            exitReason = reason;
            doRun = false;
            return false;
        };

    public:
        SocketContainer() = default;
        SocketContainer(uint8_t domain, uint8_t type): domain(domain), type(type) {}

        ~SocketContainer() {
            closeSocket(descriptor);
        }

        bool listenOn(int port, tNewConnectionCallback newConnectionCallback, const std::string& host="127.0.0.1"){
            address.sin_family = domain;
            address.sin_addr.s_addr = inet_addr(host.c_str());
            address.sin_port = htons(port);

            if(bind(descriptor, (struct sockaddr*) &address, addressLength) < 0) {
                perror("Bind error");
                return setExitReason("Could not bind to '" + host + ":" + std::to_string(port) + "'");
            }

            if(listen(descriptor, 0) < 0) {
                perror("Listen error");
                return setExitReason("Could not listen");
            }

            state = LISTENING;

            while(doRun) {
                fd = accept(descriptor, (struct sockaddr*)&address, (socklen_t*)&addressLength);
                if(fd < 0) continue;
                newConnectionCallback(inet_ntoa(address.sin_addr), port, fd, this);
                closeSocket(fd);
            }

            state = NONE;

            return true;
        }

        bool connectTo(sockaddr_in addr, tNewConnectionCallback newConnectionCallback) {
            state = CONNECTING;
            if(connect(descriptor, (sockaddr*) &addr, sizeof(sockaddr)) < 0) {
                perror("Connect error");
                state = NONE;
                return setExitReason("Could not connect.");
            }
            state = CONNECTED;

            newConnectionCallback(inet_ntoa(addr.sin_addr), addr.sin_port, descriptor, this);

            return true;
        }

        bool connectTo(const std::string& host, int port, tNewConnectionCallback newConnectionCallback, uint8_t family=AF_INET) {
            sockaddr_in addr {};
            inet_aton(host.c_str(), &addr.sin_addr);
            addr.sin_port = htons(port);
            addr.sin_family = family;
            return connectTo(addr, newConnectionCallback);
        }

        bool create() {
            descriptor = socket(domain, type, 0);
            addressLength = sizeof(address);
            if(descriptor == -1) {
                exitReason = "Creation failed";
                return false;
            }
            if(setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &socketOptions, sizeof(socketOptions))){
                exitReason = "SetSockOpt failed";
                return false;
            }

            doRun = true;

            return true;
        }

        [[nodiscard]] std::string getExitReason() const {
            return exitReason;
        };

        [[nodiscard]] SocketContainerState getState() const {
            return state;
        }

        [[nodiscard]] std::string getStateStr() const {
            switch(state) {
                case NONE:
                    return "none";
                case LISTENING:
                    return "listening";
                case CONNECTING:
                    return "connecting";
                case CONNECTED:
                    return "connected";
            }
        }

        [[nodiscard]] bool getDoRun() const {
            return doRun;
        }

        static void closeSocket(int descriptor) {
            shutdown(descriptor, SHUT_RDWR);
            close(descriptor);
        }

        static void writeTo(int fileDescriptor, const std::string& data) {
            write(fileDescriptor, data.c_str(), data.size());
        }

        void stop() {
            doRun = false;
        }
    };
}

#endif //LIBTCPSH_TCPSH_H
