#pragma once
#include <iostream>
#include <vector>
#include "componentStorage.h"

//struct cTest
//{
//	static int ECS_Index;
//};

class EntityManager {
private:

	std::vector<Interface*> _storages;   
	std::vector<Entity>     _freeIds;    
	Entity                  _next = 0;

public:



};