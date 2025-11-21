#include <GL/glut.h> 
#include <stdio.h>
#include <stdlib.h>
#include "obj_loader.h" 

Mesh* g_mesh = NULL;

void cleanup() {
    if (g_mesh) {
        free_mesh(g_mesh);
        g_mesh = NULL;
    }
}

void init(const char* obj_filename) {
    g_mesh = load_obj(obj_filename);
    if (!g_mesh) {
        fprintf(stderr, "Falha ao carregar o modelo OBJ: %s. Verifique o caminho e permissões.\n", obj_filename);
        exit(1); 
    }
    printf("OBJ carregado. Total de vértices para renderização: %d\n", g_mesh->num_render_vertices);

    glClearColor(0.45f, 0.50f, 0.55f, 1.0f);
 
    glEnable(GL_DEPTH_TEST);              
    glEnable(GL_LIGHTING);                
    glEnable(GL_LIGHT0);                  
    glEnable(GL_COLOR_MATERIAL);          

    GLfloat light_position[] = {10.0f, 10.0f, 10.0f, 0.0f}; 
    GLfloat ambient_light[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat diffuse_light[] = {0.8f, 0.8f, 0.8f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    gluPerspective(60.0, (double)glutGet(GLUT_WINDOW_WIDTH) / (double)glutGet(GLUT_WINDOW_HEIGHT), 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();
    
    gluLookAt(
        1.0, 1.0, 1.0, 
        0.0, 0.0, 0.0, 
        0.0, 1.0, 0.0
    ); 

    if (g_mesh && g_mesh->interleaved_data) {
        int data_per_vertex = 8; 

        glBegin(GL_TRIANGLES);
        for (int i = 0; i < g_mesh->num_render_vertices; i++) {
            float* ptr = g_mesh->interleaved_data + (i * data_per_vertex);

            glNormal3f(ptr[3], ptr[4], ptr[5]);

            glColor3f(1.0f, 0.55f, 0.0f);

            glVertex3f(ptr[0], ptr[1], ptr[2]);
        }
        glEnd();
    }

    glutSwapBuffers(); 
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <nome_do_arquivo.obj>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s bunny.obj\n", argv[0]);
        return 1; 
    }
    
    const char* filename = argv[1]; 

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("OpenGL OBJ Renderer");

    init(filename); 

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    
    atexit(cleanup); 

    glutMainLoop(); 

    return 0;
}