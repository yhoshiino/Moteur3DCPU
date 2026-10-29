#pragma once
#include <vector>
#include <utility>
#include <cstdint>
#include <cassert>
#include "entity.h"

class Interface {
public:
    virtual ~Interface() = default;
    virtual void remove(Entity e) = 0;
};

template <typename T>
class ComponentStorage : public Interface {
public:
    bool has(Entity e) const {
        return e < Index.size() && Index[e] != NONE;
    }

    T& add(Entity e, T value) {
        assert(!has(e) && "cette entite a deja ce composant");
        if (e >= Index.size()) Index.resize(e + 1, NONE);
        Index[e] = (uint32_t)Data.size();
        Data.push_back(std::move(value));
        Owners.push_back(e);
        return Data.back();
    }

    T& get(Entity e) {
        assert(has(e) && "composant absent");
        return Data[Index[e]];
    }

    T* tryGet(Entity e) { return has(e) ? &Data[Index[e]] : nullptr; }

    void remove(Entity e) override {
        if (!has(e)) return;
        uint32_t i = Index[e];
        uint32_t last = (uint32_t)Data.size() - 1;
        Data[i] = std::move(Data[last]);
        Owners[i] = Owners[last];
        Index[Owners[i]] = i;
        Index[e] = NONE;
        Data.pop_back();
        Owners.pop_back();
    }

    // <<< Ces trois méthodes sont celles qui manquaient : each() en a besoin
    size_t size() const { return Data.size(); }
    T& at(size_t i) { return Data[i]; }
    Entity ownerAt(size_t i) const { return Owners[i]; }

private:
    static constexpr uint32_t NONE = UINT32_MAX;

    std::vector<T>        Data;
    std::vector<Entity>   Owners;
    std::vector<uint32_t> Index;
};