#include <iostream>
#include <sqlite3.h>

sqlite3* db;

bool open_database(const char* filename) {
    int rc = sqlite3_open(filename, &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    return true;
}

void close_database() {
    sqlite3_close(db);
}

bool create_tables() {
    const char* sql_participants = R"(
        CREATE TABLE IF NOT EXISTS participants (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            age INTEGER,
            gender TEXT,
            rfid_tag TEXT UNIQUE,
            registration_time DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";

    const char* sql_race_events = R"(
        CREATE TABLE IF NOT EXISTS race_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            participant_id INTEGER,
            event_type TEXT,
            event_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(participant_id) REFERENCES participants(id)
        );
    )";

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db, sql_participants, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }

    rc = sqlite3_exec(db, sql_race_events, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

bool register_participant(const std::string& name, int age, const std::string& gender, const std::string& rfid_tag) {
    const char* sql = "INSERT INTO participants (name, age, gender, rfid_tag) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, age);
    sqlite3_bind_text(stmt, 3, gender.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, rfid_tag.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool record_race_event(const std::string& rfid_tag, const std::string& event_type) {
    // Find participant id by RFID tag
    const char* sql_find = "SELECT id FROM participants WHERE rfid_tag = ?;";
    sqlite3_stmt* stmt_find;
    int rc = sqlite3_prepare_v2(db, sql_find, -1, &stmt_find, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare find statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt_find, 1, rfid_tag.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt_find);
    if (rc != SQLITE_ROW) {
        std::cerr << "Participant not found for RFID tag: " << rfid_tag << std::endl;
        sqlite3_finalize(stmt_find);
        return false;
    }
    int participant_id = sqlite3_column_int(stmt_find, 0);
    sqlite3_finalize(stmt_find);

    // Insert race event
    const char* sql_insert = "INSERT INTO race_events (participant_id, event_type) VALUES (?, ?);";
    sqlite3_stmt* stmt_insert;
    rc = sqlite3_prepare_v2(db, sql_insert, -1, &stmt_insert, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare insert statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_int(stmt_insert, 1, participant_id);
    sqlite3_bind_text(stmt_insert, 2, event_type.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt_insert);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute insert statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt_insert);
        return false;
    }
    sqlite3_finalize(stmt_insert);
    return true;
}
