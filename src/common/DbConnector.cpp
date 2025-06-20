#include "DbConnector.h"

DbConnector::DbConnector() : m_db(nullptr)
{}

DbConnector::~DbConnector() { if (m_db) { Close(); } }

bool DbConnector::Open(const std::string& l_dbPath) {
    if (m_db) Close();

    int rc = sqlite3_open(l_dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed opening database: " << l_dbPath << std::endl;
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }
    std::cout << "Successfully opened database." << std::endl;
    return true;
}

bool DbConnector::Init() {
    if (!m_db) return false;

    char* errMsg = nullptr;
    char* sql = "CREATE TABLE IF NOT EXISTS USER("
                "ID INT PRIMARY KEY NOT NULL"
                "FIRST_NAME VARCHAR(50) NOT NULL"
                "LAST_NAME VARCHAR(50) NOT NULL"
                "PASSWORD VARCHAR(24) NOT NULL"
                "EMAIL VARCHAR(50) NOT NULL)";
    int rc = sqlite3_exec(m_db, sql, nullptr, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed creating table user" << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(m_db);
        m_db = nullptr;
        delete sql;
        return false;
    }

    sql = "CREATE TABLE IF NOT EXISTS MESSAGE("
          "ID INT PRIMARY KEY NOT NULL"
          "TIMESTAMP TEXT NOT NULL"
          "SENDER INTEGER NOT NULL"
          "RECEIVER INTEGER NOT NULL"
          "CONTENT TEXT NOT NULL"
          "FOREIGN KEY (SENDER)"
          "REFERENCES USER(ID)"
          "FOREIGN KEY (RECEIVER)"
          "REFERENCES USER(ID))";
    rc = sqlite3_exec(m_db, sql, nullptr, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed creating table user" << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(m_db);
        m_db = nullptr;
        delete sql;
        return false;
    }
    return true;
}

bool DbConnector::Close() {
    if (!m_db) return false;

    int rc = sqlite3_close(m_db);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed closing database." << std::endl;
        return false;
    }
    m_db = nullptr;
    return true;
}

bool DbConnector::Execute(const std::string& l_sql) {
    if (!m_db) return false;

    char* errMsg = nullptr;

    int rc = sqlite3_exec(m_db, l_sql.c_str(), nullptr, 0, &errMsg);
    if (rc != SQLITE_OK) {
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

    int rc = sqlite3_exec(m_db, l_sql.c_str(), callback, (void*)&result, &errMsg);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to execute query!" << std::endl;
        sqlite3_free(errMsg);
        return {};
    }
    return result;
}

int DbConnector::callback(void* l_data, int l_argc, char** l_argv, char** l_azColName) {
    DbResult* result = static_cast<DbResult*>(l_data);
    DbRow row;

    for (int i = 0; i < l_argc; ++i) {
        row[l_azColName[i]] = (l_argv[i] ? l_argv[i] : "");
    }
    result->push_back(row);
    return 0;
}

DbResult DbConnector::SelectFromUsers(int l_id, const std::string& l_firstName, const std::string& l_lastName, const std::string& l_password, 
    const std::string& l_email) 
{
    std::string sql = "SELECT * FROM 'USER'";
    bool addAnd = false;
    if (l_id != -1) { sql += " WHERE ID=" + std::to_string(l_id); addAnd = true; }
    if (l_firstName != "") { sql += (addAnd ? " AND " : " "); sql += "FIRSTNAME='" + l_firstName + "'"; addAnd = true; }
    if (l_lastName != "") { sql += (addAnd ? " AND " : " "); sql += "LASTNAME='" + l_lastName + "'"; addAnd = true; }
    if (l_password != "") { sql += (addAnd ? " AND " : " "); sql += "PASSWORD='" + l_password + "'"; addAnd = true; }
    if (l_email != "") { sql += (addAnd ? " AND " : " "); sql += "EMAIL='" + l_email + "'"; }
    sql += ";";
    return Query(sql);
}

bool DbConnector::DeleteFromUsers(int l_id, const std::string& l_firstName, const std::string& l_lastName, const std::string& l_password, 
    const std::string& l_email) 
{
    std::string sql = "DELETE FROM 'USER'";
    bool addAnd = false;
    if (l_id != -1) { sql += " WHERE ID=" + std::to_string(l_id); addAnd = true; }
    if (l_firstName != "") { sql += (addAnd ? " AND " : " "); sql += "FIRSTNAME='" + l_firstName + "'"; addAnd = true; }
    if (l_lastName != "") { sql += (addAnd ? " AND " : " "); sql += "LASTNAME='" + l_lastName + "'"; addAnd = true; }
    if (l_password != "") { sql += (addAnd ? " AND " : " "); sql += "PASSWORD='" + l_password + "'"; addAnd = true; }
    if (l_email != "") { sql += (addAnd ? " AND " : " "); sql += "EMAIL='" + l_email + "'"; }
    sql += ";"; 
    return Execute(sql);
}

bool DbConnector::InsertIntoUsers(int l_id, const std::string& l_firstName, const std::string& l_lastName, const std::string& l_password, 
    const std::string& l_email) 
{
    std::string sql = "INSERT INTO 'USER' VALUES(";
    sql += l_id + ", '" + l_firstName + "', '" + l_lastName + "', '" + l_password + "', '" + l_email + "');";
    return Execute(sql);
}

bool DbConnector::UpdateUser(int l_id, const std::string& l_firstName, const std::string& l_lastName, const std::string& l_password, 
    const std::string& l_email) 
{
    if (l_id == -1) return false;

    std::string sql = "UPDATE 'USER'";
    bool addComma = false;
    if (l_firstName != "") { sql += "SET FIRSTNAME='" + l_firstName + "' "; addComma = true; }
    if (l_lastName != "") { sql += (addComma ? ", LASTNAME='" : "SET LASTNAME='") + l_lastName + "' "; addComma = true; }
    if (l_password != "") { sql += (addComma ? ", PASSWORD='" : "SET PASSWORD='") + l_password + "' "; addComma = true; }
    if (l_email != "") { sql += (addComma ? ", EMAIL='" : "SET EMAIL='") + l_email + "'"; }
    sql += " WHERE ID=" + std::to_string(l_id) + ";"; 
    return Execute(sql);
}

DbResult DbConnector::SelectFromMessages(int l_id, const std::string& l_timestamp, int l_sender, int l_receiver, const std::string& l_content) {
    std::string sql = "SELECT * FROM 'MESSAGE'";
    bool addAnd = false;
    if (l_id != -1) { sql += " WHERE ID=" + std::to_string(l_id); addAnd = true; }
    if (l_timestamp != "") { sql += (addAnd ? " AND " : " "); sql += "TIMESTAMP='" + l_timestamp + "'"; addAnd = true; }
    if (l_sender != -1) { sql += (addAnd ? " AND " : " "); sql += "SENDER=" + l_sender; addAnd = true; }
    if (l_receiver != -1) { sql += (addAnd ? " AND " : " "); sql += "RECEIVER=" + l_receiver; addAnd = true; }
    if (l_content != "") { sql += (addAnd ? " AND " : " "); sql += "CONTENT='" + l_content + "'"; }
    sql += ";";
    return Query(sql);
}

bool DbConnector::DeleteFromMessages(int l_id, const std::string& l_timestamp, int l_sender, int l_receiver, const std::string& l_content) {
    std::string sql = "DELETE FROM 'MESSAGE'";
    bool addAnd = false;
    if (l_id != -1) { sql += " WHERE ID=" + std::to_string(l_id); addAnd = true; }
    if (l_timestamp != "") { sql += (addAnd ? " AND " : " "); sql += "TIMESTAMP='" + l_timestamp + "'"; addAnd = true; }
    if (l_sender != -1) { sql += (addAnd ? " AND " : " "); sql += "SENDER=" + std::to_string(l_sender); addAnd = true; }
    if (l_receiver != -1) { sql += (addAnd ? " AND " : " "); sql += "RECEIVER=" + std::to_string(l_receiver); addAnd = true; }
    if (l_content != "") { sql += (addAnd ? " AND " : " "); sql += "CONTENT='" + l_content + "'"; }
    sql += ";"; 
    return Execute(sql);
}

bool DbConnector::InsertIntoMessages(int l_id, const std::string& l_timestamp, int l_sender, int l_receiver, const std::string& l_content) {
    std::string sql = "INSERT INTO 'MESSAGE' VALUES(";
    sql += l_id + ", '" + l_timestamp + "', '" + std::to_string(l_sender) + "', '" + std::to_string(l_receiver) + "', '" + l_content + "');";
    return Execute(sql);
}

bool DbConnector::UpdateMessage(int l_id, const std::string& l_timestamp, int l_sender, int l_receiver, const std::string& l_content) {
    if (l_id == -1) return false;

    std::string sql = "UPDATE 'MESSAGE'";
    bool addComma = false;
    if (l_timestamp != "") { sql += "SET TIMESTAMP='" + l_timestamp + "'"; addComma = true; }
    if (l_sender != -1) { sql += (addComma ? ", SENDER='" : "SET SENDER=") + std::to_string(l_sender); addComma = true; }
    if (l_receiver != -1) { sql += (addComma ? ", PASSWORD='" : "SET RECEIVER=") + std::to_string(l_receiver); addComma = true; }
    if (l_content != "") { sql += (addComma ? ", EMAIL='" : "SET CONTENT='") + l_content + "'"; }
    sql += " WHERE ID=" + std::to_string(l_id) + ";"; 
    return Execute(sql);
}