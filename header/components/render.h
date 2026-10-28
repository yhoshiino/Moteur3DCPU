#pragma once
#include "obj.h"
#include "texture.h"

struct MeshRenderer {
    const Mesh* mesh = nullptr;       // le composant ne POSSÈDE pas le mesh
    const Texture* texture = nullptr;
    int            material = 0;         // indice dans le tableau de matériaux
};