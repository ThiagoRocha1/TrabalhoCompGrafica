#include <GL/glut.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> 
#include "obj_loader.h" 

Mesh* g_mesh = NULL;
int win_width, win_height; 

float g_quat_current[4] = {0.0f, 0.0f, 0.0f, 1.0f}; 
float g_quat_increment[4] = {0.0f, 0.0f, 0.0f, 1.0f}; 

float g_mouse_last_x = 0.0f;
float g_mouse_last_y = 0.0f;
int g_is_dragging = 0; 

float g_zoom_factor = 1.0f; 

static void normalize(float v[3]) {
    float len = (float)sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (len != 0.0f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

static void screen_to_sphere(float x, float y, float v[3]) {
    v[0] = (2.0f * x / win_width) - 1.0f;
    v[1] = 1.0f - (2.0f * y / win_height); 
    v[2] = 0.0f; 

    float R2 = v[0]*v[0] + v[1]*v[1];
    if (R2 > 1.0f) {
        normalize(v);
    } else {
        v[2] = (float)sqrt(1.0f - R2);
    }
}

static void quat_mult(float q_a[4], float q_b[4], float q_out[4]) {
    q_out[0] = q_a[3]*q_b[0] + q_a[0]*q_b[3] + q_a[1]*q_b[2] - q_a[2]*q_b[1];
    q_out[1] = q_a[3]*q_b[1] - q_a[0]*q_b[2] + q_a[1]*q_b[3] + q_a[2]*q_b[0];
    q_out[2] = q_a[3]*q_b[2] + q_a[0]*q_b[1] - q_a[1]*q_b[0] + q_a[2]*q_b[3];
    q_out[3] = q_a[3]*q_b[3] - q_a[0]*q_b[0] - q_a[1]*q_b[1] - q_a[2]*q_b[2];
}

static void quat_to_matrix(float q[4], float m[16]) {
    float x2 = q[0] * q[0];
    float y2 = q[1] * q[1];
    float z2 = q[2] * q[2];
    float xy = q[0] * q[1];
    float xz = q[0] * q[2];
    float yz = q[1] * q[2];
    float xw = q[0] * q[3];
    float yw = q[1] * q[3];
    float zw = q[2] * q[3];

    m[0] = 1.0f - 2.0f * (y2 + z2); m[4] = 2.0f * (xy - zw);        m[8] = 2.0f * (xz + yw);        m[12] = 0.0f;
    m[1] = 2.0f * (xy + zw);        m[5] = 1.0f - 2.0f * (x2 + z2); m[9] = 2.0f * (yz - xw);        m[13] = 0.0f;
    m[2] = 2.0f * (xz - yw);        m[6] = 2.0f * (yz + xw);        m[10] = 1.0f - 2.0f * (x2 + y2); m[14] = 0.0f;
    m[3] = 0.0f;                    m[7] = 0.0f;                    m[11] = 0.0f;                   m[15] = 1.0f;
}

void mouse_click(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            g_is_dragging = 1;
            g_mouse_last_x = (float)x;
            g_mouse_last_y = (float)y;
            
            float temp_quat[4];
            quat_mult(g_quat_increment, g_quat_current, temp_quat);
            memcpy(g_quat_current, temp_quat, sizeof(float) * 4);
            
            g_quat_increment[0] = g_quat_increment[1] = g_quat_increment[2] = 0.0f;
            g_quat_increment[3] = 1.0f;

        } else { 
            g_is_dragging = 0;
            glutPostRedisplay();
        }
    } 
}

void mouse_motion(int x, int y) {
    if (g_is_dragging) {
        float v_start[3], v_end[3];
        float axis[3];
        
        screen_to_sphere(g_mouse_last_x, g_mouse_last_y, v_start);
        screen_to_sphere((float)x, (float)y, v_end);

        axis[0] = v_start[1]*v_end[2] - v_start[2]*v_end[1];
        axis[1] = v_start[2]*v_end[0] - v_start[0]*v_end[2];
        axis[2] = v_start[0]*v_end[1] - v_start[1]*v_end[0];
        
        float dot = v_start[0]*v_end[0] + v_start[1]*v_end[1] + v_start[2]*v_end[2];
        if (dot > 1.0f) dot = 1.0f;
        else if (dot < -1.0f) dot = -1.0f;

        float angle = (float)acos(dot);
        
        normalize(axis);
        float sin_a = (float)sin(angle / 2.0f);

        g_quat_increment[0] = axis[0] * sin_a;
        g_quat_increment[1] = axis[1] * sin_a;
        g_quat_increment[2] = axis[2] * sin_a;
        g_quat_increment[3] = (float)cos(angle / 2.0f);
        
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    float zoom_step = 0.1f; 

    if (key == 'i' || key == 'I') {
        g_zoom_factor -= zoom_step;
        if (g_zoom_factor < 0.2f) g_zoom_factor = 0.2f;
        glutPostRedisplay();
    } else if (key == 'o' || key == 'O') {
        g_zoom_factor += zoom_step;
        if (g_zoom_factor > 10.0f) g_zoom_factor = 10.0f;
        glutPostRedisplay();
    }
}

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
    gluPerspective(60.0, (double)win_width / (double)win_height, 0.001, 1000.0);

    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();
    
    gluLookAt(
        1.0 * g_zoom_factor, 1.0 * g_zoom_factor, 1.0 * g_zoom_factor, 
        0.0, 0.0, 0.0, 
        0.0, 1.0, 0.0
    ); 

    float rotation_matrix[16];
    float final_quat[4];
    
    quat_mult(g_quat_increment, g_quat_current, final_quat);
    
    quat_to_matrix(final_quat, rotation_matrix);
    
    glMultMatrixf(rotation_matrix);

    glScalef(1.05f,1.05f, 1.05f); 

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
    win_width = w;
    win_height = h;
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

    win_width = 800;
    win_height = 600;

    init(filename); 

    glutMouseFunc(mouse_click);
    glutMotionFunc(mouse_motion);
    glutKeyboardFunc(keyboard);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    
    atexit(cleanup); 

    glutMainLoop(); 

    return 0;
}