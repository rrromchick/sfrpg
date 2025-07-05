#pragma once

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include "Server_Entity_Manager.h"
#include "Utilities.h"
#include <math.h>

enum class Sheet { Tile_Size = 32, Sheet_Width = 256, Sheet_Height = 256, Num_Layers = 4 };

using TileId = unsigned int;

struct TileInfo {
    TileInfo(TileId l_id = 0) {
        m_id = l_id;
        m_deadly = false;
    }

    ~TileInfo() {}

    unsigned int m_id;
    sf::Vector2f m_friction;
    std::string m_name;
    bool m_deadly;
};

struct Tile {
    TileInfo* m_properties;
    
    bool m_solid;
    bool m_warp;
};

using TileMap = std::unordered_map<TileId, Tile*>;
using TileSet = std::unordered_map<TileId, TileInfo*>;

class Map {
public:
    Map(ServerEntityManager* l_entityManager);
    ~Map();

    Tile* getTile(unsigned int l_x, unsigned int l_y, unsigned int l_layer);
    TileInfo* getDefaultTile();

    const sf::Vector2u& getMapSize();
    const sf::Vector2f& getPlayerStart();
    unsigned int getTileSize();

    void loadMap(const std::string& l_path);
    void update(float l_dT);
private:
    unsigned int convertCoords(unsigned int l_x, unsigned int l_y, unsigned int l_layer);
    void loadTiles(const std::string& l_path);

    void purgeTileMap();
    void purgeTileSet();

    TileMap m_tileMap;
    TileSet m_tileSet;
    TileInfo m_defaultTile;

    unsigned int m_tileSetCount;
    unsigned int m_tileCount;

    sf::Vector2u m_maxMapSize;
    sf::Vector2f m_playerStart;

    ServerEntityManager* m_entityManager;
};