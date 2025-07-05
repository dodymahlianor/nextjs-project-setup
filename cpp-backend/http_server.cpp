#include "database.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <httplib.h>

using json = nlohmann::json;

int main() {
    if (!open_database("race_system.db")) {
        std::cerr << "Failed to open database." << std::endl;
        return 1;
    }

    if (!create_tables()) {
        std::cerr << "Failed to create database tables." << std::endl;
        return 1;
    }

    httplib::Server svr;

    svr.Post("/api/register", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            std::string name = j.value("name", "");
            int age = j.value("age", 0);
            std::string gender = j.value("gender", "");
            std::string rfidTag = j.value("rfidTag", "");

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
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content("Invalid JSON", "text/plain");
        }
    });

    std::cout << "HTTP server listening on port 8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    close_database();
    return 0;
}
