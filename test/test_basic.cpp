// This serves as simple build test as well as memory/ub checking if asan and ubsan is enabled.

#include <apclient.hpp>
#include <chrono>
#include <cstdio>
#include <thread>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

#define usleep(usec) std::this_thread::sleep_for(std::chrono::microseconds(usec))

typedef websocketpp::server<websocketpp::config::asio> Server;

class TestServer final {
public:
    explicit TestServer(const uint16_t preferredPort, const uint16_t maxPort=0)
        : port(preferredPort), maxPort(maxPort == 0 ? preferredPort : maxPort)
    {
        server.set_error_channels(websocketpp::log::elevel::fatal);
        server.set_access_channels(websocketpp::log::alevel::none);
        server.init_asio();

        server.set_open_handler([this](const websocketpp::connection_hdl& hdl){on_open(hdl);});
    }

    ~TestServer()
    {
        try {
            if (running)
                stop();
        } catch (...) {}
    }

    void run()
    {
        running = true;
        while (true) {
            try {
                server.listen(port);
                break;
            } catch (const websocketpp::exception&) {
                if (port == maxPort)
                    throw;
                port += 1;
            }
        }
        server.start_accept();
        server.run();
    }

    void stop()
    {
        server.stop_listening();
        //server.stop();
        running = false;
    }

    bool is_listening() const
    {
        return server.is_listening();
    }

    uint16_t get_port() const
    {
        return port;
    }

private:
    Server server;
    uint16_t port;
    uint16_t maxPort;
    bool running = false;

    void on_open(const websocketpp::connection_hdl& hdl)
    {
        const std::string roomInfo = R""""(
[{
    "cmd": "RoomInfo",
    "seed_name": "seed_name",
    "time": 0,
    "version": {"major": 0, "minor": 6, "build": 3, "class": "Version"}
}]
)"""";
        server.send(hdl, roomInfo, websocketpp::frame::opcode::text);
    }
};

int main(int, char**)
{
    printf("Starting server...\n");
    TestServer server{38281, 38291};
    std::thread serverThread(&TestServer::run, &server);
    for (int i=0; i<10; i++) {
        usleep(2000); // wait for the server to be running before starting client
        if (server.is_listening())
            break;
    }
    if (!server.is_listening())
        throw std::runtime_error("Timeout starting server");
    printf("Server listening on %d\n", static_cast<int>(server.get_port()));

    bool error = false;
    bool connected = false;
    bool roomInfo = false;
    {
        const std::string uri = "ws://localhost:" + std::to_string(server.get_port());

        printf("Starting client for %s...\n", uri.c_str());
        APClient ap{"", "", uri};
        ap.set_socket_connected_handler([&connected]() {
            connected = true;
        });
        ap.set_socket_error_handler([&error](const std::string& msg) {
            printf("socket error: %s\n", msg.c_str());
            error = true;
        });
        ap.set_socket_disconnected_handler([]() {
            printf("socket disconnected\n");
        });
        ap.set_room_info_handler([&roomInfo]() {
            roomInfo = true;
        });
        for (int i = 0; i < 10000; ++i) {
            ap.poll();
            if (connected && roomInfo)
                break;
            usleep(1);
        }
        printf("Stopping client...\n");
        // FIXME: ~APClient can throw
    }

    printf("Stopping server...\n");
    server.stop();
    serverThread.join();

    if (!connected) {
        fprintf(stderr, "FAIL: Could not connect socket\n");
        return 1;
    }
    if (!roomInfo) {
        fprintf(stderr, "FAIL: Did not receive room info\n");
        return 1;
    }
    if (error) {
        fprintf(stderr, "FAIL: Error\n");
        return 1;
    }
    return 0;
}
