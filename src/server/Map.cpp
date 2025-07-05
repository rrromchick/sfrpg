#include "Map.h"

Map::Map(ServerEntityManager* l_entityMgr) : m_entityManager(l_entityMgr), m_maxMapSize(32, 32) {
    loadTiles("tiles.cfg");
}

Map::~Map() { purgeTileMap(); purgeTileSet(); }

Tile* Map::getTile(unsigned int l_x, unsigned int l_y, unsigned int l_layer) {
    if (l_x < 0 || l_y < 0 || l_x >= m_maxMapSize.x || l_y >= m_maxMapSize.y || l_layer >= (unsigned int)Sheet::Num_Layers) {
        return nullptr;
    }

    auto itr = m_tileMap.find(convertCoords(l_x, l_y, l_layer));
    if (itr == m_tileMap.end()) return nullptr;
    return itr->second;
}

TileInfo* Map::getDefaultTile() { return &m_defaultTile; }
const sf::Vector2f& Map::getPlayerStart() { return m_playerStart; }
const sf::Vector2u& Map::getMapSize() { return m_maxMapSize; }
unsigned int Map::getTileSize() { return (unsigned int)Sheet::Tile_Size; }

void Map::loadMap(const std::string& l_path) {
    std::ifstream file;
    file.open(Utils::getWorkingDirectory() + l_path);

    if (!file.is_open()) {
        std::cerr << "Failed to load map from file: " << l_path << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line[0] == '|') continue;
        std::stringstream keystream(line);
        std::string type;
        keystream >> type;

        if (type == "TILE") {
            unsigned int tileId = 0;
            keystream >> tileId;
            
            if (tileId < 0) { std::cout << "Bad tile id: " << tileId << std::endl; continue; }

            auto itr = m_tileSet.find(tileId);
            if (itr == m_tileSet.end()) { std::cout << "Unknown tile id!" << std::endl; continue; }

            sf::Vector2u tileCoords;
            unsigned int tileLayer = 0;
            bool tileSolidity = 0;
            keystream >> tileCoords.x >> tileCoords.y >> tileLayer >> tileSolidity;

            if (tileCoords.x > m_maxMapSize.x || tileCoords.y > m_maxMapSize.y || tileLayer >= (unsigned int)Sheet::Num_Layers) {
                std::cout << "Invalid tile loaded: " << tileCoords.x << ":" << tileCoords.y << std::endl;
                continue;
            }

            Tile* tile = new Tile();
            tile->m_properties = itr->second;
            tile->m_solid = tileSolidity;

            if (!m_tileMap.emplace(convertCoords(tileCoords.x, tileCoords.y, tileLayer), tile).second) {
                std::cerr << "Failed to emplace tile to map!" << std::endl;
                delete tile;
                tile = nullptr;
                continue;
            }

            std::string warp;
            keystream >> warp;
            if (warp == "WARP") tile->m_warp = true;
            else tile->m_warp = false;
        } else if (type == "SIZE") {
            keystream >> m_maxMapSize.x >> m_maxMapSize.y;
        } else if (type == "DEFAULT_FRICTION") {
            keystream >> m_defaultTile.m_friction.x >> m_defaultTile.m_friction.y;
        } else if (type == "ENTITY") {
            std::string name;
            keystream >> name;

            int entityId = m_entityManager->addEntity(name);
            if (entityId < 0) continue;

            C_Base* position = nullptr;
            if (position = m_entityManager->getComponent<C_Position>(entityId, Component::Position)) keystream >> *position;
        } else {
            std::cout << "Unknown type!" << std::endl;
        }
    }

    file.close();
    std::cout << "Map loaded!" << std::endl;
}

void Map::loadTiles(const std::string& l_path) {
    std::ifstream file;
    file.open(Utils::getWorkingDirectory() + l_path);

    if (!file.is_open()) {
        std::cerr << "Failed to load tile set from file: " << l_path << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line[0] == '|') continue;

        std::stringstream keystream(line);
        TileId id = 0;
        keystream >> id;
        if (id < 0) { std::cout << "Bad id: " << id << std::endl; continue; }

        TileInfo* info = new TileInfo(id);
        keystream >> info->m_name >> info->m_friction.x >> info->m_friction.y >> info->m_deadly;
        if (!m_tileSet.emplace(id, info).second) {
            std::cerr << "Failed to emplace tile to tile set: " << info->m_name << std::endl;
            delete info;
        }
    }

    file.close();
}

void Map::update(float l_dT) {}

unsigned int Map::convertCoords(unsigned int l_x, unsigned int l_y, unsigned int l_layer) {
    return ((l_layer * m_maxMapSize.y + l_y) * m_maxMapSize.x + l_x);
}

void Map::purgeTileMap() {
    while (m_tileMap.begin() != m_tileMap.end()) {
        delete m_tileMap.begin()->second;
        m_tileMap.erase(m_tileMap.begin());
    }

    m_tileCount = 0;
    m_entityManager->purge();
}

void Map::purgeTileSet() {
    while (m_tileSet.begin() != m_tileSet.end()) {
        delete m_tileSet.begin()->second;
        m_tileSet.erase(m_tileSet.begin());
    }

    m_tileSetCount = 0;
}