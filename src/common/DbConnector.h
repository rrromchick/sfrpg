#pragma once

#include "sqlite3.h"
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>

using DbRow = std::unordered_map<std::string, std::string>;
using DbResult = std::vector<DbRow>;

class DbConnector {
public:
    DbConnector();
    ~DbConnector();

    DbConnector(const DbConnector& l_other) = delete;
    DbConnector& operator =(const DbConnector& l_other) = delete;

    bool Open(const std::string& l_dbPath);
    bool Init();
    bool Close();
    bool Execute(const std::string& l_sql);
    DbResult Query(const std::string& l_sql);

    bool InsertIntoUsers(int l_id, const std::string& l_firstName, const std::string& l_lastName,
        const std::string& l_password, const std::string& l_email);
    bool DeleteFromUsers(int l_id = -1, const std::string& l_firstName = "", const std::string& l_lastName = "",
        const std::string& l_password = "", const std::string& l_email = "");
    bool UpdateUser(int l_id = -1, const std::string& l_firstName = "", const std::string& l_lastName = "",
        const std::string& l_password = "", const std::string& l_email = "");
    DbResult SelectFromUsers(int l_id = -1, const std::string& l_firstName = "", const std::string& l_lastName = "",
        const std::string& l_password = "", const std::string& l_email = "");
    
    bool InsertIntoMessages(int l_id, const std::string& l_timestamp, int l_sender, int l_receiver, const std::string& l_content);
    bool DeleteFromMessages(int l_id = -1, const std::string& l_timestamp = "", int l_sender = -1, int l_receiver = -1, const std::string& l_content = "");
    bool UpdateMessage(int l_id = -1, const std::string& l_timestamp = "", int l_sender = -1, int l_receiver = -1, const std::string& l_content = "");
    DbResult SelectFromMessages(int l_id = -1, const std::string& l_timestamp = "", int l_sender = -1, int l_receiver = -1, const std::string& l_content = "");
private:
    sqlite3* m_db;

    static int callback(void* l_data, int argc, char** argv, char** l_azColName);
};