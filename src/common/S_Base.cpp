#include "S_Base.h"

S_Base::S_Base(const System& l_id, SystemManager* l_systemMgr) 
    : m_id(l_id), m_systemManager(l_systemMgr) 
{}

S_Base::~S_Base() { purge(); }

bool S_Base::hasEntity(const EntityId& l_entity) {
    return (std::find(m_entities.begin(), m_entities.end(), l_entity) != m_entities.end());
}

bool S_Base::addEntity(const EntityId& l_entity) {
    if (hasEntity(l_entity)) return false;
    m_entities.emplace_back(l_entity);
    return true;
}

bool S_Base::removeEntity(const EntityId& l_entity) {
    auto itr = std::find_if(m_entities.begin(), m_entities.end(), [&l_entity](const EntityId& id) {
        return l_entity == id;
    });
    if (itr == m_entities.end()) return false;
    m_entities.erase(itr);
    return true;
}

System S_Base::getId() { return m_id; }

bool S_Base::fitsRequirements(const Bitmask& l_bits) {
    return std::find_if(m_requiredComponents.begin(), m_requiredComponents.end(), [&l_bits](const Bitmask& b) {
        return b.matches(l_bits, b.getMask());
    }) != m_requiredComponents.end();
}

void S_Base::purge() {
    m_entities.clear();
}