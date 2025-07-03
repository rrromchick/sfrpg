#pragma once

#include "Message.h"

class Observer {
public:
    virtual ~Observer() {}
    virtual void notify(const Message& l_message) = 0;
};