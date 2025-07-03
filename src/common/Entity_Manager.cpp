#include "Entity_Manager.h"
#include "System_Manager.h"

EntityManager::EntityManager(SystemManager* l_systemMgr) : m_systemManager(l_systemMgr) {}

EntityManager::~EntityManager() { purge(); }

int EntityManager::addEntity(const Bitmask& l_bitmask, int l_id) {
    unsigned int id = (l_id == -1 ? m_idCounter : l_id);
    if (!m_entities.emplace(id, EntityData()).second) return -1;
    if (l_id == -1) ++m_idCounter;

    for (unsigned int i = 0; i < N_COMPONENT_TYPES; ++i) {
        if (l_bitmask.getBit(i)) {
            addComponent(id, (Component)i);
        }
    }

    m_systemManager->entityModified(id, l_bitmask);
    m_systemManager->addEvent(id, (EventID)EntityEvent::Spawned);
    
    return id;
}

int EntityManager::addEntity(const std::string& l_entityFile, int l_id) {
    int entityId = -1;

    std::ifstream file;
    file.open(Utils::getWorkingDirectory() + "media/Entities/" + l_entityFile + ".entity");

    if (!file.is_open()) {
        std::cerr << "! Failed to open entity file: " << l_entityFile << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line[0] == '|') continue;

        std::stringstream keystream(line);
        std::string type;
        keystream >> type;

        if (type == "Name") {
            // ...
        } else if (type == "Attributes") {
            if (entityId != -1) continue;

            Bitset set = 0;
            Bitmask mask;
            keystream >> set;
            mask.setMask(set);

            entityId = addEntity(mask);
            if (entityId == -1) return -1;
        } else if (type == "Component") {
            if (entityId == -1) continue;

            unsigned int cid = 0;
            keystream >> cid;
            
            C_Base* component = getComponent<C_Base>(entityId, (Component)cid);
            if (!component) continue;

            keystream >> *component;
        }
    }

    file.close();
    m_entities.at(entityId).m_type = l_entityFile;
    return entityId;
}

bool EntityManager::removeEntity(const EntityId& l_entity) {
    auto itr = m_entities.find(l_entity);
    if (itr == m_entities.end()) return false;

    Message msg((MessageType)EntityMessage::Removed_Entity);
    msg.m_receiver = l_entity;
    msg.m_int = l_entity;
    m_systemManager->getMessageHandler()->dispatch(msg);

    while (itr->second.m_components.begin() != itr->second.m_components.end()) {
        delete *itr->second.m_components.begin();
        itr->second.m_components.erase(itr->second.m_components.begin());
    }

    itr->second.m_bitmask.clear();
    m_entities.erase(itr);
    --m_idCounter;

    m_systemManager->removeEntity(l_entity);
    return true;
}

bool EntityManager::hasEntity(const EntityId& l_entity) {
    auto itr = m_entities.find(l_entity);
    return (itr != m_entities.end());
}

bool EntityManager::addComponent(const EntityId& l_entity, const Component& l_component) {
    auto itr = m_entities.find(l_entity);
    if (itr == m_entities.end()) return false;
    if (itr->second.m_bitmask.getBit((unsigned int)l_component)) return false;

    auto itr2 = m_compFactory.find(l_component);
    if (itr2 == m_compFactory.end()) return false;
    
    C_Base* comp = itr2->second();
    itr->second.m_components.emplace_back(comp);
    itr->second.m_bitmask.turnOnBit((unsigned int)l_component);

    m_systemManager->entityModified(l_entity, itr->second.m_bitmask);
    return true;
}

bool EntityManager::hasComponent(const EntityId& l_entity, const Component& l_component) {
    auto itr = m_entities.find(l_entity);
    if (itr == m_entities.end()) return false;

    auto& container = itr->second.m_components;
    auto itr2 = std::find_if(container.begin(), container.end(), [&l_component](C_Base* b) {
        return b->getType() == l_component;
    });

    return (itr2 != container.end());
}

bool EntityManager::removeComponent(const EntityId& l_entity, const Component& l_component) {
    auto itr = m_entities.find(l_entity);
    if (itr == m_entities.end()) return false;
    if (!itr->second.m_bitmask.getBit((unsigned int)l_component)) return false;

    auto& container = itr->second.m_components;
    auto comp = std::find_if(container.begin(), container.end(), [&l_component](C_Base* b) {
        return b->getType() == l_component;
    });

    if (comp == container.end()) return false;
    delete *comp;
    itr->second.m_bitmask.clearBit((unsigned int)l_component);
    container.erase(comp);

    m_systemManager->entityModified(l_entity, itr->second.m_bitmask);
    return true;
}

void EntityManager::purge() {
    for (auto& itr : m_entities) {
        for (auto& comp : itr.second.m_components) { delete comp; }
        itr.second.m_components.clear();
        itr.second.m_bitmask.clear();
    }
    
    m_idCounter = 0;
    m_entities.clear();
}

unsigned int EntityManager::getEntityCount() const { return m_entities.size(); }