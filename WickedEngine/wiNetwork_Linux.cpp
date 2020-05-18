#include    "wiNetwork.h"

namespace wiNetwork
{
    void Initialize() {
        assert(false);
    }

    bool CreateSocket(Socket* sock) {
        assert(false);
    }

    bool Destroy(Socket* sock) {
        assert(false);
    }

    bool Send(const Socket* sock, const Connection* connection, const void* data, size_t dataSize) {
        assert(false);
    }

	bool ListenPort(const Socket* sock, uint16_t port) {
        assert(false);
    }

	bool CanReceive(const Socket* sock, long timeout_microseconds) {
        assert(false);
    }

	bool Receive(const Socket* sock, Connection* connection, void* data, size_t dataSize) {
        assert(false);
    }
};
