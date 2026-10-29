#pragma once
#include <array>
#include <any>
#include <iostream>
#include <utility>
#include <cstdint>
#include <cassert>
#include "entity.h"


class IComponentStorage {
public:
    virtual ~IComponentStorage() = default;
    virtual void Remove(Entity e) = 0;
    virtual void Add(Entity e, void* value) = 0;
    virtual std::any& Get(Entity e) = 0;
    virtual bool Has(Entity e) = 0;
};

template <typename T>
class componentStorage : public IComponentStorage {
private:

    std::array<T, 100>    Data;
    std::array<bool, 100> Owned;


public:

    void Add(Entity e, void* value) override {
        Data[e] = *(static_cast<T*>(value));
        Owned[e] = true;
    }

    void Remove(Entity e) override {
        Owned[e] = false;
    }

    void* Get(Entity e) override {
        assert(!Has(e) && "composant non existant");
        return (void*)&Data[e];
    }

    bool Has(Entity e) override {
        return Owned[e];
    }
};