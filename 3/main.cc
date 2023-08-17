#include "echo.h"
#include "address.h"
#include "eventloop.h"

using namespace tiny_muduo;


int main(int argc, char* argv[]) {
    if (argc <= 1)
    {
        printf( "Usage: %s portname\n", argv[0] );
        return 0;
    }

    EventLoop loop;
    Address listen_address(argv[1]);
    EchoServer server(&loop, listen_address);
    server.Start();
    loop.Loop();
    return 0;
}