#include "DbConnector.h"

DbConnector::DbConnector() : m_db(nullptr)
{}

DbConnector::~DbConnector() { if (m_db) { Close(); } }

bool DbConnector::Open(const std::string& l_dbPath) {
    if (m_db) Close();

    int res = sqlite3_open(l_dbPath.c_str(), &m_db);
    if (res != SQLITE_OK) {
        std::cerr << "Failed opening database: " << l_dbPath << std::endl;
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }
    std::cout << "Successfully opened database." << std::endl;
    return true;
}

bool DbConnector::Close() {
    if (!m_db) return false;

    int res = sqlite3_close(m_db);
    if (res != SQLITE_OK) {
        std::cerr << "Failed closing database." << std::endl;
        return false;
    }
    m_db = nullptr;
    return true;
}

bool DbConnector::Execute(const std::string& l_sql) {
    if (!m_db) return false;

    char* errMsg = nullptr;

    int res = sqlite3_exec(m_db, l_sql.c_str(), nullptr, 0, &errMsg);
    if (res != SQLITE_OK) {
        std::cout << "Failed to execute sql!" << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

DbResult DbConnector::Query(const std::string& l_sql) {
    if (!m_db) return {};

    DbResult result;
    char* errMsg = nullptr;

    int res = sqlite3_exec(m_db, l_sql.c_str(), callback, (void*)&result, &errMsg);
    if (res != SQLITE_OK) {
        std::cout << "Failed to execute query!" << std::endl;
        sqlite3_free(errMsg);
        return {};
    }
    return result;
}

int callback(void* l_data, int l_argc, char** l_argv, char** l_azColName) {
    DbResult* result = static_cast<DbResult*>(l_data);
    DbRow row;

    for (int i = 0; i < l_argc; ++i) {
        row[l_azColName[i]] = (l_argv[i] ? l_argv[i] : "");
    }
    result->push_back(row);
    return 0;
}