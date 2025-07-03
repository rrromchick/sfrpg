#pragma once

#include "C_Base.h"

class C_Name : public C_Base {
public:
    C_Name() : C_Base(Component::Name)
    {}

    void readIn(std::stringstream& l_stream) { l_stream >> m_name; }

    const std::string& getName() { return m_name; }
    void setName(const std::string& l_name) { m_name = l_name; }
private:
    std::string m_name;
};