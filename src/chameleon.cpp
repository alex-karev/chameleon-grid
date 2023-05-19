#include "chameleon.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void ChameleonGrid::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_chunk_size"), &ChameleonGrid::get_chunk_size);
    ClassDB::bind_method(D_METHOD("set_chunk_size", "p_chunk_size"), &ChameleonGrid::set_chunk_size);
    ClassDB::add_property("ChameleonGrid", PropertyInfo(Variant::VECTOR3, "chunk_size"), "set_chunk_size", "get_chunk_size");

    ClassDB::bind_method(D_METHOD("add_chunk", "chunk_index"), &ChameleonGrid::add_chunk);
    ClassDB::bind_method(D_METHOD("remove_chunk", "chunk_id"), &ChameleonGrid::remove_chunk);
    ClassDB::bind_method(D_METHOD("count_chunks"), &ChameleonGrid::count_chunks);
    ClassDB::bind_method(D_METHOD("count_chunks_mem"), &ChameleonGrid::count_chunks_mem);
    ClassDB::bind_method(D_METHOD("optimize_chunks"), &ChameleonGrid::optimize_chunks);
    ClassDB::bind_method(D_METHOD("get_chunk_id", "chunk_index"), &ChameleonGrid::get_chunk_id);
    ClassDB::bind_method(D_METHOD("get_chunk_index", "chunk_id"), &ChameleonGrid::get_chunk_index);
}

ChameleonGrid::ChameleonGrid() {
    // Initialize any variables here.
    for (int i = 0; i < 3; i++)
        chunk_size[i] = DEFAULT_CHUNK_SIZE[i];
    chunk_number = 0;

}

ChameleonGrid::~ChameleonGrid() {
    // Add your cleanup here.
}

void ChameleonGrid::set_chunk_size(const Vector3 size) {
    chunk_size[0] = size.x;
    chunk_size[1] = size.y;
    chunk_size[2] = size.z;
}

Vector3 ChameleonGrid::get_chunk_size(){
    return Vector3(chunk_size[0], chunk_size[1], chunk_size[2]);
}


void ChameleonGrid::_ready() {
    // When added to scene
    voxel_number = chunk_size[0]*chunk_size[1]*chunk_size[2];
}

/*
* Chunks
*/

int ChameleonGrid::add_chunk(Vector3 index) {
    int id = -1;
    // Check if there are any chunks marked for rewrite
    for (int i = 0; i < chunks.size(); i++)
        if (chunks[i].rewrite)
            id = i;
    // Else create a new chunk
    if (id == -1) {
        chunks.push_back(Chunk());
        id = chunks.size()-1;
    }
    // Reset chunk values
    Chunk * chunk = &chunks[id];
    for (int i = 0; i < 3; i++)
        chunk->index[i] = index[i];
    chunk->values.resize(voxel_number, 0);
    chunk->mask.resize(voxel_number, -1);
    chunk->rewrite = 0;
    chunk_number += 1;
    // Return chunk id
    return id;
}
void ChameleonGrid::remove_chunk(int id) {
    chunks[id].rewrite = 1;
    chunk_number -= 1;
}
int ChameleonGrid::count_chunks() {
    return chunk_number;
}
int ChameleonGrid::count_chunks_mem() {
    return chunks.size();
}
void ChameleonGrid::optimize_chunks() {
    int i = 0;
    while (i < chunks.size()) {
        if (chunks[i].rewrite)
            chunks.erase(chunks.begin() + i);
        else
            i+=1;
    }
    chunks.shrink_to_fit();
}
int ChameleonGrid::get_chunk_id(Vector3 index) {
    for (int i = 0; i < chunks.size(); i++) {
        if (!chunks[i].rewrite &&
            chunks[i].index[0] == index.x &&
            chunks[i].index[1] == index.y &&
            chunks[i].index[2] == index.z)
            return i;
    }
    return -1;
}
Vector3 ChameleonGrid::get_chunk_index(int id) {
    Chunk * chunk = &chunks[id];
    return Vector3(chunk->index[0], chunk->index[1], chunk->index[2]);
}