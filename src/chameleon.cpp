#include "chameleon.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void ChameleonGrid::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_chunk_size"), &ChameleonGrid::get_chunk_size);
    ClassDB::bind_method(D_METHOD("set_chunk_size", "p_chunk_size"), &ChameleonGrid::set_chunk_size);
    ClassDB::add_property("ChameleonGrid", PropertyInfo(Variant::VECTOR3I, "chunk_size"), "set_chunk_size", "get_chunk_size");

    ClassDB::bind_method(D_METHOD("add_chunk",        "chunk_index"), &ChameleonGrid::add_chunk);
    ClassDB::bind_method(D_METHOD("remove_chunk",     "chunk_id"), &ChameleonGrid::remove_chunk);
    ClassDB::bind_method(D_METHOD("count_chunks"      ), &ChameleonGrid::count_chunks);
    ClassDB::bind_method(D_METHOD("count_chunks_mem"  ), &ChameleonGrid::count_chunks_mem);
    ClassDB::bind_method(D_METHOD("optimize_chunks"   ), &ChameleonGrid::optimize_chunks);
    ClassDB::bind_method(D_METHOD("get_chunk_id",     "chunk_index"), &ChameleonGrid::get_chunk_id);
    ClassDB::bind_method(D_METHOD("get_chunk_index",  "chunk_id"), &ChameleonGrid::get_chunk_index);
    ClassDB::bind_method(D_METHOD("get_voxel",        "position"), &ChameleonGrid::get_voxel);
    ClassDB::bind_method(D_METHOD("set_voxel",        "position", "value"), &ChameleonGrid::set_voxel);
    ClassDB::bind_method(D_METHOD("get_voxel_fast",   "chunk_id", "voxel_id"), &ChameleonGrid::get_voxel_fast);
    ClassDB::bind_method(D_METHOD("set_voxel_fast",   "chunk_id", "voxel_id", "value"), &ChameleonGrid::set_voxel_fast);
    ClassDB::bind_method(D_METHOD("get_voxel_chunk",  "position"), &ChameleonGrid::get_voxel_chunk);
    ClassDB::bind_method(D_METHOD("get_voxel_id",     "position"), &ChameleonGrid::get_voxel_id);
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

void ChameleonGrid::set_chunk_size(const Vector3i size) {
    for (int i = 0; i < 3; i++)
        chunk_size[i] = size[i];
}

Vector3i ChameleonGrid::get_chunk_size(){
    return Vector3i(chunk_size[X], chunk_size[Y], chunk_size[Z]);
}

/*
* Chunks
*/

int ChameleonGrid::add_chunk(Vector3i index) {
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
    int voxel_number = chunk_size[Z]*chunk_size[Y]*chunk_size[Z];
    chunk->values.resize(voxel_number, 0);
    chunk->mask.resize(voxel_number, -1);
    for (int i = 0; i < voxel_number; i++) {
        chunk->values[i] = 0;
        chunk->mask[i] = -1;
    }
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
int ChameleonGrid::get_chunk_id(Vector3i index) {
    for (int i = 0; i < chunks.size(); i++) {
        if (!chunks[i].rewrite &&
            chunks[i].index[X] == index.x &&
            chunks[i].index[Y] == index.y &&
            chunks[i].index[Z] == index.z)
            return i;
    }
    return -1;
}
Vector3i ChameleonGrid::get_chunk_index(int id) {
    Chunk * chunk = &chunks[id];
    return Vector3i(chunk->index[X], chunk->index[Y], chunk->index[Z]);
}
int ChameleonGrid::get_voxel(Vector3i position) {
    int chunk_id = get_voxel_chunk(position);
    if (chunk_id >=0 )
        return get_voxel_fast(chunk_id, get_voxel_id(position));
    return -1;
}
void ChameleonGrid::set_voxel(Vector3i position, int value) {
    int chunk_id = get_voxel_chunk(position);
    if (chunk_id >=0 )
        set_voxel_fast(chunk_id, get_voxel_id(position), value);
}
int ChameleonGrid::get_voxel_fast(int chunk_id, int voxel_id) {
    return chunks[chunk_id].values[voxel_id];
}
void ChameleonGrid::set_voxel_fast(int chunk_id, int voxel_id, int value) {
    chunks[chunk_id].values[voxel_id] = value;
}  
int ChameleonGrid::get_voxel_chunk(Vector3i position){
    return get_chunk_id(Vector3i(
        std::floor(position.x/chunk_size[X]), 
        std::floor(position.y/chunk_size[Y]), 
        std::floor(position.z/chunk_size[Z])
    ));
}
int ChameleonGrid::get_voxel_id(Vector3i position) {
    return (position.x % chunk_size[X]) * chunk_size[Y] * chunk_size[Z] 
         + (position.y % chunk_size[Y]) * chunk_size[Z] 
         + (position.z % chunk_size[Z]);
}