#pragma once
#include <iostream>
#include <vector>


using Entity = uint32_t;
constexpr uint32_t NONE = UINT32_MAX;

class Interface {
public:                                          
	virtual ~Interface() = default;              
	virtual void remove(Entity e) = 0;           
};

template <typename T>
class ComponentStorage : public Interface{
public:

	T add() override {

	}




private:

	std::vector<T> Data;

};
