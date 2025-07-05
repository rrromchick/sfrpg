#include "S_Collision.h"
#include "System_Manager.h"
#include "Map.h"
#include "C_Attacker.h"

S_Collision::S_Collision(SystemManager* l_systemMgr) : S_Base(System::Collision, l_systemMgr) {
    Bitmask req;

    req.turnOnBit((unsigned int)Component::Position);
    req.turnOnBit((unsigned int)Component::Collidable);
    m_requiredComponents.push_back(req);
    req.clear();

    m_gameMap = nullptr;
}

S_Collision::~S_Collision() {}

void S_Collision::update(float l_dT) {
    if (!m_gameMap) return;
    
    EntityManager* entityManager = m_systemManager->getEntityManager();
    for (auto& itr : m_entities) {
        C_Position* pos = entityManager->getComponent<C_Position>(itr, Component::Position);
        C_Collidable* col = entityManager->getComponent<C_Collidable>(itr, Component::Collidable);

        col->resetCollisionFlags();
        checkOutOfBounds(pos, col);
        mapCollisions(itr, pos, col);
    }
    entityCollisions();
}

void S_Collision::entityCollisions() {
    EntityManager* entityManager = m_systemManager->getEntityManager();

    for (auto itr = m_entities.begin(); itr != m_entities.end(); ++itr) {
        for (auto itr2 = std::next(itr); itr2 != m_entities.end(); ++itr2) {
            C_Collidable* col1 = entityManager->getComponent<C_Collidable>(*itr, Component::Collidable);
            C_Collidable* col2 = entityManager->getComponent<C_Collidable>(*itr2, Component::Collidable);

            if (col1->getCollidable().intersects(col2->getCollidable())) {
                // Entity-on-entity collisions!
            }

            C_Attacker* a1 = entityManager->getComponent<C_Attacker>(*itr, Component::Attacker);
            C_Attacker* a2 = entityManager->getComponent<C_Attacker>(*itr2, Component::Attacker);

            if (!a1 && !a2) continue;

            Message msg((MessageType)EntityMessage::Being_Attacked);
            
            if (a1) {
                if (a1->getAreaOfAttack().intersects(col2->getCollidable())) {
                    msg.m_sender = *itr;
                    msg.m_receiver = *itr2;

                    m_systemManager->getMessageHandler()->dispatch(msg);
                }
            }

            if (a2) {
                if (a2->getAreaOfAttack().intersects(col1->getCollidable())) {
                    msg.m_sender = *itr2;
                    msg.m_receiver = *itr;

                    m_systemManager->getMessageHandler()->dispatch(msg);
                }
            }
        }
    }
}

void S_Collision::setMap(Map* l_map) { m_gameMap = l_map; }

void S_Collision::checkOutOfBounds(C_Position* l_pos, C_Collidable* l_col) {
    unsigned int tileSize = m_gameMap->getTileSize();

    if (l_pos->getPosition().x < 0) {
        l_pos->setPosition(0.f, l_pos->getPosition().y);
        l_col->setPosition(l_pos->getPosition());
    } else if (l_pos->getPosition().x > m_gameMap->getMapSize().x * tileSize) {
        l_pos->setPosition(m_gameMap->getMapSize().x * tileSize, l_pos->getPosition().y);
        l_col->setPosition(l_pos->getPosition());
    }

    if (l_pos->getPosition().y < 0) {
        l_pos->setPosition(l_pos->getPosition().x, 0.f);
        l_col->setPosition(l_pos->getPosition());
    } else if (l_pos->getPosition().y > m_gameMap->getMapSize().y * tileSize) {
        l_pos->setPosition(l_pos->getPosition().x, m_gameMap->getMapSize().y * tileSize);
        l_col->setPosition(l_pos->getPosition());
    }
}

void S_Collision::mapCollisions(const EntityId& l_entity, C_Position* l_pos, C_Collidable* l_col) {
    if (!hasEntity(l_entity)) return;

    Collisions c;
    unsigned int tileSize = m_gameMap->getTileSize();

    sf::FloatRect entityAABB = l_col->getCollidable();
    int fromX = floor(entityAABB.left / tileSize);
    int toX = floor((entityAABB.left + entityAABB.width) / tileSize);
    int fromY = floor(entityAABB.top / tileSize);
    int toY = floor((entityAABB.top + entityAABB.height) / tileSize);

    for (int x = fromX; x <= toX; ++x) {
        for (int y = fromY; y <= toY; ++y) {
            for (int l = 0; l < (int)Sheet::Num_Layers; ++l) {
                Tile* t = m_gameMap->getTile(x, y, l);
                if (!t) continue;
                if (!t->m_solid) continue;

                sf::FloatRect tileAABB = sf::FloatRect(x * tileSize, y * tileSize, tileSize, tileSize);
                sf::FloatRect intersection;
                entityAABB.intersects(tileAABB, intersection);
                float S = intersection.width * intersection.height;
                if (!S) continue;
                c.emplace_back(S, t->m_properties, tileAABB);
                break;
            }
        }
    }

    if (c.empty()) return;
    std::sort(c.begin(), c.end(), [](const CollisionElement& c1, const CollisionElement& c2) {
        return c1.m_area > c2.m_area;
    });

    for (auto& col : c) {
        entityAABB = l_col->getCollidable();
        
        float xDiff = (entityAABB.left + (entityAABB.width / 2)) - (col.m_tileBounds.left + (col.m_tileBounds.width / 2));
        float yDiff = (entityAABB.top + (entityAABB.height / 2)) - (col.m_tileBounds.top + (col.m_tileBounds.height / 2));

        float resolve = 0;

        if (std::abs(xDiff) > std::abs(yDiff)) {
            if (xDiff > 0) {
                resolve = (col.m_tileBounds.left + tileSize) - entityAABB.left;
            } else {
                resolve = -((entityAABB.left + entityAABB.width) - col.m_tileBounds.left);
            }

            l_pos->moveBy(resolve, 0.f);
            l_col->setPosition(l_pos->getPosition());
            m_systemManager->addEvent(l_entity, (EventID)EntityEvent::Colliding_X);
            l_col->collideOnX();
        } else {
            if (yDiff > 0) {
                resolve = (col.m_tileBounds.top + tileSize) - entityAABB.top;
            } else {
                resolve = -((entityAABB.top + entityAABB.height) - col.m_tileBounds.top);
            }

            l_pos->moveBy(0.f, resolve);
            l_col->setPosition(l_pos->getPosition());
            m_systemManager->addEvent(l_entity, (EventID)EntityEvent::Colliding_Y);
            l_col->collideOnY();
        }
    }
}

void S_Collision::handleEvent(const EntityId& l_entity, const EntityEvent& l_event) {}
void S_Collision::notify(const Message& l_message) {}