// This includes more headers than you will probably need.
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <iostream>
#include <sstream>
#include <map>
#include <utility> // pair
#include <list>
#include <vector>
#include <limits>
#include <string>
#include <cstring>
#include <fstream>
#include <algorithm> //swap
#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "tools.h" // this calls half_edge.h

using namespace std;

struct Point_Light
{
    float position[4];
    float color[3];
    float attenuation_k;
};

// prototype functions
GLuint loadBMP_custom(const char * imagepath);

//gl prototypes
void init(void);
void reshape(int width, int height);
void display(void);

void init_lights();
void set_lights();
void draw_objects();
void initTextures();

void mouse_pressed(int button, int state, int x, int y);
void mouse_moved(int x, int y);
void key_pressed(unsigned char key, int x, int y);

void create_lights();

void set_face_normals();
void set_vertex_normals();
void draw_faces();


//gl globals
GLUquadricObj *quadratic;
GLuint image;

int mouse_x, mouse_y;
float mouse_scale_x, mouse_scale_y;
static const float gridSize = 100.0;
const float x_view_step = 90.0, y_view_step = 90.0;
float x_view_angle = 0, y_view_angle = 0;

bool is_pressed = false;

float cam_position[] = {0, 0, 2};

float cam_orientation_axis[] = {1, 1, 1};

float cam_orientation_angle = 0; // degrees

float near_param = 1, far_param = 10,
      left_param = -1, right_param = 1,
      top_param = 1, bottom_param = -1;

vector<Point_Light> lights;

char path_to_bmp[125]; // store the bmp texture file path

Mesh * mesh_data = new Mesh;
vector<glm::vec3> vertices;
vector<glm::vec3> vertex_normals;
vector<glm::vec3> face_normals;
vector<float> face_areas;
half_edge::half_edge_t mesh;

/* This function splits a given string into tokens based on the specified
 * delimiter and stores the tokens into a preconstructed, passed-in vector.
 */
vector<string> &split(const string &str, char delim, vector<string> &tokens)
{
    stringstream ss(str);
    string token;

    while (getline(ss, token, delim))
        if(!token.empty())
            tokens.push_back(token);

    return tokens;
}

/* This function splits a given string into tokens based on the specified
 * delimiter and returns a new vector with all the tokens.
 */
vector<string> split(const string &str, char delim)
{
    vector<string> tokens;
    split(str, delim, tokens);
    return tokens;
}

/* String to integer conversion.
 */
float stoi(const string str)
{
    return atoi(str.c_str());
}

/* String to float conversion.
 */
float stof(const string str)
{
    return atof(str.c_str());
}

/* This function fills a passed-in mesh data container with a list of vertices
 * and faces specified by the given .obj file.
 */
void parse(istream &data)
{
    mesh_data->points = new vector<point*>;
    mesh_data->faces = new vector<face*>;

    string read_line;
    while(getline(data, read_line))
    {
        vector<string> tokens = split(read_line, ' ');

        if(tokens[0].at(0) == 'v')
        {
            point *p = new point;
            p->x = stof(tokens[1]);
            p->y = stof(tokens[2]);
            p->z = stof(tokens[3]);

            mesh_data->points->push_back(p);
        }
        else if(tokens[0].at(0) == 'f')
        {
            face *f = new face;
            // SUBTRACT ONE because starts at 1, not 0!
            f->ind1 = stoi(tokens[1]) - 1;
            f->ind2 = stoi(tokens[2]) - 1;
            f->ind3 = stoi(tokens[3]) - 1;

            mesh_data->faces->push_back(f);
        }
    }
}

void set_vertex_normals()
{
    int num_vertices = mesh_data->points->size();
    vector< half_edge::index_t > neighs;
    for( int vi = 0; vi < num_vertices; vi++ )
    {
        mesh.vertex_face_neighbors( vi, neighs );
        glm::vec3 vertex_normal(0.0, 0.0, 0.0);

        if (!mesh.vertex_is_boundary(vi))
        {
            for( int i = 0; i < neighs.size(); i++ )
            {
                vertex_normal += face_areas[neighs.at(i)] * \
                    face_normals[neighs.at(i)];
            }
            vertex_normal = glm::normalize(vertex_normal);
            bump_map(vertex_normal);
        }

        vertex_normals.push_back(vertex_normal);
    }
}

GLuint loadBMP_custom(const char * imagepath)
{
    // Data read from the header of the BMP file
    // Each BMP file begins by a 54-bytes header
    unsigned char header[54];
    // Position in the file where the actual data begins
    unsigned int dataPos;
    unsigned int width, height;
    unsigned int imageSize; // = width*height*3
    // Actual RGB data
    unsigned char * data;

    // Open the file
    printf("filename: %s\n",imagepath);
    FILE * file = fopen(imagepath,"rb");
    if (!file)
    {
        printf("Image could not be opened\n");
        return 0;
    }

    if ( fread(header, 1, 54, file)!=54 )
    { 
        // If not 54 bytes read : problem
        printf("Not a correct BMP file\n");
        return false;
    }

    if ( header[0]!='B' || header[1]!='M' ){
        printf("Not a correct BMP file\n");
        return 0;
    }

    // Read ints from the byte array
    dataPos    = *(int*)&(header[0x0A]);
    imageSize  = *(int*)&(header[0x22]);
    width      = *(int*)&(header[0x12]);
    height     = *(int*)&(header[0x16]);

    // Create a buffer
    data = new unsigned char [imageSize];
     
    // Read the actual data from the file into the buffer
    fread(data,1,imageSize,file);
     
    //Everything is in memory now, the file can be closed
    fclose(file);

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
     
    // "Bind" the newly created texture:
    // all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);
     
    // Give the image to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, \
        GL_BGR, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return textureID;
}


void initTextures()
{
    image = loadBMP_custom(path_to_bmp);
}

void init(void)
{
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(left_param, right_param,
              bottom_param, top_param,
              near_param, far_param);
    glMatrixMode(GL_MODELVIEW); 

    quadratic = gluNewQuadric();
    gluQuadricNormals(quadratic, GLU_SMOOTH);
    gluQuadricTexture(quadratic, GL_TRUE);

    initTextures();

    create_lights();
    init_lights();
}

void reshape(int width, int height)
{
    height = (height == 0) ? 1 : height;
    width = (width == 0) ? 1 : width;
    glViewport(0, 0, width, height);
    mouse_scale_x = (float) (right_param - left_param) / (float) width;
    mouse_scale_y = (float) (top_param - bottom_param) / (float) height;
    glutPostRedisplay();
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glRotatef(-cam_orientation_angle,
              cam_orientation_axis[0], cam_orientation_axis[1],
              cam_orientation_axis[2]);
    glTranslatef(-cam_position[0], -cam_position[1], -cam_position[2]);
    set_lights();
    glEnable(GL_TEXTURE_2D);
    draw_objects();
    glutSwapBuffers();
}

void init_lights()
{
    glEnable(GL_LIGHTING);
    int num_lights = lights.size();
    for(int i = 0; i < num_lights; ++i)
    {
        int light_id = GL_LIGHT0 + i;
        glEnable(light_id);
        glLightfv(light_id, GL_AMBIENT, lights[i].color);
        glLightfv(light_id, GL_DIFFUSE, lights[i].color);
        glLightfv(light_id, GL_SPECULAR, lights[i].color);
        glLightf(light_id, GL_QUADRATIC_ATTENUATION, lights[i].attenuation_k);
    }
}

void set_face_normals()
{
    vector<point *> * points = mesh_data->points;
    vector<face *> * faces = mesh_data->faces;

    for (int i = 0; i < faces->size(); i++)
    {
        face * curr = (*faces)[i];
        int ia = curr->ind1;
        int ib = curr->ind2;
        int ic = curr->ind3;
        
        glm::vec3 vert1, vert2, vert3;
        point2vector((*points)[ia], vert1);
        point2vector((*points)[ib], vert2);
        point2vector((*points)[ic], vert3);
                
        glm::vec3 normal;
        normal = glm::normalize(glm::cross(vert2 - vert1, \
            vert3 - vert1));
        face_normals.push_back(normal);

        float area = 0;
        area += glm::length(glm::cross(vert2 - vert1, vert3 - vert1));
        face_areas.push_back(area / 2);
    }
}

void draw_faces()
{
    vector<face*> * faces = mesh_data->faces;
    vector<point*> * vertices = mesh_data->points;
    for (int f = 0; f < faces->size(); f++)
    {   
        face * curr = (*faces)[f];
        glBegin(GL_TRIANGLES); 
        glTexCoord2d(0.4, 0.4);
        glNormal3f(vertex_normals[curr->ind1].x, \
            vertex_normals[curr->ind1].y, vertex_normals[curr->ind1].z);
        glVertex3f((*vertices)[curr->ind1]->x, \
            (*vertices)[curr->ind1]->y, (*vertices)[curr->ind1]->z);
        
        glTexCoord2d(0.4, 0.6);
        glNormal3f (vertex_normals[curr->ind2].x, \
            vertex_normals[curr->ind2].y, vertex_normals[curr->ind2].z);
        glVertex3f((*vertices)[curr->ind2]->x, \
            (*vertices)[curr->ind2]->y, (*vertices)[curr->ind2]->z);
        
        glTexCoord2d(0.6, 0.6);
        glNormal3f(vertex_normals[curr->ind3].x, \
            vertex_normals[curr->ind3].y, vertex_normals[curr->ind3].z);
        glVertex3f((*vertices)[curr->ind3]->x, \
            (*vertices)[curr->ind3]->y, (*vertices)[curr->ind3]->z);
        
        glEnd();
        
    }

}

void draw_objects()
{
    float colrs1[3]; float colrs2[3]; float colrs3[3]; 
    colrs1[0]=.5; colrs1[1]=.5; colrs1[2]=.5;
    glMaterialfv(GL_FRONT, GL_AMBIENT, colrs1);

    colrs2[0]=.9; colrs2[1]=.9; colrs2[2]=.9;
    glMaterialfv(GL_FRONT, GL_DIFFUSE, colrs2);

    colrs3[0]=.3; colrs3[1]=.3; colrs3[2]=.3;    
    glMaterialfv(GL_FRONT, GL_SPECULAR, colrs3);

    glMaterialf(GL_FRONT, GL_SHININESS, 8);

    glPushMatrix();

    glRotatef(x_view_angle, 0, 1, 0);
    glRotatef(y_view_angle, 1, 0, 0);
    
    draw_faces();

    glPopMatrix();
}

void set_lights()
{
    int num_lights = lights.size();
    
    for(int i = 0; i < num_lights; ++i)
    {
        int light_id = GL_LIGHT0 + i;
        
        glLightfv(light_id, GL_POSITION, lights[i].position);
    }
}

void mouse_pressed(int button, int state, int x, int y)
{
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        mouse_x = x;
        mouse_y = y;
        is_pressed = true;
    }

    else if(button == GLUT_LEFT_BUTTON && state == GLUT_UP)
    {
        is_pressed = false;
    }
}

void mouse_moved(int x, int y)
{
    if(is_pressed)
    {
        x_view_angle += ((float) x - (float) mouse_x) * \
            mouse_scale_x * x_view_step;
        y_view_angle += ((float) y - (float) mouse_y) * \
            mouse_scale_y * y_view_step;

        mouse_x = x;
        mouse_y = y;
        glutPostRedisplay();
    }
}

float deg2rad(float angle)
{
    return angle * M_PI / 180.0;
}

void key_pressed(unsigned char key, int x, int y)
{
    if(key == 'q')
    {
        exit(0);
    }
}

void create_lights()
{
    ////////////////////////
    // Light 1 Below
    ////////////////////////
    
    Point_Light light1;
    
    light1.position[0] = -1;
    light1.position[1] = 0;
    light1.position[2] = 1;
    light1.position[3] = 1;
    
    light1.color[0] = 1;
    light1.color[1] = 1;
    light1.color[2] = 1;
    light1.attenuation_k = 0;
    
    lights.push_back(light1);
    
}

void build_half_edge()
{
    set_face_normals();
    vector<point*> * points = mesh_data->points;
    vector<face*> * faces = mesh_data->faces;
    int num_points = points->size();
    vector<half_edge::edge_t> edges;
    half_edge::faces_to_edges( faces, edges );
    mesh.build_he_structure( num_points, faces, edges.size(), &edges[0] );
    set_vertex_normals();
}

// Takes in a texture bmp file, and outputs an image
// rendering the bmp texture onto the obj defined in
// obj file.
int main(int argc, char ** argv)
{
    if ( argc != 2 )
    {
        cout << "Invalid number of arguments!" << endl;
        cout << "Usage: " << argv[0] << \
                " [texture.bmp] < [example.obj]" << endl;
        return 1;
    }

    int temp = sprintf(path_to_bmp, "%s", argv[1]);
        
    printf("Parsing...\n");
    parse(cin);
    printf("Finished parsing.\n");

    build_half_edge();

    // use opengl stuff
    int xres = 500;
    int yres = 500;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(xres, yres);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("CS171 Homework 5");
    init();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse_pressed);
    glutMotionFunc(mouse_moved);
    glutKeyboardFunc(key_pressed);
    glutMainLoop();
    return 0;
}
