#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <mutex>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "database.h"

#include <httplib.h> // Using cpp-httplib for HTTP server

std::mutex cout_mutex;

void handle_client(int client_socket) {
    char buffer[1024];
    while (true) {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Client disconnected or error occurred." << std::endl;
            break;
        }
        buffer[bytes_received] = '\0';
        std::string rfid_tag(buffer);

        // Record race event for RFID tag read
        if (!record_race_event(rfid_tag, "checkpoint")) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Failed to record race event for tag: " << rfid_tag << std::endl;
        } else {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "RFID tag read and recorded: " << rfid_tag << std::endl;
        }
    }
    close(client_socket);
}

int main() {
    if (!open_database("race_system.db")) {
        std::cerr << "Failed to open database." << std::endl;
        return 1;
    }

    if (!create_tables()) {
        std::cerr << "Failed to create database tables." << std::endl;
        return 1;
    }

    // Start HTTP server for registration API
    httplib::Server svr;

    svr.Post("/api/register", [](const httplib::Request& req, httplib::Response& res) {
        auto json = nlohmann::json::parse(req.body, nullptr, false);
        if (json.is_discarded()) {
            res.status = 400;
            res.set_content("Invalid JSON", "text/plain");
            return;
        }
        std::string name = json.value("name", "");
        int age = json.value("age", 0);
        std::string gender = json.value("gender", "");
        std::string rfidTag = json.value("rfidTag", "");

        if (name.empty() || age <= 0 || gender.empty()) {
            res.status = 400;
            res.set_content("Missing required fields", "text/plain");
            return;
        }

        if (register_participant(name, age, gender, rfidTag)) {
            res.status = 200;
            res.set_content("Participant registered successfully", "text/plain");
        } else {
            res.status = 500;
            res.set_content("Failed to register participant", "text/plain");
        }
    });

    // Start RFID TCP server in separate thread
    std::thread rfid_thread([]() {
        const char* ip_address = "192.168.1.116";
        const int port = 9090;

        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            std::cerr << "Failed to create socket." << std::endl;
            return;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip_address, &server_addr.sin_addr);

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Bind failed." << std::endl;
            close(server_fd);
            return;
        }

        if (listen(server_fd, 5) < 0) {
            std::cerr << "Listen failed." << std::endl;
            close(server_fd);
            return;
        }

        std::cout << "RFID TCP Server listening on " << ip_address << ":" << port << std::endl;

        while (true) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_fd, (sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                std::cerr << "Failed to accept client." << std::endl;
                continue;
            }
            std::thread client_thread(handle_client, client_socket);
            client_thread.detach();
        }

        close(server_fd);
    });

    // Start HTTP server on port 8080
    std::cout << "HTTP server listening on port 8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    rfid_thread.join();

    close_database();
    return 0;
}

int main() {
    // The main function content has been moved above to avoid duplication.
    return 0;
}
