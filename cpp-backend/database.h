#ifndef DATABASE_H
#define DATABASE_H

#include <string>

bool open_database(const char* filename);
void close_database();
bool create_tables();

bool register_participant(const std::string& name, int age, const std::string& gender, const std::string& rfid_tag);
bool record_race_event(const std::string& rfid_tag, const std::string& event_type);

#endif // DATABASE_H
