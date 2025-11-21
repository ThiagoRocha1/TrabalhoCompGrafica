#include "obj_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

static int get_actual_index(int index, int total_elements) {
    if (index > 0) {
        return index - 1; 
    } else if (index < 0) {
        return total_elements + index; 
    }
    return -1; 
}

void free_mesh(Mesh* mesh) {
    if (!mesh) return;
    
    if (mesh->vertices) free(mesh->vertices);
    if (mesh->texcoords) free(mesh->texcoords);
    if (mesh->normals) free(mesh->normals);
    
    if (mesh->interleaved_data) free(mesh->interleaved_data);
    
    free(mesh); 
}

Mesh* load_obj(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", filename);
        return NULL;
    }

    Mesh* final_mesh = (Mesh*)calloc(1, sizeof(Mesh));
    if (!final_mesh) {
        fclose(file);
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        if (line[0] == 'v') {
            if (line[1] == ' ') final_mesh->num_vertices++;
            else if (line[1] == 't') final_mesh->num_texcoords++;
            else if (line[1] == 'n') final_mesh->num_normals++;
        }
    }

    final_mesh->vertices = (PositonRawPos*)malloc(final_mesh->num_vertices * sizeof(PositonRawPos));
    final_mesh->texcoords = (TextRawPos*)malloc(final_mesh->num_texcoords * sizeof(TextRawPos));
    final_mesh->normals = (PositonRawPos*)malloc(final_mesh->num_normals * sizeof(PositonRawPos));

    if (!final_mesh->vertices || !final_mesh->texcoords || !final_mesh->normals) {
        free_mesh(final_mesh);
        fclose(file);
        return NULL;
    }

    rewind(file); 

    int v_count = 0, vt_count = 0, vn_count = 0;
    int faces_capacity = 1000; 
    FaceVertex* render_indices = (FaceVertex*)malloc(faces_capacity * 3 * sizeof(FaceVertex));
    if (!render_indices) {
        free_mesh(final_mesh);
        fclose(file);
        return NULL;
    }

    while (fgets(line, MAX_LINE_LENGTH, file)) {
        if (line[0] == 'v') {
            if (line[1] == ' ') { 
                sscanf(line, "v %f %f %f", &final_mesh->vertices[v_count].x, &final_mesh->vertices[v_count].y, &final_mesh->vertices[v_count].z);
                v_count++;
            } else if (line[1] == 't') { 
                sscanf(line, "vt %f %f", &final_mesh->texcoords[vt_count].u, &final_mesh->texcoords[vt_count].v);
                vt_count++;
            } else if (line[1] == 'n') { 
                sscanf(line, "vn %f %f %f", &final_mesh->normals[vn_count].x, &final_mesh->normals[vn_count].y, &final_mesh->normals[vn_count].z);
                vn_count++;
            }
        } else if (line[0] == 'f') { 
            // CORREÇÃO: Adicionado \t (tabulação) como delimitador
            char* token = strtok(line + 1, " \t\n\r"); 
            FaceVertex face_v[32]; 
            int current_face_v_count = 0;

            while (token) {
                int v = 0, vt = 0, vn = 0;
                char* p1 = strchr(token, '/');
                char* p2 = p1 ? strchr(p1 + 1, '/') : NULL;

                if (p1 == NULL) sscanf(token, "%d", &v);
                else if (p2 == NULL) sscanf(token, "%d/%d", &v, &vt);
                else if (*(p1 + 1) == '/') sscanf(token, "%d//%d", &v, &vn);
                else sscanf(token, "%d/%d/%d", &v, &vt, &vn);

                if (current_face_v_count < 32) {
                    face_v[current_face_v_count].v_idx = get_actual_index(v, final_mesh->num_vertices);
                    face_v[current_face_v_count].vt_idx = get_actual_index(vt, final_mesh->num_texcoords);
                    face_v[current_face_v_count].vn_idx = get_actual_index(vn, final_mesh->num_normals);
                    current_face_v_count++;
                }
                token = strtok(NULL, " \t\n\r"); // CORREÇÃO: Manter o mesmo delimitador
            }

            for (int i = 1; i < current_face_v_count - 1; i++) {
                if (final_mesh->num_render_vertices + 3 > faces_capacity * 3) {
                    faces_capacity *= 2;
                    render_indices = (FaceVertex*)realloc(render_indices, faces_capacity * 3 * sizeof(FaceVertex));
                    if (!render_indices) {
                        free_mesh(final_mesh);
                        fclose(file);
                        return NULL;
                    }
                }

                render_indices[final_mesh->num_render_vertices++] = face_v[0];
                render_indices[final_mesh->num_render_vertices++] = face_v[i];
                render_indices[final_mesh->num_render_vertices++] = face_v[i + 1];
            }
        }
    }
    fclose(file);

    int data_per_vertex = 8; 
    final_mesh->interleaved_data = (float*)malloc(final_mesh->num_render_vertices * data_per_vertex * sizeof(float));
    if (!final_mesh->interleaved_data) {
        free(render_indices);
        free_mesh(final_mesh);
        return NULL;
    }

    for (int i = 0; i < final_mesh->num_render_vertices; i++) {
        FaceVertex fv = render_indices[i];
        float* ptr = final_mesh->interleaved_data + (i * data_per_vertex);

        if (fv.v_idx != -1) {
            ptr[0] = final_mesh->vertices[fv.v_idx].x;
            ptr[1] = final_mesh->vertices[fv.v_idx].y;
            ptr[2] = final_mesh->vertices[fv.v_idx].z;
        } else { ptr[0] = ptr[1] = ptr[2] = 0.0f; }

        if (fv.vn_idx != -1) {
            ptr[3] = final_mesh->normals[fv.vn_idx].x;
            ptr[4] = final_mesh->normals[fv.vn_idx].y;
            ptr[5] = final_mesh->normals[fv.vn_idx].z;
        } else { ptr[3] = 0.0f; ptr[4] = 0.0f; ptr[5] = 1.0f; }

        if (fv.vt_idx != -1) {
            ptr[6] = final_mesh->texcoords[fv.vt_idx].u;
            ptr[7] = final_mesh->texcoords[fv.vt_idx].v;
        } else { ptr[6] = ptr[7] = 0.0f; }
    }

    free(render_indices); 
    
    free(final_mesh->vertices);
    free(final_mesh->texcoords);
    free(final_mesh->normals);
    final_mesh->vertices = NULL;
    final_mesh->texcoords = NULL;
    final_mesh->normals = NULL;

    return final_mesh;
}