#pragma once

#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Utilities.h"

template <typename Derived, typename T>
class ResourceManager {
public:
    ResourceManager(const std::string& l_pathFile) {
        loadPaths(l_pathFile);
    }

    virtual ~ResourceManager() { purgeResources(); }

    std::string getPath(const std::string& l_id) {
        auto itr = m_paths.find(l_id);
        return (itr != m_paths.end() ? itr->second : "");
    }

    T* getResource(const std::string& l_id) {
        auto itr = find(l_id);
        return (itr ? itr->first : nullptr);
    }

    bool requireResource(const std::string& l_id) {
        auto itr = find(l_id);
        if (itr != nullptr) {
            ++itr->second;
            return true;
        }

        auto path = m_paths.find(l_id);
        if (path == m_paths.end()) return false;

        T* resource = load(path);
        if (!resource) return false;

        m_resources.emplace(l_id, std::make_pair(resource, 1));
        return true;
    }

    bool releaseResource(const std::string& l_id) {
        auto itr = find(l_id);
        if (itr == nullptr) return false;
        --itr->second;
        if (!itr->second) unload(l_id);
        return true;
    }

    void purgeResources() {
        std::cout << "Purging all resources: " << std::endl;
        while (m_resources.begin() != m_resources.end()) {
            std::cout << "Purging resource: " << m_resources.begin()->first << std::endl;
            delete m_resources.begin()->second.first;
            m_resources.erase(m_resources.begin()):
        }
        
        std::cout << "Purging finished." << std::endl;
    }
protected:
    T* load(const std::string& l_id) {
        return static_cast<Derived*>(this)->load(l_id);
    }
private:
    std::pair<T*, unsigned int>* find(const std::string& l_id) {
        auto itr = m_resources.find(l_id);
        return (itr != m_resources.end() ? &itr->second : nullptr);
    }

    bool unload(const std::string& l_id) {
        auto itr = m_resources.find(l_id);
        if (itr == m_resources.end()) return false;
        delete itr->second.first;
        m_resources.erase(itr);
        return true;
    }

    void loadPaths(const std::string& l_pathFile) {
        std::ifstream file;
        file.open(Utils::getWorkingDirectory() + l_pathFile);

        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                std::stringstream keystream(line);
                std::string pathName;
                std::string path;

                keystream >> pathName;
                keystream >> path;

                m_paths.emplace(pathName, path);
            }

            file.close();
            return;
        }

        std::cerr << "Failed to load paths from file: " << l_pathFile << std::endl;
    }

    std::unordered_map<std::string, std::pair<T*, unsigned int>> m_resources;
    std::unordered_map<std::string, std::string> m_paths;
};