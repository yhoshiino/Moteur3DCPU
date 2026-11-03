#pragma once
#include <vector>
#include <iostream>
#include "componentStorage.h"
#include <bitset>



class entityManager {
private:

	std::vector<IComponentStorage*> Storage;
	std::array<uint32_t, MAX_ENTITIES> entityComponents;
	std::vector<Entity> UsableIDs;

	int counter = 0;

public:

	entityManager() = default;
	entityManager(const entityManager&) = delete;
	~entityManager() {
		for (IComponentStorage* s : Storage) delete s;
	}


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

	void addComponent(Entity e, int componentID, void* value) {
		Storage[componentID]->Add(e, value);
		entityComponents[e] |= (1u << componentID);
	}

	void removeComponent(Entity e, int componentID) {
		Storage[componentID]->Remove(e);
		entityComponents[e] &= ~(1u << componentID);
	};


	Entity createEntity(std::string name = "entity") {
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
		entityComponents[e] = 0;
	};

	std::vector<int> FetchAllEntities(int componentsIDs...)
	{
		uint32_t searchBitset = (0u | (1u << componentsIDs));

		std::vector<int> value;
		if (searchBitset == 0) return value;

		for (int e = 0; e <= counter; e++) {
			if ((entityComponents[e] & searchBitset) == searchBitset) {
				value.push_back(e);
			}
		}
		
		return value;
		// Créé le bitset de la recherche avec la liste des componentIDs
		// Créé le vector des Entity(int) qui respectent le bitset de la recherche
		// C'est clairement l'endroit ou les performances s'effondrent le plus, un iterator serait déjà moins horrible
	}

};
