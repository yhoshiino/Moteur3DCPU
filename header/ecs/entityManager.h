#pragma once
#include <vector>
#include <iostream>
#include "componentStorage.h"


class entityManager {
private:

	std::vector<IComponentStorage*> Storage;
	std::vector<bool> Exists{};
	std::vector<Entity> UsableIDs;

	int counter = 0;

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

	int entityCount() {
		return counter;
	}

	bool hasComponent(int entity, int componentID) {
		return Storage[componentID]->Has(entity);
	}

	void addComponent(int entity, int componentID, void* value) {
		Storage[componentID]->Add(entity, value);
	}

	void removeComponent(Entity e, int componentID) {
		Storage[componentID]->Remove(e);
	};


	int createEntity(std::string name = "entity") {
		(void)name;
		if (!UsableIDs.empty()) {
			Entity e = UsableIDs.back();
			UsableIDs.pop_back();
			return static_cast<int>(e);
		}
		return ++counter;
	};


	void destroyEntity(Entity e) {
		UsableIDs.push_back(e);
		for (int i = 0; i < Storage.size(); i++) {
			if (Storage[i]->Has(e)) {
				Storage[i]->Remove(e);
			}
		}
	};

};
