#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <signal.h>

#include <ads1256.h>


int main() {
    // Ignore SIGPIPE so the app doesn't crash when Python disconnects
    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    const char* socket_path = "/tmp/spi_socket";
    unlink(socket_path); // Remove if exists
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    std::cout << "Waiting for Python to connect..." << std::endl;
    while (true) {
        int client_sock = accept(server_fd, NULL, NULL);
        
        if (client_sock < 0) continue;
        std::cout << "Python connected!" << std::endl;

        ADS1256 adc;

        auto next_tick = std::chrono::steady_clock::now();
        double interval = 1000000.0 / 100.0; // 100Hz = 10,000 microseconds

        while (true) {
            next_tick += std::chrono::microseconds((int)interval);

            int16_t sensor_data = 42;

            // If send fails, break the inner loop to wait for a new connection
            if (send(client_sock, &sensor_data, sizeof(sensor_data), 0) < 0) {
                std::cerr << "Python disconnected." << std::endl;
                close(client_sock);
                break; 
            }

            std::this_thread::sleep_until(next_tick);
        }
    }

    close(server_fd);
    return 0;
}