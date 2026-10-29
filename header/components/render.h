#pragma once
#include "obj.h"
#include "texture.h"

struct MeshRenderer {
    const Mesh* mesh = nullptr;      
    const Texture* texture = nullptr;
    int            material = 0;     
};