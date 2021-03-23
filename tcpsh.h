//
// Created by nbdy on 23.03.21.
//
/*
 * MIT License
 *
 * Copyright libtcpsh 2021 Pascal Eberlein
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

#include <map>
#include <thread>
#include <utility>
#include <unistd.h>
#include <functional>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <ohlog.h>

namespace rshell {
    typedef void (*TCPShellCallback)(const std::string host, int port, const std::string& data);
    typedef std::map<std::string, TCPShellCallback> tcpShellCallbacks;

    template<typename DATATYPE>
    class TCPShell {
    public:
        TCPShell(std::string host, int port): host(std::move(host)), port(port), log(ohlog::Logger::get()){};

        void start() {
            onStart();
            run();
            onStop();
        };

        void run() {
            log->i(LOG_TAG, "Starting");
            struct sockaddr_in address {};
            int readSize, s = 0;
            int addressLength = sizeof(address);
            char buffer[4096];
            int opt = 1;
            int sockedDescriptor = socket(AF_INET, SOCK_STREAM, 0);
            if(sockedDescriptor == 0){
                exitReason = "Could not create socket descriptor";
                return;
            }

            if(setsockopt(sockedDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
                exitReason = "Setting usage of '" + host + ":" + std::to_string(port) + "' failed";
                return;
            }

            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port);

            if(bind(sockedDescriptor, (struct sockaddr*) &address, addressLength) < 0) {
                exitReason = "Could not bind to '" + host + ":" + std::to_string(port) + "'";
                return;
            }

            if(listen(sockedDescriptor, 0) < 0) {
                exitReason = "Could not listen.";
                return;
            }

            doRun = true;
            log->i(LOG_TAG, "Listening on '%s:%i' for connections...", host.c_str(), port);
            while(doRun) {
                s = accept(sockedDescriptor, (struct sockaddr*)&address, (socklen_t*)&addressLength);
                if(s < 0) {
                    log->w(LOG_TAG, "Could not accept connection from '%s:%i'", inet_ntoa(address.sin_addr), address.sin_port);
                    continue;
                }

                log->i(LOG_TAG, "New connection from: '%s:%i'", inet_ntoa(address.sin_addr), address.sin_port);
                readSize = read(s, buffer, sizeof(buffer));
                std::stringstream data;
                data << buffer;
                log->d(LOG_TAG, "Read '%s' which is %i bytes. Processing.", data.str().c_str(), readSize);
                auto obj = parseData(buffer);
            }
        };

        [[nodiscard]] std::string getExitReason() const {
            return exitReason;
        };

    protected:
        ohlog::Logger* log;

        virtual void onStart() {}
        virtual void onStop() {}

        virtual DATATYPE parseData(char* buffer){
            return (DATATYPE) buffer;
        };

        virtual void processData(DATATYPE data){};

    private:
        bool doRun = false;

        std::string host;
        int port;

        std::string exitReason;

        std::string LOG_TAG = "TCPShell";

        tcpShellCallbacks callbacks;
    };
}

#endif //LIBTCPSH_TCPSH_H
