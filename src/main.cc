#include "../includes/manager_server.hpp"

int main(int argc, char** argv){
    ManagerServer* server= new ManagerServer();
    server->Add("127.0.0.1", 12345, "127.0.0.1", 5555);
    server->Handle();
    delete server;
    return 0;
}