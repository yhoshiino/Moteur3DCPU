// ============================================================
//  obj.h — a minimal Wavefront OBJ loader (chapter 9)
// ------------------------------------------------------------
//  Chapters 6-8 read only vertex positions ('v') and the leading
//  vertex index of each face. Chapter 9 needs texture coordinates,
//  so we also read:
//
//    vt  u v           -> a texture coordinate (stored in uvs[])
//    f   v0/t0 v1/t1 v2/t2
//                      -> a triangle, where each corner now also
//                         carries a UV index (the number after the
//                         first '/').
//
//  Face tokens may be "v", "v/t", "v/t/n", or "v//n". We read the
//  vertex index (before the first '/') and, when present, the
//  texture index (between the first and second '/'). Normals ('vn')
//  are still ignored — we compute our own in chapter 10.
//
//  OBJ indices start at 1, so we subtract 1. Still not a complete
//  parser (no quads, no negative indices, no materials) — just
//  enough for the triangulated teaching meshes we use.
// ============================================================
#pragma once
#include "vec.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    vec3* verts;     int vert_count;
    vec2* uvs;       int uv_count;     // Wavefront 'vt' lines
    int* faces;     int face_count;   // 3 vertex indices per face (0-based)
    int* face_uvs;                    // 3 UV indices per face (0-based, or -1)
} Mesh;

// Parse one face token starting at *p, e.g. "12", "12/3", "12/3/4",
// or "12//4". Advances *p past the token. Writes the (1-based)
// vertex index to *vi and the (1-based) texture index to *ti, or
// 0 into *ti if the token had no texture part.
static void obj__parse_face_token(const char** p, int* vi, int* ti) {
    while (**p == ' ' || **p == '\t') (*p)++;
    *vi = atoi(*p);
    *ti = 0;
    // advance over the vertex-index digits
    while (**p && **p != '/' && **p != ' ' && **p != '\t' && **p != '\n' && **p != '\r') (*p)++;
    if (**p == '/') {
        (*p)++;                       // skip the first '/'
        if (**p != '/') {             // "v/t..." — a texture index is present
            *ti = atoi(*p);
            while (**p && **p != '/' && **p != ' ' && **p != '\t' && **p != '\n' && **p != '\r') (*p)++;
        }
        if (**p == '/') {             // skip "/n" (normal index), unused
            (*p)++;
            while (**p && **p != ' ' && **p != '\t' && **p != '\n' && **p != '\r') (*p)++;
        }
    }
}

// Load a mesh from an OBJ file. Returns 1 on success, 0 on failure.
static int mesh_load(const char* path, Mesh* out) {
    FILE* f = fopen(path, "r");
    if (!f) { fprintf(stderr, "could not open %s\n", path); return 0; }

    // Grow-able arrays. Start small, double when full.
    int vcap = 1024, vtcap = 1024, fcap = 1024;
    out->verts = (vec3*)malloc(vcap * sizeof(vec3));
    out->uvs = (vec2*)malloc(vtcap * sizeof(vec2));
    out->faces = (int*)malloc(fcap * 3 * sizeof(int));
    out->face_uvs = (int*)malloc(fcap * 3 * sizeof(int));
    out->vert_count = 0;
    out->uv_count = 0;
    out->face_count = 0;

    char line[256];
    while (fgets(line, sizeof line, f)) {
        if (line[0] == 'v' && line[1] == ' ') {
            // a vertex position: three floats
            vec3 v;
            if (sscanf(line + 2, "%f %f %f", &v.x, &v.y, &v.z) == 3) {
                if (out->vert_count == vcap) {
                    vcap *= 2;
                    out->verts = (vec3*)realloc(out->verts, vcap * sizeof(vec3));
                }
                out->verts[out->vert_count++] = v;
            }
        }
        else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
            // a texture coordinate: two floats
            vec2 t;
            if (sscanf(line + 3, "%f %f", &t.x, &t.y) >= 2) {
                if (out->uv_count == vtcap) {
                    vtcap *= 2;
                    out->uvs = (vec2*)realloc(out->uvs, vtcap * sizeof(vec2));
                }
                out->uvs[out->uv_count++] = t;
            }
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            // a triangle: three "v/t" corners (1-based -> subtract 1)
            const char* p = line + 2;
            int va, ta, vb, tb, vc, tc;
            obj__parse_face_token(&p, &va, &ta);
            obj__parse_face_token(&p, &vb, &tb);
            obj__parse_face_token(&p, &vc, &tc);
            if (out->face_count == fcap) {
                fcap *= 2;
                out->faces = (int*)realloc(out->faces, fcap * 3 * sizeof(int));
                out->face_uvs = (int*)realloc(out->face_uvs, fcap * 3 * sizeof(int));
            }
            int fi = out->face_count * 3;
            out->faces[fi + 0] = va - 1;
            out->faces[fi + 1] = vb - 1;
            out->faces[fi + 2] = vc - 1;
            // texture index is 0 when absent; map present->0-based, absent->-1
            out->face_uvs[fi + 0] = ta ? ta - 1 : -1;
            out->face_uvs[fi + 1] = tb ? tb - 1 : -1;
            out->face_uvs[fi + 2] = tc ? tc - 1 : -1;
            out->face_count++;
        }
        // any other line type is ignored
    }

    fclose(f);
    return 1;
}

static void mesh_free(Mesh* m) {
    free(m->verts);
    free(m->uvs);
    free(m->faces);
    free(m->face_uvs);
    m->verts = NULL;
    m->uvs = NULL;
    m->faces = NULL;
    m->face_uvs = NULL;
    m->vert_count = 0;
    m->uv_count = 0;
    m->face_count = 0;
}