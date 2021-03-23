#include "tcpsh.h"

class MyTCPShell: public rshell::TCPShell<char*> {
public:
    MyTCPShell(const std::string& host, int port): rshell::TCPShell<char *>(host, port) {}

    void processData(char *data) override {
        log->i("MydTCPShell", "Data: %s", data);
    }
};

int main() {
    MyTCPShell shell("127.0.0.1", 5000);
    shell.start();
    return 0;
}
