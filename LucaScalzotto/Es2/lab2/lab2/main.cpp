#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GLUT/glut.h>

#include "v3d.hpp"

#define MAX_VERTICES 10000 // Massimo numero di vertici
#define N_MESH 3 // Numero di mesh
#define N_CV 7 // Numero di punti di controllo

float camCenter[3]; // Centro del sistema
float camEye[3]; // Punto di vista
float camUp[3]; // Vettore up della camera
float lightPosition[4]; // Posizione della luce

float fieldOfView; // Angolo del punto di vista
float angle[3]; // Angoli di rotazione degli assi WCS

float matrix[N_MESH][16] = { {0.64, 0.76, 0.0, 0.0, -0.76, 0.64, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.3, 1.4, 2.2, 1.0},
    {1.0, 0.0, 0.0, 0.0, 0.0, 0.64, 0.76, 0.0, 0.0, -0.76, 0.64, 0.0, 0.5, 1.3, 0.1, 1.0},
    {0.64, 0.0, -0.76, 0.0, 0.0, 1.0, 0.0, 0.0, 0.76, 0.0, 0.64, 0.0, 0.3, 0.4, 0.2, 1.0} };

float matrixWCS[N_MESH][16] = { {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1} };
float matrixOCS[N_MESH][16] = { {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1} };

int nameList[N_MESH]; // Nomi delle mesh per l'invocazione tramite DisplayList
int indexObjectToMove;

int WindowWidth = 700;
int WindowHeight = 700;
float aspect = WindowWidth / WindowHeight; // Width / Height

bool wireframe; // Flag per abilitare/disabilitare la modalità wireframe
bool orthogonal; // Flag per abilitare/disabilitare la visualizzazione in modalità ortografica (default -> prospettica)
bool culling; // Flag per abilitare/disabilitare la modalita' culling
int material; // Seleziona uno dei materiali disponibili
bool shading; // Flag per abilitare/disabilitare la modalità shading

float brass_ambient[] = {0.33, 0.22, 0.03, 1.0},
brass_diffuse[] = {0.78, 0.57, 0.11, 1.0},
brass_specular[] = {0.99, 0.91, 0.81, 1.0},
brass_shininess[] = {27.8};

float red_plastic_ambient[] = {0.0, 0.0, 0.0},
red_plastic_diffuse[] = {0.5, 0.0, 0.0},
red_plastic_specular[] = {0.7, 0.6, 0.6},
red_plastic_shininess[] = {32.0};

float emerald_ambient[] = {0.0215, 0.1745, 0.0215},
emerald_diffuse[] = {0.07568, 0.61424, 0.07568},
emerald_specular[] = {0.633, 0.727811, 0.633},
emerald_shininess[] = {76.8};

float slate_ambient[] = {0.02, 0.02, 0.02},
slate_diffuse[] = {0.02, 0.01, 0.01},
slate_specular[] = {0.4, 0.4, 0.4},
slate_shininess[] = {.78125};

GLUquadricObj * myReusableQuadric = 0;

// Variabili necessarie per il funzionamento della track-ball
float trackBallAngle = 0.0;
float trackBallAxis[3];

int trackBallDragging = 0;
float trackBallV[3];
float trackBallW[3];
// ---

// Variabili necessarie al movimento della camera su una curva di Bézier
int numCV = N_CV; // Numero di punti di controllo

// Punti di controllo della curva di Bèzier
float CV[N_CV][3] = { {0, 20, -10}, {10, 20, -7}, {10, 20, 7}, {0, 20, 10}, {-10, 20, 7}, {-10, 20, -7}, {0, 20, -10} };

int indexMotion = 0; // Indice di movimento sulla curva di Bèzier [0, 100]
bool flagMotion = false; // Flag che abilita/disabilita lo spostamento sulla curva di Bézier
// ---

void deCasteljau(float t, float * result);

enum Modes {
    MODE_INVALID,
    MODE_CHANGE_EYE_POS,
    MODE_CHANGE_REFERENCE_POS,
    MODE_CHANGE_UP_POS,
    MODE_CHANGE_LIGHT_POS,
    MODE_CHANGE_ZOOM,
    MODE_ROTATE_MODEL,
    
    MODE_CHANGE_CULLING,
    MODE_CHANGE_WIREFRAME,
    MODE_CHANGE_PROJECTION,
    MODE_CHANGE_SHADING,
    MODE_CHANGE_MATERIAL,
    
    MODE_TRANSLATE_WCS,
    MODE_ROTATE_WCS,
    MODE_TRANSLATE_OCS,
    MODE_ROTATE_OCS,
    MODE_TRANSLATE_VCS,
    MODE_ROTATE_VCS,
    
    MODE_PRINT_SYSTEM_STATUS,
    MODE_RESET,
    MODE_QUIT
};

enum Modes mode = MODE_INVALID; // Variabile che indica la modalità attuale

void drawAxis(float scale, int drawLetters) {
    glDisable(GL_LIGHTING);
    
    glPushMatrix();
    glScalef(scale, scale, scale);
    
    glBegin(GL_LINES);
    glColor4d(1.0, 0.0, 0.0, 1.0);
    if (drawLetters) {
        glVertex3f(0.8, 0.05, 0.0); glVertex3f(1.0, 0.25, 0.0); // Lettera X
        glVertex3f(0.8, 0.25, 0.0); glVertex3f(1.0, 0.05, 0.0); // Lettera X
    }
    glVertex3f(0.0, 0.0, 0.0); glVertex3f(1.0, 0.0, 0.0); // Asse X
    
    glColor4d(0.0, 1.0, 0.0, 1.0);
    if (drawLetters) {
        glVertex3f(0.10, 0.8, 0.0); glVertex3f(0.10, 0.90, 0.0); // Lettera Y
        glVertex3f(0.10, 0.90, 0.0); glVertex3f(0.05, 1.0, 0.0); // Lettera Y
        glVertex3f(0.10, 0.90, 0.0); glVertex3f(0.15, 1.0, 0.0); // Lettera Y
    }
    glVertex3f(0.0, 0.0, 0.0); glVertex3f(0.0, 1.0, 0.0); // Asse Y
    
    glColor4d(0.0, 0.0, 1.0, 1.0);
    if (drawLetters) {
        glVertex3f(0.05, 0, 0.8); glVertex3f(0.20, 0, 0.8); // Lettera Z
        glVertex3f(0.20, 0, 0.8); glVertex3f(0.05, 0, 1.0); // Lettera Z
        glVertex3f(0.05, 0, 1.0); glVertex3f(0.20, 0, 1.0); // Lettera Z
    }
    glVertex3f(0.0, 0.0, 0.0); glVertex3f(0.0, 0.0, 1.0); // Asse Z
    
    glEnd();
    
    glPopMatrix();
    
    glEnable(GL_LIGHTING);
}

void computePointOnTrackball(int x, int y, float point[3]) {
    float zTemp;
    
    point[0] = (2.0 * x - WindowWidth) / WindowWidth;
    point[1] = (WindowHeight - 2.0f * y) / WindowHeight;
    
    zTemp = 1.0 - (point[0] * point[0]) - (point[1] * point[1]);
    point[2] = (zTemp > 0.0 ) ? sqrt(zTemp) : 0.0;
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        trackBallDragging = true;
        computePointOnTrackball(x, y, trackBallV);
    }
    if ( button == GLUT_LEFT_BUTTON && state == GLUT_UP )
        trackBallDragging = false;
}

void motion(int x, int y) {
    if (trackBallDragging == true) {
        computePointOnTrackball(x, y, trackBallW);
        
        float dx = trackBallV[0] - trackBallW[0];
        float dy = trackBallV[1] - trackBallW[1];
        float dz = trackBallV[2] - trackBallW[2];
        
        if (dx || dy || dz) {
            trackBallAngle += sqrt(dx * dx + dy * dy + dz * dz) * (180.0 / M_PI);
            
            trackBallAxis[0] += trackBallW[1] * trackBallV[2] - trackBallW[2] * trackBallV[1];
            trackBallAxis[1] += trackBallW[2] * trackBallV[0] - trackBallW[0] * trackBallV[2];
            trackBallAxis[2] += trackBallW[0] * trackBallV[1] - trackBallW[1] * trackBallV[0];
            
            trackBallV[0] = trackBallW[0];
            trackBallV[1] = trackBallW[1];
            trackBallV[2] = trackBallW[2];
        }

        glutPostRedisplay();
    }
}

void passiveMotion(int x, int y) { }

void special(int key, int x, int y) { }

void toMove(float * arrayToModify, float step, unsigned char key) {
    if ( key == 'x' )
        arrayToModify[0] += step;
    if ( key == 'X' )
        arrayToModify[0] -= step;
    if ( key == 'y' )
        arrayToModify[1] += step;
    if ( key == 'Y' )
        arrayToModify[1] -= step;
    if ( key == 'z' )
        arrayToModify[2] += step;
    if ( key == 'Z' )
        arrayToModify[2] -= step;
}

void toZoom(float * toModify, float step, unsigned char key) {
    if ( key == 'f' )
        (* toModify) += step;
    if ( key == 'F' )
        (* toModify) -= step;
}

void toModifyMatrixTranslation(float matrix[16], float step, unsigned char key) {
    glPushMatrix();
    glLoadIdentity();
    
    if ( key == 'x' )
        glTranslatef(step, 0, 0);
    if ( key == 'X' )
        glTranslatef(-step, 0, 0);
    if ( key == 'y' )
        glTranslatef(0, step, 0);
    if ( key == 'Y' )
        glTranslatef(0, -step, 0);
    if ( key == 'z' )
        glTranslatef(0, 0, step);
    if ( key == 'Z' )
        glTranslatef(0, 0, -step);
    
    glMultMatrixf(matrix);
    
    glGetFloatv(GL_MODELVIEW, matrix);
    
    glPopMatrix();
}

void toModifyMatrixRotation(float matrix[16], float step, unsigned char key) {
    glPushMatrix();
    glLoadIdentity();
    
    if ( key == 'x' )
        glRotatef(step, 1.0, 0.0, 0.0);
    if ( key == 'X' )
        glRotatef(-step, 1.0, 0.0, 0.0);
    if ( key == 'y' )
        glRotatef(step, 0.0, 1.0, 0.0);
    if ( key == 'Y' )
        glRotatef(-step, 0.0, 1.0, 0.0);
    if ( key == 'z' )
        glRotatef(step, 0.0, 0.0, 1.0);
    if ( key == 'Z' )
        glRotatef(-step, 0.0, 0.0, 1.0);
    
    glMultMatrixf(matrix);
    
    glGetFloatv(GL_MODELVIEW, matrix);
    
    glPopMatrix();
}

void keyboard(unsigned char key, int x, int y) {
    if ( mode == MODE_CHANGE_EYE_POS )
        toMove(camEye, 0.5, key);
    if ( mode == MODE_CHANGE_REFERENCE_POS )
        toMove(camCenter, 0.5, key);
    if ( mode == MODE_CHANGE_UP_POS )
        toMove(camUp, 0.5, key);
    if ( mode == MODE_CHANGE_LIGHT_POS )
        toMove(lightPosition, 0.5, key);
    if ( mode == MODE_CHANGE_ZOOM )
        toZoom(&fieldOfView, 5.0, key);
    if ( mode == MODE_ROTATE_MODEL )
        toMove(angle, 1.0, key);
    
    if (key == 's') {
        flagMotion = !flagMotion;
        if ( flagMotion == true )
            indexMotion = 0;
    }
    
    if ( key == '1' )
        indexObjectToMove = 0;
    if ( key == '2' )
        indexObjectToMove = 1;
    if ( key == '3' )
        indexObjectToMove = 2;
    
    if ( key == 'w' )
        mode = MODE_TRANSLATE_WCS;
    if ( key == 'W' )
        mode = MODE_ROTATE_WCS;
    if ( key == 'o' )
        mode = MODE_TRANSLATE_OCS;
    if ( key == 'O' )
        mode = MODE_ROTATE_OCS;
    
    if ( mode == MODE_TRANSLATE_WCS )
        toModifyMatrixTranslation(matrixWCS[indexObjectToMove], 1.0, key);
    if ( mode == MODE_ROTATE_WCS )
        toModifyMatrixRotation(matrixWCS[indexObjectToMove], 5.0, key);
    if ( mode == MODE_TRANSLATE_OCS )
        toModifyMatrixTranslation(matrixOCS[indexObjectToMove], 1.0, key);
    if ( mode == MODE_ROTATE_OCS )
        toModifyMatrixRotation(matrixOCS[indexObjectToMove], 5.0, key);
    
    if ( key == 27 )
        exit(0);
    
    glutPostRedisplay();
}

void idle() {
    if (flagMotion == true) {
        float t = (float) indexMotion / 100;
        float result[3];
        
        deCasteljau(t, &result[0]);
        camEye[0] = result[0];
        camEye[1] = result[1];
        camEye[2] = result[2];
        
        indexMotion++;
        if ( indexMotion == 100 )
            indexMotion = 0;
        
        glutPostRedisplay();
    }
}

void display() {
    if (material == 0) { // Ottone
        glLightfv(GL_LIGHT0, GL_AMBIENT, brass_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, brass_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, brass_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, brass_shininess);
    }
    if (material == 1) { // Plastica rossa
        glLightfv(GL_LIGHT0, GL_AMBIENT, red_plastic_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, red_plastic_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, red_plastic_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, red_plastic_shininess);
    }
    if (material == 2) { // Smeraldo
        glLightfv(GL_LIGHT0, GL_AMBIENT, emerald_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, emerald_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, emerald_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, emerald_shininess);
    }
    if (material==3) { // Slate
        glLightfv(GL_LIGHT0, GL_AMBIENT, slate_ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, slate_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, slate_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, slate_shininess);
    }
    
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    
    /* TODO:
     * Punto 1
     */
    
    if ( wireframe == true )
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else if ( wireframe == false )
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    if ( shading == true )
        glShadeModel(GL_SMOOTH);
    else if ( shading == false )
        glShadeModel(GL_FLAT);
    
    // culling... (punto 2)
    
    if (culling == true) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else if ( culling == false )
        glDisable(GL_CULL_FACE);
    
    // ---
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    if ( orthogonal == false )
        gluPerspective(fieldOfView, aspect, 1, 100);
    else if ( orthogonal == true )
        glOrtho(-2.0, 2.0, -2.0, 2.0, -100, 100);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    gluLookAt(camEye[0], camEye[1], camEye[2], camCenter[0], camCenter[1], camCenter[2], camUp[0], camUp[1], camUp[2]);
    
    // Rotazione della track-ball
    glRotatef(trackBallAngle, trackBallAxis[0], trackBallAxis[1], trackBallAxis[2]);
    
    glLineWidth(2);
    drawAxis(2.0, 1);
    glLineWidth(1);
    
    glRotatef(angle[0], 1.0, 0.0, 0.0); // Non utilizzato nel punto 3
    glRotatef(angle[1], 0.0, 1.0, 0.0); // Non utilizzato nel punto 3
    glRotatef(angle[2], 0.0, 0.0, 1.0); // Non utilizzato nel punto 3
    
    for (int i = 0; i < N_MESH; i++) {
        glPushMatrix();
        
        glMultMatrixf(matrixWCS[i]);
        glMultMatrixf(matrix[i]);
        drawAxis(1.0, 1); // Assi OCS dell'i-esima mesh
        glMultMatrixf(matrixOCS[i]);
        
        glCallList(nameList[i]);
        
        glPopMatrix();
    }
    
    // Disegno di una sfera
    // gluSphere(myReusableQuadric, 1.0, 12, 12);
    
    // glutWireIcosahedron();
    // glutWireDodecahedron();
    // glutWireTeapot(1.0);
    // glutWireTorus(0.5, 1.0, 50, 100);
    // glutWireCone(1.0, 1.0, 10, 10);
    
    glTranslated(0.0, 0.0, 1.5);
    gluCylinder(myReusableQuadric, 0.5, 0.2, 0.5, 12, 12);
    
    glutSwapBuffers();
}

void reset() {
    angle[0] = 0.0;
    angle[1] = 0.0;
    angle[2] = 0.0;
    
    camEye[0] = 8.8;
    camEye[1] = 4.9;
    camEye[2] = 9.0;
    
    camCenter[0] = 0.0;
    camCenter[1] = 0.0;
    camCenter[2] = 0.0;
    
    camUp[0] = 0.0;
    camUp[1] = 1.0;
    camUp[2] = 0.0;
    
    lightPosition[0] =  5.0;
    lightPosition[1] =  5.0;
    lightPosition[2] =  5.0;
    lightPosition[3] =  1.0;
    
    for (int i = 0; i < N_MESH; i++) {
        glPushMatrix();
        glLoadIdentity();
        glGetFloatv(GL_MODELVIEW, matrixWCS[i]);
        glGetFloatv(GL_MODELVIEW, matrixOCS[i]);
        glPopMatrix();
    }
    
    trackBallAngle = 0;
    trackBallAxis[0] = 0;
    trackBallAxis[1] = 0;
    trackBallAxis[2] = 0;
    
    fieldOfView = 20;
    wireframe = true;
    culling = false;
    material = 1;
    orthogonal = false;
    shading = true;
    
    glutPositionWindow(10,10);
    glutReshapeWindow(700,700);
    glutPostRedisplay();
}

void loadMesh(char path[256], int faces[MAX_VERTICES][3], int * nFaces, float vertices[MAX_VERTICES][3], int * nVertices,
              float vNormals[MAX_VERTICES][3], float fNormals[MAX_VERTICES][3]) {
    
    for (int i = 0 ; i < MAX_VERTICES; i++) {
        for (int j = 0; j < 3; j++) {
            faces[i][j] = 0.0f;
            vertices[i][j] = 0.0f;
            vNormals[i][j] = 0.0f;
            fNormals[i][j] = 0.0f;
        }
    }
    
    (* nVertices) = 0;
    (* nFaces) = 0;
    
    FILE * file;
    
    printf("Apertura del file '%s'... \n", path);
    
    if ( (file = fopen(path, "r")) == NULL) {
        perror("File non trovato. \n");
        exit(1);
    }
    
    printf("File aperto con successo. \n");
    
    char type[16];
    int id = 0;
    float a,b,c;
    
    float * vertex;
    int * face;
    
    while (!feof(file)) {
        fscanf(file, "%s %d %f %f %f", type, &id, &a, &b, &c);
        switch (type[0]) {
            case ('V'):
                vertex = vertices[ (* nVertices) ];
                
                vertex[0] = a;
                vertex[1] = b;
                vertex[2] = c;
                
                (* nVertices)++;
                
                break;
            case ('N'):
                printf("Vertex normals not used! \n");
                break;
            case ('F'):
                face = faces[ (* nFaces) ];
                
                face[0] = (int) a - 1;
                face[1] = (int) b - 1;
                face[2] = (int) c - 1;
                
                /*
                 * Punto 1: calcolo normali alle facce
                 */
                
                float v1[3];
                float v2[3];
                
                float normal[3];
                
                v3dSub(vertices[face[1]], vertices[face[0]], v1);
                v3dSub(vertices[face[2]], vertices[face[0]], v2);
                v3dCross(v1, v2, normal);
                v3dNormalize(normal);
                
                fNormals[ (* nFaces) ][0] = normal[0];
                fNormals[ (* nFaces) ][1] = normal[1];
                fNormals[ (* nFaces) ][2] = normal[2];
                
                (* nFaces)++;
                
                /*
                 * Punto 1: Calcolo normale al vertice
                 */
                
                vNormals[face[0]][0] += normal[0];
                vNormals[face[0]][1] += normal[1];
                vNormals[face[0]][2] += normal[2];
                
                vNormals[face[1]][0] += normal[0];
                vNormals[face[1]][1] += normal[1];
                vNormals[face[1]][2] += normal[2];
                
                vNormals[face[2]][0] += normal[0];
                vNormals[face[2]][1] += normal[1];
                vNormals[face[2]][2] += normal[2];
                
                break;
            case ('E'):
                printf("Edge not used! \n");
                break;
            default:
                printf("Error! \n");
                break;
        }
    }
    
    for ( int i = 0; i < (* nFaces); i++ )
        v3dNormalize(fNormals[i]);
    
    for ( int i = 0; i < (* nVertices); i++ )
        v3dNormalize(vNormals[i]);
    
    int rows = (* nVertices) + (* nFaces);
    
    printf("Chiusura del file... \n");
    fclose(file);
    printf("File chiuso correttamente. \n");
    
    printf("Vertici: %d, facce %d -> %d.\n", (* nVertices), (* nFaces), rows);
}

void init() {
    char path[256];
    strcpy(path, "/Users/luca.scalzotto/Desktop/Es2/lab2/data/pig.m");
    
    char files[N_MESH][256];
    strcpy(files[0], path);
    strcpy(files[1], path);
    strcpy(files[2], path);
    
    for (int i = 0; i < N_MESH; i++) {
        int faces[MAX_VERTICES][3];
        int nFaces;
        float vertices[MAX_VERTICES][3];
        int nVertices;
        float vNormals[MAX_VERTICES][3];
        float fNormals[MAX_VERTICES][3];
        
        loadMesh(files[i], faces, &nFaces, vertices, &nVertices, vNormals, fNormals);
        
        // Display list mesh
        nameList[i] = glGenLists(1);
        glNewList(nameList[i], GL_COMPILE);

        int ids[3];
        
        for (int i = 0; i < nFaces; i++) {
            ids[2] = faces[i][0];
            ids[1] = faces[i][1];
            ids[0] = faces[i][2];
            
            // Disegna triangoli coi vertici specificati
            glBegin(GL_TRIANGLES);
            glColor3f(1, 0, 0);
            for (int ii = 2; ii >= 0; ii--)
            {
                glNormal3f( vNormals[ids[ii]][0], vNormals[ids[ii]][1], vNormals[ids[ii]][2]);
                glVertex3f( vertices[ids[ii]][0], vertices[ids[ii]][1], vertices[ids[ii]][2]);
            }
            glEnd();
        }
        
        glEndList();
    }
    
    myReusableQuadric = gluNewQuadric();
    gluQuadricNormals(myReusableQuadric, GLU_SMOOTH);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    
    glClearColor(1.0, 1.0, 1.0, 1.0);
}

void print_sys_status() {
    system("clear");
    printf("\nSystem status\n");
    printf("---------------------------------------------------\n");
    printf("Eye point coordinates :\n");
    printf("x = %.2f, y = %.2f, z = %.2f\n", camEye[0], camEye[1], camEye[2]);
    printf("---------------------------------------------------\n");
    printf("Reference point coordinates :\n");
    printf("x = %.2f, y = %.2f, z = %.2f\n", camCenter[0], camCenter[1], camCenter[2]);
    printf("---------------------------------------------------\n");
    printf("Up vector coordinates :\n");
    printf("x = %.2f, y = %.2f, z = %.2f\n", camUp[0], camUp[1], camUp[2]);
    printf("---------------------------------------------------\n");
    printf("Light position coordinates :\n");
    printf("x = %.2f, y = %.2f, z = %.2f\n", lightPosition[0], lightPosition[1], lightPosition[2]);
    printf("---------------------------------------------------\n");
    
//    printf("Axis rotation (in degree) :\n");
//    printf("x = %.2f, y = %.2f, z = %.2f\n",angle[0], angle[1], angle[2] );
//    printf("---------------------------------------------------\n");
    
    printf("Field of view angle (in degree) = %.2f\n", fieldOfView);
    printf("---------------------------------------------------\n");
    if ( wireframe )
        printf("Wireframe = YES\n");
    else
        printf("Wireframe = NO\n");
    printf("---------------------------------------------------\n");
    if ( culling )
        printf("Culling = YES\n");
    else
        printf("Culling = NO\n");
    printf("---------------------------------------------------\n");
    printf("Material = %d\n", material);
    printf("---------------------------------------------------\n");
    if ( orthogonal )
        printf("Orthogonal = YES\n");
    else
        printf("Orthogonal = NO\n");
    printf("---------------------------------------------------\n");
    if ( shading )
        printf("Shading = YES\n");
    else
        printf("Shading = NO\n");
    printf("---------------------------------------------------\n");
}

void menu(int sel) {
    if ( sel == MODE_CHANGE_EYE_POS ||
            sel == MODE_CHANGE_REFERENCE_POS ||
            sel == MODE_CHANGE_UP_POS ||
            sel == MODE_CHANGE_LIGHT_POS ||
            sel == MODE_CHANGE_ZOOM ||
            sel == MODE_ROTATE_MODEL )
        mode = (Modes) sel;
    
    if ( sel == MODE_CHANGE_CULLING )
        culling = !culling;

    if ( sel == MODE_CHANGE_WIREFRAME )
        wireframe = !wireframe;

    if ( sel == MODE_CHANGE_PROJECTION )
        orthogonal = !orthogonal;
    
    if ( sel == MODE_CHANGE_SHADING )
        shading = !shading;

    if ( sel == MODE_CHANGE_MATERIAL )
        material = (material + 1) % 4;
    
    if ( sel == MODE_RESET )
        reset();
    
    if ( sel == MODE_QUIT )
        exit(0);
    
    if ( sel == MODE_PRINT_SYSTEM_STATUS )
        print_sys_status();
    
    glutPostRedisplay();
}

void deCasteljau(float t, float * result){
    float coordX[numCV], coordY[numCV], coordZ[numCV];
    
    for(int i = 0; i < numCV; i++){
        coordX[i] = CV[i][0];
        coordY[i] = CV[i][1];
        coordZ[i] = CV[i][2];
    }
    
    for(int i = 1; i < numCV; i++){
        for(int j = 0; j < numCV - i; j++){
            coordX[j] = (1-t) * coordX[j] + (t) * coordX[j+1];
            coordY[j] = (1-t) * coordY[j] + (t) * coordY[j+1];
            coordZ[j] = (1-t) * coordZ[j] + (t) * coordZ[j+1];
        }
    }
    
    result[0] = coordX[0];
    result[1] = coordY[0];
    result[2] = coordZ[0];
}

int main(int argc, char ** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(WindowWidth, WindowHeight);
    glutInitWindowPosition(10, 10);
    glutCreateWindow("Model Viewer");

    glutDisplayFunc(display);
    glutSpecialFunc(special);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(passiveMotion);
    glutIdleFunc(idle);
    
    glutCreateMenu(menu);
    glutAddMenuEntry("Menu", -1);
    glutAddMenuEntry("", -1);
    glutAddMenuEntry("Change eye point (use x,y,z,X,Y,Z)", MODE_CHANGE_EYE_POS);
    glutAddMenuEntry("Change reference point (use x,y,z,X,Y,Z)", MODE_CHANGE_REFERENCE_POS);
    glutAddMenuEntry("Change up vector (use x,y,z,X,Y,Z)", MODE_CHANGE_UP_POS);
    glutAddMenuEntry("Change light position (use x,y,z,X,Y,Z)", MODE_CHANGE_LIGHT_POS);
    glutAddMenuEntry("Zoom (use f,F)", MODE_CHANGE_ZOOM);
    // glutAddMenuEntry("Rotate Object (use x,y,z,X,Y,Z)", MODE_ROTATE_MODEL );
    
    glutAddMenuEntry("", -1);
    glutAddMenuEntry("Culling", MODE_CHANGE_CULLING);
    glutAddMenuEntry("Wireframe", MODE_CHANGE_WIREFRAME);
    glutAddMenuEntry("Ortographic or Perspective", MODE_CHANGE_PROJECTION);
    glutAddMenuEntry("Shading", MODE_CHANGE_SHADING);
    glutAddMenuEntry("Material", MODE_CHANGE_MATERIAL);
    
    glutAddMenuEntry("", -1);
    glutAddMenuEntry("Print system status", MODE_PRINT_SYSTEM_STATUS);
    glutAddMenuEntry("Reset", MODE_RESET);
    glutAddMenuEntry("Quit", MODE_QUIT);
    
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    reset();
    init();
    
    glutMainLoop();
    
    return (0);
}
