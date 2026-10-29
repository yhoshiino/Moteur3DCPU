#pragma once
#include <vector>
#include <iostream>
#include "componentStorage.h"


class entityManager {
private:

	std::vector<IComponentStorage*> Storage;

	

public:

	template <typename TComponent>
	int addComponentType() {
		componentStorage<TComponent>* storage = new componentStorage<TComponent>();
		Storage.push_back(static_cast<IComponentStorage*> storage);

		return Storage.size();
	}

	void addComponent(int entity, int componentID, void* value) {
		Storage[componentID]->Add(entity, value);
	}

};
