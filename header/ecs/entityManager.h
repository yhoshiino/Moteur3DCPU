#pragma once
#include <iostream>
#include <vector>
#include "componentStorage.h"


inline int nextComponentId() { static int id = 0; return id++; }

template <typename T>
int componentID() {
	static const int id = nextComponentId();
	return id;
}

class EntityManager {
private:

    std::vector<Interface*> _storages;  
    std::vector<bool>       _alive;      
    std::vector<Entity>     _freeIds;    

	template <typename T>
	ComponentStorage<T>& storage() {
		size_t id = (size_t)componentID<T>();
		if (id >= _storages.size()) {
			_storages.resize(id + 1, nullptr);
		}
		if (!_storages[id]) {
			_storages[id] = new ComponentStorage<T>();
		}
		return *static_cast<ComponentStorage<T>*>(_storages[id]);
	}


public:

    EntityManager() = default;
    EntityManager(const EntityManager&) = delete;         
    EntityManager& operator=(const EntityManager&) = delete;
    ~EntityManager() { for (Interface* s : _storages) delete s; }

    // ---- Entités ----
    Entity create() {
        if (!_freeIds.empty()) {
            Entity e = _freeIds.back();
            _freeIds.pop_back();
            _alive[e] = true;
            return e;
        }
        _alive.push_back(true);
        return (Entity)(_alive.size() - 1);
    }

    bool isAlive(Entity e) const { return e < _alive.size() && _alive[e]; }

    void destroy(Entity e) {
        if (!isAlive(e)) return;                 // évite de recycler deux fois le même id
        for (Interface* s : _storages)
            if (s) s->remove(e);                 
        _alive[e] = false;
        _freeIds.push_back(e);
    }

    // ---- Composants ----
    template <typename T>
    T& add(Entity e, T value = T{}) {
        assert(isAlive(e));
        return storage<T>().add(e, std::move(value));
    }

    template <typename T> bool has(Entity e) { return storage<T>().has(e); }
    template <typename T> T& get(Entity e) { return storage<T>().get(e); }
    template <typename T> T* tryGet(Entity e) { return storage<T>().tryGet(e); }
    template <typename T> void remove(Entity e) { storage<T>().remove(e); }

    // ---- Systèmes : parcours de toutes les entités ayant TOUS les composants demandés ----
    // On parcourt le premier slot et on vérifie que l'entité a aussi les autres.
    template <typename First, typename... Rest, typename Fn>
    void each(Fn fn) {
        auto& first = storage<First>();
        for (size_t i = 0; i < first.size(); i++) {
            Entity e = first.ownerAt(i);
            if ((storage<Rest>().has(e) && ...))                    // True si c vide
                fn(e, first.at(i), storage<Rest>().get(e)...);
        }
    }


};