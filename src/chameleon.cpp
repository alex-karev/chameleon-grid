#include "chameleon.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void ChameleonGrid::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_chunk_size"), &ChameleonGrid::get_chunk_size);
    ClassDB::bind_method(D_METHOD("set_chunk_size", "p_chunk_size"), &ChameleonGrid::set_chunk_size);
    ClassDB::add_property("ChameleonGrid", PropertyInfo(Variant::VECTOR3, "chunk_size"), "set_chunk_size", "get_chunk_size");

}

ChameleonGrid::ChameleonGrid() {
    // Initialize any variables here.
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
}