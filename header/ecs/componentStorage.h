#pragma once
#include <iostream>
#include <vector>
#include "entity.h"


class Interface {
public:                                          
	virtual ~Interface() = default;              
	virtual void remove(Entity e) = 0;           
};

template <typename T>
class ComponentStorage : public Interface{
public:

	T add() override {

	};




private:

	std::vector<T> Data;

};
