#pragma once
#include <iostream>
#include <vector>
#include "componentStorage.h"

struct cTest
{
	static int ECS_Index;
};

class EntityManager {
private:

	std::vector<Interface*> _storages;   // _storages[componentId<T>()] = stockage de T
	std::vector<Entity>     _freeIds;    // ids recyclables
	Entity                  _next = 0;

public:



};