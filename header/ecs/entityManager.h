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
		Storage.push_back(storage);

		return static_cast<int>(Storage.size()) - 1;
	}

	template <typename TComponent>
	TComponent* getComponent(int entity, int componentID) {
		return static_cast<TComponent*>(Storage[componentID]->Get(entity));
	}

	bool hasComponent(int entity, int componentID) {
		return Storage[componentID]->Has(entity);
	}

	void addComponent(int entity, int componentID, void* value) {
		Storage[componentID]->Add(entity, value);
	}



};
