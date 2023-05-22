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
    ClassDB::bind_method(D_METHOD("add_material",     "material", "atlas_size"), &ChameleonGrid::add_material);
    ClassDB::bind_method(D_METHOD("add_voxel",        "material", "atlas_index[6]", "smoothness", "smooth_shading"), &ChameleonGrid::add_voxel);
    ClassDB::bind_method(D_METHOD("remesh"            ), &ChameleonGrid::remesh);
}

ChameleonGrid::ChameleonGrid() {
    // Initialize any variables here.
    for (int i = 0; i < 3; i++)
        chunk_size[i] = DEFAULT_CHUNK_SIZE[i];
    chunk_number = 0;
    //DEFAULT_MATERIAL.material = Ref<StandardMaterial3D>();
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
        chunks.push_back(ChameleonChunk());
        id = chunks.size()-1;
    }
    // Reset chunk values
    ChameleonChunk * chunk = &chunks[id];
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
    // Return chunk id
    return id;
}
void ChameleonGrid::remove_chunk(int id) {
    // Mark chunk for rewrite and update chunk counter
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
    ChameleonChunk * chunk = &chunks[id];
    return Vector3i(chunk->index[X], chunk->index[Y], chunk->index[Z]);
}

void ChameleonGrid::update_chunk_mesh_data(int chunk_id) {
    // Define varaibles needed for mesh generation
    ChameleonChunk * chunk = &chunks[chunk_id];
    chunk->faces.clear();
    chunk->vertices.clear();
    chunk->face_values.clear();
    int n = 0;
    int R[3] = {1, chunk_size[X]+1, (chunk_size[X]+1)*(chunk_size[Y]+1)};
    int grid[8] = {0,0,0,0,0,0,0,0};
    int buf_no = 1;
    Vector3 pos = Vector3(0,0,0);
	double smoothness = DEFAULT_VOXEL.smoothness;

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
            // Set voxel parameters  
            pos = Vector3(x,y,z);
            smoothness = DEFAULT_VOXEL.smoothness;
            // Read in 8 field values around this vertex and store them in an array
            // Also calculate 8-bit mask, like in marching cubes, so we can speed up sign checks later
            int mask = 0, g = 0, idx = n;
            for (int i = 0; i < 2; i++, idx += chunk_size[X]*(chunk_size[Y]-2))
            for (int j = 0; j < 2; j++, idx += chunk_size[X]-2)
            for (int k = 0; k < 2; k++, ++g, ++idx) {
                int p = chunk->values[idx];
                double p_smoothness = DEFAULT_VOXEL.smoothness;
                if (p < 0) {
                    grid[g] = p;
                    mask |= 1<<g;
                }
                else {
                    p_smoothness = voxels[p].smoothness;
                    grid[g] = p;
                    mask |= 0;
                }
                if (p_smoothness < smoothness)
                    smoothness = p_smoothness;
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
                int g0 = grid[e0] > 0 ? 1 : 0;
                int g1 = grid[e1] > 0 ? 1 : 0;
                int t = g0-g1; // Compute point of intersection
                // Interpolate vertices and add up intersections (this can be done without multiplying)
                for (int j = 0, k = 1; j < 3; ++j, k <<= 1) {
                    int a = e0 & k;
                    int b = e1 & k;
                    if (a != b) {
                        v[j] += a ? t/2 : -t/2;
                    }
                    else
                        v[j] += a ? 0.5 : -0.5;
                }
            }
            // Now we just average the edge intersections and add them to coordinate
            v = pos + v * (1.0 / e_count) * smoothness;
            // Add vertex to buffer, store pointer to vertex index in buffer
            buffer[m] = chunk->vertices.size();
            chunk->vertices.push_back(v);
            // Skipt the first row
            if (x==0 || y==0 || z==0)
                continue;
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
                // TODO: REFACTORING
                // Remember to flip orientation depending on the sign of the corner.
                int value = 0;
                if (mask & 1) {
                    if (i == 0) value = grid[1];
                    else if (i==1) value = grid[2];
                    else value = grid[4];
                    chunk->faces.push_back(buffer[m]);
                    chunk->faces.push_back(buffer[m-du]);
                    chunk->faces.push_back(buffer[m-du-dv]);
                    chunk->faces.push_back(buffer[m-dv]);
                }
                else {
                    value = grid[0];
                    chunk->faces.push_back(buffer[m]);
                    chunk->faces.push_back(buffer[m-dv]);
                    chunk->faces.push_back(buffer[m-du-dv]);
                    chunk->faces.push_back(buffer[m-du]);
                }
                chunk->face_values.push_back(value);
            }
        }
    }
}

void ChameleonGrid::remesh() {
    mesh_instance = (MeshInstance3D*)get_child(get_child_count()-1);
    // TODO: REFACTORING
    Ref<ArrayMesh> mesh;
    std::vector<SurfaceTool> st;
    st.resize(materials.size());
    std::vector<int> stn;
    stn.resize(materials.size(), 0);
    for (int i = 0; i < st.size(); i++)
        st[i].begin(Mesh::PRIMITIVE_TRIANGLES);

    for (ChameleonChunk & chunk : chunks) {
        for (int i = 0; i < chunk.face_values.size(); i++) {
            ChameleonVoxel * voxel = &voxels[chunk.face_values[i]];
            int mat = voxel->material;
            int shade = voxel->smooth_shading;
            stn[mat] += 1;
            for (int k = 0; k < 6; k++) {
                int v = chunk.faces[i*4+QUAD_TO_TRIGS[k]];
                if (shade)
                    st[mat].set_smooth_group(0);
                else
                    st[mat].set_smooth_group(-1);
                st[mat].set_uv(Vector2(CUBE_UV[k*2], CUBE_UV[k*2+1]));
                Vector3 offset = Vector3(chunk.index[X]*(chunk_size[X]-2), chunk.index[Y]*(chunk_size[Y]-2), chunk.index[Z]*(chunk_size[Z]-2));
                st[mat].add_vertex(chunk.vertices[v]+offset);
            }
        }
    }

    int surface = 0;
    for(int i = 0; i < st.size(); i++) {
        if (stn[i] > 0) {
            st[i].generate_normals();
            st[i].index();
            mesh = st[i].commit(mesh);
            mesh->surface_set_material(surface, materials[i].material);
            surface += 1;
        }
    }
    mesh_instance->set_mesh(mesh);
}

void ChameleonGrid::update_chunk(int id) {
   // Ignore unexisting chunks
    if (id >= chunks.size() || chunks[id].rewrite)
        return;
    update_chunk_mesh_data(id);
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
void ChameleonGrid::add_material(Ref<Material> material, Vector2i atlas_size) {
    ChameleonMaterial mat;
    mat.material = material;
    mat.atlas_size[0] = atlas_size.x;
    mat.atlas_size[1] = atlas_size.y;
    materials.push_back(mat);
}
void ChameleonGrid::add_voxel(int material, TypedArray<int> atlas_index, double smoothness, bool smooth_shading) {
    ERR_FAIL_COND_MSG(atlas_index.size() != 6, "Invalid atlas index provided. Should be an array of 6 integers for each side of the voxel");
    ChameleonVoxel voxel;
    voxel.material = material;
    for (int i = 0; i < 6; i++)
        voxel.atlas_index[i] = atlas_index[i];
    voxel.smoothness = smoothness;
    voxel.smooth_shading = smooth_shading;
    voxels.push_back(voxel);
}
