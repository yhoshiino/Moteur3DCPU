#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "vec.h"


struct Mesh {
	std::vector<vec3> verts;
	std::vector<vec2> uvs;
	std::vector<int> faces;
	std::vector<int> face_uvs;

    bool load(const std::string& path);

	int vert_count() const { return (int)verts.size(); }
	int uv_count() const { return (int)uvs.size(); }
	int face_count() const { return (int)faces.size() / 3; }
};

namespace obj_detail {

    inline void parseFaceToken(const std::string& tok, int& vi, int& ti) {
        size_t s1 = tok.find('/');
        vi = std::stoi(tok.substr(0, s1));
        ti = 0;
        if (s1 == std::string::npos) return;

        size_t s2 = tok.find('/', s1 + 1);
        size_t len = (s2 == std::string::npos) ? std::string::npos : s2 - s1 - 1;
        std::string t = tok.substr(s1 + 1, len); 
        if (!t.empty()) ti = std::stoi(t);
    }
}

inline bool Mesh::load(const std::string& path) {
    std::fstream file(path);
    if (!file) {
		std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return false;
    }

	verts.clear();
	uvs.clear();
	faces.clear();
	face_uvs.clear();


    std::string line;
    while (std::getline(file, line)) {
        std::istringstream in(line);
        std::string tag;
		in >> tag;

		if (tag == "v") {
			vec3 v;
			if (in >> v.x >> v.y >> v.z)
				verts.push_back(v);
		}
		else if (tag == "vt") {
			vec2 t;
			if (in >> t.x >> t.y)
				uvs.push_back(t);
		}
		else if (tag == "f") {
			std::string tok[3];
			if (!(in >> tok[0] >> tok[1] >> tok[2])) continue;
			for (int i = 0; i < 3; i++) {
				int vi, ti;
				obj_detail::parseFaceToken(tok[i], vi, ti);
				faces.push_back(vi - 1);
				face_uvs.push_back(ti - 1);
			}
		}
    }
	return true;
}