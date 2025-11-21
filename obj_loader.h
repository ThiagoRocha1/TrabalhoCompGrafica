#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

typedef struct {
    float x, y, z;
} PositonRawPos;

typedef struct {
    float u, v;
} TextRawPos;

typedef struct {
    int v_idx;
    int vt_idx;
    int vn_idx;
} FaceVertex;

typedef struct {
    PositonRawPos* vertices;      
    TextRawPos* texcoords;     
    PositonRawPos* normals;       
    int num_vertices;
    int num_texcoords;
    int num_normals;

    float* interleaved_data; 
    int num_render_vertices; 
} Mesh;


Mesh* load_obj(const char* filename);
void free_mesh(Mesh* mesh);

#endif