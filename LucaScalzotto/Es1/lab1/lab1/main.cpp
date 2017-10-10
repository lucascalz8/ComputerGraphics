/*
 * main.cpp
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GLUT/glut.h>

#define MAX_CV 64
#define ITERATIONS 100
#define MAX_DISTANCE 0.01

#define NORMAL 0
#define ADAPTIVE 1

// Global variables
float CV[MAX_CV][3];
int numCV = 0;

bool toMove = false;
int pointToMoveIndex = -1;

int type = NORMAL;
// ---

// Window size in pixels
int WindowHeight;
int WindowWidth;
// ---

// Prototypes
void addNewPoint(float x, float y, float z);
void removeFirstPoint();
void removeLastPoint();

void normal(float controlPoints[MAX_CV][3], int n, float pointsToDraw[1024][3], int * nToDraw);
void adaptive(float controlPoints[MAX_CV][3], int n, float pointsToDraw[1024][3], int * nToDraw);

void deCasteljauAlgorithm(float controlPoints[MAX_CV][3], int n, float t, float pointToDraw[3]);
void deCasteljauAlgorithmAdaptive(float controlPoints[MAX_CV][3], int n, float t, float firstHalf[MAX_CV][3], float secondHalf[MAX_CV][3]);
bool flatTest(float firstX, float firstY, float lastX, float lastY, float x, float y);
// ---

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case ('f'):
            removeFirstPoint();
            glutPostRedisplay();
            break;
        case ('l'):
            removeLastPoint();
            glutPostRedisplay();
            break;
        case ('1'):
            type = NORMAL;
            glutPostRedisplay();
            break;
        case ('2'):
            type = ADAPTIVE;
            glutPostRedisplay();
            break;
        case (27): // Escape key
            exit(0);
            break;
    }
}

void evaluatePositions(int x, int y, float * xPos, float * yPos, float * zPos) {
    (* xPos) = ((float) x) / ((float) (WindowWidth - 1));
    (* yPos) = ((float) y) / ((float) (WindowHeight - 1));
    (* yPos) = 1.0f - (* yPos); // Flip value since y position is from top row.
    (* zPos) = 0;
}

void mouse(int button, int state, int x, int y) {
    float xPos = -1; float yPos = -1; float zPos = -1;
    evaluatePositions(x, y, &xPos, &yPos, &zPos);
    
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        addNewPoint(xPos, yPos, zPos);
        glutPostRedisplay();
    }
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        for (int i = 0; i < numCV; i++) {
            if ( (CV[i][0] <= xPos + 0.05 && CV[i][0] >= xPos - 0.05) && (CV[i][1] <= yPos + 0.05 && CV[i][1] >= yPos - 0.05)) {
                toMove = true;
                pointToMoveIndex = i;
            }
        }
    }
    
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        toMove = false;
        pointToMoveIndex = -1;
    }
}

void motion(int x, int y) {
    float xPos = -1; float yPos = -1; float zPos = -1;
    evaluatePositions(x, y, &xPos, &yPos, &zPos);
    
    if (toMove == true) {
        CV[pointToMoveIndex][0] = xPos;
        CV[pointToMoveIndex][1] = yPos;
        CV[pointToMoveIndex][2] = zPos;
        
        glutPostRedisplay();
    }
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_POINTS);
    glColor3f(0.0f, 0.0f, 0.0f);
    for ( int i = 0; i < numCV; i++ )
        glVertex3f(CV[i][0], CV[i][1], CV[i][2]);
    glEnd();
    
    if (numCV > 1) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);
        glColor3f(1.0f, 0.0f, 0.8f);
        glBegin(GL_LINE_STRIP);
        for ( int i = 0; i < numCV; i++ )
            glVertex3f(CV[i][0], CV[i][1], CV[i][2]);
        glEnd();
        glDisable(GL_LINE_STIPPLE);
        
        if (type == NORMAL) {
            // glEnable(GL_MAP1_VERTEX_3);
            // glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, numCV, &CV[0][0]);
            // glBegin(GL_LINE_STRIP);
            // glColor3f(0.0f, 0.0f, 0.0f);
            // for ( int i = 0; i <= ITERATIONS; i++ ) {
                // float t = (float) i / ITERATIONS;
                // glEvalCoord1f(t);
            // }
            // glEnd();
            
            float pointsToDraw[1024][3];
            int nToDraw = 0;
            normal(CV, numCV, pointsToDraw, &nToDraw);
            glBegin(GL_LINE_STRIP);
            glColor3f(0.0f, 0.0f, 0.0f);
            for ( int i = 0; i < nToDraw; i++ )
                glVertex3f(pointsToDraw[i][0], pointsToDraw[i][1], pointsToDraw[i][2]);
            glEnd();
        } else if ( type == ADAPTIVE ) {
            float pointsToDraw[1024][3];
            int nToDraw = 0;
            adaptive(CV, numCV, pointsToDraw, &nToDraw);
            glBegin(GL_LINE_STRIP);
            glColor3f(0.0f, 0.0f, 0.0f);
            for ( int i = 0; i < nToDraw; i++ )
                glVertex3f(pointsToDraw[i][0], pointsToDraw[i][1], pointsToDraw[i][2]);
            glEnd();
        }
        glEnd();
    }
    
    glFlush();
}

void reshape(int w, int h) {
    WindowHeight = (h > 1) ? h : 2;
    WindowWidth = (w > 1) ? w : 2;
    glViewport (0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluOrtho2D (0.0f, 1.0f, 0.0f, 1.0f); // Always view [0,1]x[0,1].
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
}

void addNewPoint(float x, float y, float z) {
    if ( numCV >= MAX_CV )
        removeFirstPoint ();
    
    CV[numCV][0] = x;
    CV[numCV][1] = y;
    CV[numCV][2] = z;
    
    numCV++;
}

void removeFirstPoint() {
    if (numCV > 0) {
        for (int i = 0; i < (numCV - 1); i++) {
            CV[i][0] = CV[i + 1][0];
            CV[i][1] = CV[i + 1][1];
            CV[i][2] = CV[i + 1][2];
        }
        numCV--;
    }
}

void removeLastPoint() {
    if ( numCV > 0 )
        numCV--;
}

void normal(float controlPoints[MAX_CV][3], int n, float pointsToDraw[1024][3], int * nToDraw) {
    for (int i = 0; i <= ITERATIONS; i++) {
        float t = (float) i / ITERATIONS;
        float pointToDraw[3];
        deCasteljauAlgorithm(controlPoints, n, t, pointToDraw);
        
        pointsToDraw[ (* nToDraw) ][0] = pointToDraw[0];
        pointsToDraw[ (* nToDraw) ][1] = pointToDraw[1];
        pointsToDraw[ (* nToDraw) ][2] = pointToDraw[2];
        
        (* nToDraw) = (* nToDraw) + 1;
    }
}

void adaptive(float controlPoints[MAX_CV][3], int n, float pointsToDraw[1024][3], int * nToDraw) {
    float firstX = controlPoints[0][0];
    float firstY = controlPoints[0][1];
    float firstZ = controlPoints[0][2];
    
    float lastX = controlPoints[n - 1][0];
    float lastY = controlPoints[n - 1][1];
    float lastZ = controlPoints[n - 1][2];
    
    bool result = true;
    for (int i = 1; i < (n - 1); i++) {
        bool currentFlatTest = flatTest(firstX, firstY, lastX, lastY, controlPoints[i][0], controlPoints[i][1]);
        result = result && currentFlatTest;
    }
    
    if (result == true) {
        pointsToDraw[ (* nToDraw) ][0] = firstX;
        pointsToDraw[ (* nToDraw) ][1] = firstY;
        pointsToDraw[ (* nToDraw) ][2] = firstZ;
        
        (* nToDraw) = (*nToDraw) + 1;
        
        pointsToDraw[ (* nToDraw) ][0] = lastX;
        pointsToDraw[ (* nToDraw) ][1] = lastY;
        pointsToDraw[ (* nToDraw) ][2] = lastZ;
        
        (* nToDraw) = (*nToDraw) + 1;
    } else {
        float controlPoints1[MAX_CV][3];
        float controlPoints2[MAX_CV][3];
        
        deCasteljauAlgorithmAdaptive(controlPoints, n, 0.5, controlPoints1, controlPoints2);
        
        float reversedControlPoints2[MAX_CV][3];
        int currentIndex = 0;
        for (int i = n - 1; i >= 0; i--) {
            reversedControlPoints2[currentIndex][0] = controlPoints2[i][0];
            reversedControlPoints2[currentIndex][1] = controlPoints2[i][1];
            reversedControlPoints2[currentIndex][2] = controlPoints2[i][2];
            
            currentIndex++;
        }
        
        adaptive(controlPoints1, n, pointsToDraw, nToDraw);
        adaptive(reversedControlPoints2, n, pointsToDraw, nToDraw);
    }

}

void deCasteljauAlgorithm(float controlPoints[MAX_CV][3], int n, float t, float pointToDraw[3]) {
    float coordX[MAX_CV];
    float coordY[MAX_CV];
    float coordZ[MAX_CV];
    
    for (int i = 0; i < n; i++) {
        coordX[i] = controlPoints[i][0];
        coordY[i] = controlPoints[i][1];
        coordZ[i] = controlPoints[i][2];
    }

    for (int i = 0; i < (n - 1); i++) {
        for (int j = 0; j < (n - i - 1); j++) {
            coordX[j] = (1 - t) * coordX[j] + t * coordX[j + 1];
            coordY[j] = (1 - t) * coordY[j] + t * coordY[j + 1];
            coordZ[j] = (1 - t) * coordZ[j] + t * coordZ[j + 1];
        }
    }
    
    pointToDraw[0] = coordX[0];
    pointToDraw[1] = coordY[0];
    pointToDraw[2] = coordZ[0];
}

void deCasteljauAlgorithmAdaptive(float controlPoints[MAX_CV][3], int n, float t, float firstHalf[MAX_CV][3], float secondHalf[MAX_CV][3]) {
    float coordX[MAX_CV];
    float coordY[MAX_CV];
    float coordZ[MAX_CV];
    
    for (int i = 0; i < n; i++) {
        coordX[i] = controlPoints[i][0];
        coordY[i] = controlPoints[i][1];
        coordZ[i] = controlPoints[i][2];
    }
    
    firstHalf[0][0] = coordX[0];
    firstHalf[0][1] = coordY[0];
    firstHalf[0][2] = coordZ[0];
    
    secondHalf[0][0] = coordX[n - 1];
    secondHalf[0][1] = coordY[n - 1];
    secondHalf[0][2] = coordZ[n - 1];
    
    for (int i = 0; i < (n - 1); i++) {
        for (int j = 0; j < (n - i - 1); j++) {
            coordX[j] = (1 - t) * coordX[j] + t * coordX[j + 1];
            coordY[j] = (1 - t) * coordY[j] + t * coordY[j + 1];
            coordZ[j] = (1 - t) * coordZ[j] + t * coordZ[j + 1];
        }
        
        firstHalf[i + 1][0] = coordX[0];
        firstHalf[i + 1][1] = coordY[0];
        firstHalf[i + 1][2] = coordZ[0];
        
        secondHalf[i + 1][0] = coordX[n - i - 2];
        secondHalf[i + 1][1] = coordY[n - i - 2];
        secondHalf[i + 1][2] = coordZ[n - i - 2];
    }
}

bool flatTest(float firstX, float firstY, float lastX, float lastY, float x, float y) {
    float a = -(lastY - firstY);
    float b = (lastX - firstX);
    float c = (firstX * (lastY - firstY)) - (lastY * (lastX - firstX));
    // ax + by + c = 0
    
    // d = |ax + by + c| / radiceQuadrata(a^2 + b^2)
    float d = ( fabsf(a * x + b * y + c) / sqrtf( powf(a, 2) + powf(b, 2) ) );
    
    return (d < MAX_DISTANCE);
}

void initRendering() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Make big points and wide lines. (This may be commented out if desired.)
    glPointSize(5);
    glLineWidth(1);
    
    // The following commands should induce OpenGL to create round points and
    // antialias points and lines.  (This is implementation dependent unfortunately, and
    // may slow down rendering considerably.)
    // You may comment these out if you wish.
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST); // Make round points, not square points
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST); // Antialias the lines
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(600, 400);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);
    initRendering();
    
    // Callback functions
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    
    glutMainLoop(); // Event loop
    return (0); // This line is never reached
}
