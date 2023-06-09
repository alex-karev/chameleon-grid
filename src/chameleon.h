#ifndef CHAMELEONGRID_H
#define CHAMELEONGRID_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace godot {

class ChameleonGrid : public Node3D {
    GDCLASS(ChameleonGrid, Node3D)

protected:
    static void _bind_methods();

public:
    ChameleonGrid();
    ~ChameleonGrid();
    
    //! Set chunk size
    void set_chunk_size(const Vector3i size);
    //! Get chunk size
    Vector3i get_chunk_size();

    //! Add new empty chunk, return its id
    int add_chunk(Vector3i index);
    //! Remove chunk (mark it for rewrite)
    void remove_chunk(int id);
    //! Count active chunks
    int count_chunks();
    //! Count all chunks cached in memory
    int count_chunks_mem();
    /*! 
    * Optimize memory by removing chunks marked for rewrite. 
    * WARNING: It will also change ids of existing chunks
    */
    void optimize_chunks();
    //! Find chunk by Vector3i index and return it's id. Returns -1 if not found
    int get_chunk_id(Vector3i index);
    //! Get Vector3i index of a chunk
    Vector3i get_chunk_index(int id);
    //! Update mesh of a chunk
    void update_chunk(int chunk_id);
    //! Get voxel by global position. Returns -1 if chunk is not loaded
    int get_voxel(Vector3i position);
    //! Set voxel by global position. Does nothing if chunk is not loaded
    void set_voxel(Vector3i position, int value);
    //! Get voxel in specified chunk (fast). Chunk id and voxel id are required
    int get_voxel_fast(int chunk_id, int voxel_id);
    //! Set voxel in specified chunk (fast). Chunk id and voxel id are required
    void set_voxel_fast(int chunk_id, int voxel_id, int value);
    //! Get chunk id in voxel position. Returns -1 if chunk is not loaded
    int get_voxel_chunk(Vector3i position);
    //! Get voxel id inside its chunk.
    int get_voxel_id(Vector3i position);
    
    //! Add new voxel material
    void add_material(Ref<Material> material, Vector2i atlas_size);
    //! Add new voxel type
    void add_voxel(int material, TypedArray<int> atlas_index, double smoothness, bool smooth_shading);


private:
    // Material structure
    struct ChameleonMaterial {
        Ref<Material> material;
        int atlas_size[2] = {1,1};
    };
    // Material database
    std::vector<ChameleonMaterial> materials;
    
    // Voxel structure
    struct ChameleonVoxel {
        int material = -1;
        int atlas_index[6] = {0,0,0,0,0,0};
        double smoothness = 1.0;
        int smooth_shading = 1;
    };
    // Voxel database
    std::vector<ChameleonVoxel> voxels;

    // Chunk structure
    struct ChameleonChunk {
        int index[3];
        std::vector<int> values;
        std::vector<int> mask;
        MeshInstance3D * mesh_instance;
        int rewrite;
    };

    // Chunk storage
    std::vector<ChameleonChunk> chunks;
    // Chunk number
    int chunk_number;

    // Size of one chunk
    int chunk_size[3];
    
    // Default size of one chunk
    const int DEFAULT_CHUNK_SIZE[3] = {10,10,10};

    // Default voxel to be used when voxel id is invalid
    const ChameleonVoxel DEFAULT_VOXEL;

    // Default material to be used with default voxel
    ChameleonMaterial DEFAULT_MATERIAL;

    // Useful when using arrays instead of vectors
    enum {X,Y,Z};

    /* 
    * Precomputed edge table
    * This saves a bit of time when computing the centroid of each boundary cell
    * See https://github.com/mikolalysenko/mikolalysenko.github.com/blob/master/Isosurface/js/surfacenets.js 
    * for original code that generates it
    */
    int CUBE_EDGES[24] = {
        0, 1, 0, 2, 0, 4, 1, 3, 
        1, 5, 2, 3, 2, 6, 3, 7, 
        4, 5, 4, 6, 5, 7, 6, 7
    };
    int EDGE_TABLE[256] = {
        0,    7,    25,   30,   98,   101,  123,  124,  168,  175,  177,  182,  202,  205,  211,  212, 
        772,  771,  797,  794,  870,  865,  895,  888,  940,  939,  949,  946,  974,  969,  983,  976, 
        1296, 1303, 1289, 1294, 1394, 1397, 1387, 1388, 1464, 1471, 1441, 1446, 1498, 1501, 1475, 1476, 
        1556, 1555, 1549, 1546, 1654, 1649, 1647, 1640, 1724, 1723, 1701, 1698, 1758, 1753, 1735, 1728, 
        2624, 2631, 2649, 2654, 2594, 2597, 2619, 2620, 2792, 2799, 2801, 2806, 2698, 2701, 2707, 2708, 
        2372, 2371, 2397, 2394, 2342, 2337, 2367, 2360, 2540, 2539, 2549, 2546, 2446, 2441, 2455, 2448, 
        3920, 3927, 3913, 3918, 3890, 3893, 3883, 3884, 4088, 4095, 4065, 4070, 3994, 3997, 3971, 3972, 
        3156, 3155, 3149, 3146, 3126, 3121, 3119, 3112, 3324, 3323, 3301, 3298, 3230, 3225, 3207, 3200, 
        3200, 3207, 3225, 3230, 3298, 3301, 3323, 3324, 3112, 3119, 3121, 3126, 3146, 3149, 3155, 3156, 
        3972, 3971, 3997, 3994, 4070, 4065, 4095, 4088, 3884, 3883, 3893, 3890, 3918, 3913, 3927, 3920, 
        2448, 2455, 2441, 2446, 2546, 2549, 2539, 2540, 2360, 2367, 2337, 2342, 2394, 2397, 2371, 2372, 
        2708, 2707, 2701, 2698, 2806, 2801, 2799, 2792, 2620, 2619, 2597, 2594, 2654, 2649, 2631, 2624, 
        1728, 1735, 1753, 1758, 1698, 1701, 1723, 1724, 1640, 1647, 1649, 1654, 1546, 1549, 1555, 1556, 
        1476, 1475, 1501, 1498, 1446, 1441, 1471, 1464, 1388, 1387, 1397, 1394, 1294, 1289, 1303, 1296, 
        976,  983,  969,  974,  946,  949,  939,  940,  888,  895,  865,  870,  794,  797,  771,  772, 
        212,  211,  205,  202,  182,  177,  175,  168,  124,  123,  101,  98,   30,   25,   7,    0
    };
    // Remaps vertex indices from 1 quad into 2 trigs
    const int QUAD_TO_TRIGS[6] = {1,3,0,2,3,1};
    // UV map of triangles in one face
    int CUBE_UV[16] = {
        0,0, 1,0, 1,1, 0,1,
        0,0, 0,1, 1,1, 1,0
    };
};

}

#endif