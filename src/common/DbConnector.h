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
    bool Close();
    bool Execute(const std::string& l_sql);
    DbResult Query(const std::string& l_sql);
private:
    sqlite3* m_db;

    static int callback(void* l_data, int argc, char** argv, char** l_azColName);
};