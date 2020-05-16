#include "glSetup.h"
#include "hsv2rgb.h"

#include <Eigen/Dense>
#include <Eigen/Geometry>
using namespace Eigen;

#include <iostream>
#include <vector>
#include <math.h>
using namespace std;

void construct(int repetition);
void finalize();

void render(GLFWwindow* window);
void reshape(GLFWwindow* window, int w, int h);
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse(GLFWwindow* window, int button, int action, int mods);

void convertCoordinates(double* xPos, double *yPos);

// Camera configuration
Vector3f eye(0, 0, 9);
Vector3f center(0, 0, 0);
Vector3f up(0, 1, 0);

// Global coordinate frame
double AXIS_LENGTH = 3;
double AXIS_LINE_WIDTH = 2;

// Colors
GLfloat bgColor[4] = { 1, 1, 1, 1 };

// Mouse
double xPos, yPos;

// Drag
int  dragPoint; 
bool isDragging = false;

// Mode
char mode;

// # of endpoint repetitions
int repetition = 0;

// (x, y, z) positions of the data
const int        N = 12;  // Here, N is the number of points
vector<Vector3f> p;

int       nControlPoints = 0;
Vector3f* controlPoints = NULL;

int main(int argc, char* argv[])
{
    if (argc >= 2)  repetition = atoi(argv[1]);

    // Initialize the OpenGL system
    GLFWwindow* window = initializeOpenGL(argc, argv, bgColor);
    if (window == NULL) return -1;

    // Callbacks
    glfwSetKeyCallback(window, keyboard);
    glfwSetMouseButtonCallback(window, mouse);

    // Depth test
    glDisable(GL_DEPTH_TEST);

    // Normal vectors are normalized after transformation.
    glEnable(GL_NORMALIZE);

    // Back face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Viewport and perspective setting
    reshape(window, windowW, windowH);

    // Initialization - Main loop - Finalization
    //
    cout << "Maximum of control points: " << N << endl;
    construct(0);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        render(window);             // Draw one frame
        glfwSwapBuffers(window);    // Swap buffers
        glfwPollEvents();           // Events

        if (isDragging)            // Drag
        {
            glfwGetCursorPos(window, &xPos, &yPos);
            convertCoordinates(&xPos, &yPos);
            p[dragPoint][0] = xPos;
            p[dragPoint][1] = yPos;
            construct(repetition);
        }
    }

    // Finalization
    finalize();

    // Terminate the glfw system
    glfwDestroyWindow(window);  // Is it required?
    glfwTerminate();

    return 0;
}

void construct(int r)
{
    int repetition = r; // 2 for endpoint interpolation

    nControlPoints = p.size() + 2*repetition;
    controlPoints = new Vector3f[nControlPoints];

    for (int i = 0; i < p.size(); i++)
        controlPoints[i+repetition] = Vector3f(p[i][0], p[i][1], p[i][2]);
    
    for (int i = 0; i < repetition; i++)
    {
        controlPoints[i] = controlPoints[repetition];                   // From beginning
        controlPoints[p.size()+repetition+i] = controlPoints[p.size()+repetition-1];  // Before ending
    }
}

void finalize()
{
    delete [] controlPoints;
}

Vector3f BsplinePoint(Vector3f b[4], float t1)
{
    float t2 = t1 * t1;
    float t3 = t2 * t1;

    float B0 = 1 - 3*t1 + 3*t2 - t3;
    float B1 = 4 - 6*t2 + 3*t3;
    float B2 = 1 + 3*t1 + 3*t2 - 3*t3;
    float B3 = t3;

    return (b[0]*B0 + b[1]*B1 + b[2]*B2 + b[3]*B3) / 6;
}

// Draw the B-spline
void drawBSpline()
{
    int N_POINTS_PER_SEGMENTS = 40;

    // Curve
    glLineWidth(3);

    // Colors
    float hsv[3] = {0, 1, 1};
    float rgb[3];

    Vector3f b[4];
    for (int i = 0; i < nControlPoints-3; i++)
    {
        hsv[0] = 360.0 * i/(nControlPoints-3);  // Degree
        HSV2RGB(hsv, rgb);
        glColor3f(rgb[0], rgb[1], rgb[2]);

        for (int j = 0; j < 4; j++)
            b[j] = controlPoints[i+j];

        // N_POINTS_PER_SEGMENTS points for each curve segment
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j < N_POINTS_PER_SEGMENTS; j++)
        {
            float t = (float)j / (N_POINTS_PER_SEGMENTS-1);

            Vector3f pt = BsplinePoint(b, t);
            glVertex3fv(pt.data());
        }
        glEnd();
    }

    // Control points
    glPointSize(10);
    glColor3f(1, 0, 0);
    glBegin(GL_POINTS);
    for (int i = 0; i < p.size(); i++)
    {
        glVertex3f(p[i][0], p[i][1], p[i][2]);
    }
    glEnd();
}

// Draw the polygon for selecting edges
void drawPolygon()
{
    glLineWidth(3);
    glColor3f(0, 0, 1);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < p.size(); i++)
    {
        glVertex3f(p[i][0], p[i][1], p[i][2]);
    }
    glEnd();
}

void render(GLFWwindow* window)
{
    // Background color
    glClearColor(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(eye[0],eye[1],eye[2], center[0],center[1],center[2], up[0],up[1],up[2]);

    // Draw the B-spline curve
    drawBSpline();

    // Draw the polygon when pressed i key
    if (mode == 'i')
    {
        drawPolygon();
    }
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch(key)
        {
        // Quit
        case GLFW_KEY_Q:
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GL_TRUE); break;

        // Mode
        case GLFW_KEY_A: cout << "ADD MODE" << endl;    mode = 'a'; break;
        case GLFW_KEY_R: cout << "REMOVE MODE" << endl; mode = 'r'; break;
        case GLFW_KEY_D: cout << "DRAG MODE" << endl;   mode = 'd'; break;
        case GLFW_KEY_I: cout << "INSERT MODE" << endl; mode = 'i'; break;

        default: cerr << "Press the 'a', 'r', 'd', 'i' key to change the mode." << endl; break;
        }
    }
}

// Find the nearset point to input position
int selectPoint(double x, double y)
{
    double minDist = 15;
    int    nearestPoint = -1;

    for (int i = 0; i < p.size(); i++)
    {
        double dist = sqrt((x - p[i][0]) * (x - p[i][0]) + (y - p[i][1]) * (y - p[i][1]));
        if (dist < minDist)
        {
            nearestPoint = i;
            minDist = dist;
        }
    }

    return nearestPoint;
}

void addControlPoint(float x, float y)
{
    Vector3f v(x, y, 0);
    p.push_back(v);
    construct(repetition);
}

void removeControlPoint(int index)
{
    if (index >= 0)
    {
        p.erase(p.begin() + index);
        construct(repetition);
    }
}

void dragControlPoint(int index)
{
    if (index >= 0)
    {
        dragPoint = index;
        isDragging = true;
    }
}

void insertControlPoint(float xPos, float yPos)
{
    Vector3f inputPos(xPos, yPos, 0);
    float    minDist = 10;
    int      nearestLine = -1;

    // Find the nearest line to input position
    for (int i = 0; i < (int)p.size()-1; i++)
    {
        // Line segment area
        if (((p[i+1]-p[i]).dot(inputPos-p[i]) > 0) && ((p[i]-p[i+1]).dot(inputPos-p[i+1])) > 0)
        {
            // Distance between a point and line segment
            float dist = ParametrizedLine<float, 3>::Through(p[i], p[i+1]).distance(inputPos);
            if (dist < minDist)
            {
                minDist = dist;
                nearestLine = i;
            }
        }
    }

    if (nearestLine >= 0)
    {
        p.insert(p.begin()+nearestLine+1, inputPos);
        construct(repetition);
    }
}

void convertCoordinates(double* xPos, double *yPos)
{
    *xPos = *xPos - coordinateX;
    *yPos = coordinateY - *yPos;
}

void mouse(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        switch (mode)
        {
        case 'a':
            if (p.size() < N)
            {
                glfwGetCursorPos(window, &xPos, &yPos);
                convertCoordinates(&xPos, &yPos);
                addControlPoint(xPos, yPos);
            }
            break;

        case 'r':
            glfwGetCursorPos(window, &xPos, &yPos);
            convertCoordinates(&xPos, &yPos);
            removeControlPoint(selectPoint(xPos, yPos));
            break;

        case 'd':
            glfwGetCursorPos(window, &xPos, &yPos);
            convertCoordinates(&xPos, &yPos);
            dragControlPoint(selectPoint(xPos, yPos));
            break;

        case 'i':
            if (p.size() < N)
            {
                glfwGetCursorPos(window, &xPos, &yPos);
                convertCoordinates(&xPos, &yPos);
                insertControlPoint(xPos, yPos);
            }
            break;

        default: cerr << "Press the 'a', 'r', 'd', 'i' key to change the mode." << endl; break;
        }
    }

    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        switch (mode)
        {
        case 'd': isDragging = false; break;

        default : break;
        }
    }
}