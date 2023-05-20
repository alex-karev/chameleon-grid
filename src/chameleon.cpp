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
    ClassDB::bind_method(D_METHOD("update_chunk",     "chunk_id"), &ChameleonGrid::update_chunk);
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
    chunk->values.resize(voxel_number, -1);
    chunk->mask.resize(voxel_number, -1);
    for (int i = 0; i < voxel_number; i++) {
        chunk->values[i] = -1;
        chunk->mask[i] = -1;
    }
    chunk->rewrite = 0;
    chunk_number += 1;
    // Add new mesh instance
    MeshInstance3D mesh_instance;
    add_child(&mesh_instance);
    chunk->mesh_instance = (MeshInstance3D*)get_child(get_child_count()-1);
    // Return chunk id
    return id;
}
void ChameleonGrid::remove_chunk(int id) {
    // Mark chunk for rewrite and update chunk counter
    chunks[id].rewrite = 1;
    chunk_number -= 1;
    // Remove mesh instance
    chunks[id].mesh_instance->queue_free();
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

void ChameleonGrid::update_chunk(int id) {
    // Ignore unexisting chunks
    if (id >= chunks.size() || chunks[id].rewrite)
        return;
    // Define varaibles needed for mesh generation
    std::vector<Vector3> vertices;
    std::vector<int> faces;
    int n = 0;
    int R[3] = {1, chunk_size[X]+1, (chunk_size[X]+1)*(chunk_size[Y]+1)};
    int grid[8] = {0,0,0,0,0,0,0,0};
    int buf_no = 1;
    Vector3 pos = Vector3(0,0,0);
	
    // Internal buffer
    std::vector<int> buffer;
    buffer.resize(R[Z] * 2, 0);

	// March over the voxel grid
	for (int z = 0; z < chunk_size[Z]-1; ++z, n+=chunk_size[X], buf_no^=1, R[Z]=-R[Z]) {
        /*
		* m is the pointer into the buffer we are going to use.  
		* This is slightly obtuse because javascript does not have good support for packed data structures, so we must use typed arrays :(
		* The contents of the buffer will be the indices of the vertices on the previous x/y slice of the volume
		*/
        int m = 1 + (chunk_size[X]+1) * (1 + buf_no * (chunk_size[Y]+1));
        for (int y = 0; y < chunk_size[Y]-1; ++y, ++n, m+=2)
        for (int x = 0; x < chunk_size[X]-1; ++x, ++n, ++m) {
            pos = Vector3(x,y,z);
            // Read in 8 field values around this vertex and store them in an array
            // Also calculate 8-bit mask, like in marching cubes, so we can speed up sign checks later
            int mask = 0, g = 0, idx = n;
            for (int i = 0; i < 2; i++, idx += chunk_size[X]*(chunk_size[Y]-2))
            for (int j = 0; j < 2; j++, idx += chunk_size[X]-2)
            for (int k = 0; k < 2; k++, ++g, ++idx) {
                int p = get_voxel_fast(id, idx);
                if (p < 0) {
                    grid[g] = 0;
                    mask |= 1<<g;
                }
                else {
                    grid[g] = 1;
                    mask |= 0;
                }
            }
            // Check for early termination if cell does not intersect boundary
            if (mask == 0 || mask == 0xff)
                continue;
            // Sum up edge intersections
            int edge_mask = EDGE_TABLE[mask];
            Vector3 v = Vector3(0,0,0);
            int e_count = 0;
            // For every edge of the cube...
            for (int i = 0; i < 12; ++i) {
            // Use edge mask to check if it is crossed
                if (!(edge_mask & (1<<i)))
                    continue;
                // If it did, increment number of edge crossings
                e_count += 1;
                // Now find the point of intersection
                int e0 = CUBE_EDGES[ i<<1 ]; // Unpack vertices
                int e1 = CUBE_EDGES[(i<<1)+1];
                // TODO: REFACTORING NEEDED
                int t = std::max(grid[e0]-grid[e1], 0); // Compute point of intersection
                // Interpolate vertices and add up intersections (this can be done without multiplying)
                for (int j = 0, k = 1; j < 3; ++j, k <<= 1) {
                    int a = e0 & k;
                    int b = e1 & k;
                    if (a != b)
                        v[j] += a ? 1.0 - t : t;
                    else
                        v[j] += a ? 1.0 : 0;
                }
            }
            // Now we just average the edge intersections and add them to coordinate
            v *= 1.0 / e_count;
            v += pos;
            // Add vertex to buffer, store pointer to vertex index in buffer
            buffer[m] = vertices.size();
            vertices.push_back(v);
            // Now we need to add faces together, to do this we just loop over 3 basis components
            for (int i = 0; i < 3; ++i) {
                // The first three entries of the edge_mask count the crossings along the edge
                if (!(edge_mask & (1<<i)))
                    continue;
                // i = axes we are point along.  iu, iv = orthogonal axes
                int iu = (i+1)%3;
                int iv = (i+2)%3;
                // If we are on a boundary, skip it
                if (pos[iu] == 0 || pos[iv] == 0)
                    continue;
                // Otherwise, look up adjacent edges in buffer
                int du = R[iu];
                int dv = R[iv];
                // Remember to flip orientation depending on the sign of the corner.
                if (mask & 1) {
                    faces.push_back(buffer[m]);
                    faces.push_back(buffer[m-du]);
                    faces.push_back(buffer[m-du-dv]);
                    faces.push_back(buffer[m-dv]);
                }
                else {
                    faces.push_back(buffer[m]);
                    faces.push_back(buffer[m-dv]);
                    faces.push_back(buffer[m-du-dv]);
                    faces.push_back(buffer[m-du]);
                }
            }
        }
    }
    SurfaceTool st;
	st.begin(Mesh::PRIMITIVE_TRIANGLES);
    for (int i = 0; i < faces.size()/4; i++)
    for (int j = 0; j < 6; j++) {
		st.set_smooth_group(-1);
        st.add_vertex(vertices[faces[i*4+QUAD_TO_TRIGS[j]]]);
    }
	st.generate_normals();
	st.index();
    chunks[id].mesh_instance->set_mesh(st.commit());
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