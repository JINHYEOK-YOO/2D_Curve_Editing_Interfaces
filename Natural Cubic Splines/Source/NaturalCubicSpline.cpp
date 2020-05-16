#include "glSetup.h"

#include <Eigen/Dense>
#include <Eigen/Geometry>
using namespace Eigen;

#include <iostream>
#include <vector>
#include <math.h>
using namespace std;

void init();

void render(GLFWwindow* window);
void reshape(GLFWwindow* window, int w, int h);
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse(GLFWwindow* window, int button, int action, int mods);

void convertCoordinates(double* xPos, double *yPos);

void arrangeMatrix();
void solveLinearSystem();

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

// (x, y, z) positions of the data
const int        N = 11;
vector<Vector3f> p;


// Linear system : Ac = b
//  from n equations of the form:
//  p_i(t) = c_0^i + (c_1^i * t^1) + (c_2^i * t^2) + (c_3^i * t^3)
//
MatrixXd A;
MatrixXd b;  // 4n x 3 to solve the 3 linear systems at once
MatrixXd c;  // 4n x 3 to solve the 3 linear systems at once

int main(int argc, char* argv[])
{
    
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
    init();

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        render(window);             // Draw one frame
        glfwSwapBuffers(window);    // Swap buffers
        glfwPollEvents();           // Events

        if (isDragging)             // Drag
        {
            glfwGetCursorPos(window, &xPos, &yPos);
            convertCoordinates(&xPos, &yPos);
            p[dragPoint][0] = xPos;
            p[dragPoint][1] = yPos;
            arrangeMatrix();
            solveLinearSystem();
        }
    }

    // Terminate the glfw system
    glfwDestroyWindow(window);  // Is it required?
    glfwTerminate();

    return 0;
}

void buildLinearSystem()
{
    // Build A and b for N segments. A is independent of the locations of the points.
    A.resize(4*N, 4*N);
    A.setZero();

    b.resize(4*N, 3);
    b.setZero();

    // Equation number
    int row = 0;

    // 2n equations for end point interpolation
    for (int i = 0; i < N; i++, row += 2)
    {
        // p_i(0) = c_0^i
        A(row, 4*i+0) = 1;

        // p_i(1) = c_0^i + c_1^i + c_2^i + c_3^i
        A(row+1, 4*i+0) = 1;
        A(row+1, 4*i+1) = 1;
        A(row+1, 4*i+2) = 1;
        A(row+1, 4*i+3) = 1;
    }

    // (n-1) equations for the tangential continuity
    for (int i = 0; i < N-1; i++, row++)
    {
        // p'_i(1) = 1*c_1^i + 2*c_2^i + 3*c_3^i = c_1^(i+1) = p'_(i+1)(0)
        A(row, 4*i+1) = 1;
        A(row, 4*i+2) = 2;
        A(row, 4*i+3) = 3;
        A(row, 4*i+5) = -1;
    }

    // (n-1) equations for the second-derivative continuity
    for (int i = 0; i < N-1; i++, row++)
    {
        // p''_i(1) = 2*c_2^i + 6*c_3^i = 2*c_2^(i+1) = p''_(i+1)(0)
        A(row, 4*i+2) = 2;
        A(row, 4*i+3) = 6;
        A(row, 4*i+6) = -2;
    }            

    // Natural spline boundary condition
    {
        // p''_0(0) = 2*c_2^0 = 0
        A(row, 2) = 2;
    }
}

void init()
{
    cout << "Maximum of data points: " << N+1 << endl;
    buildLinearSystem();
}

// Arrange elements of Matrix A and b by data points
void arrangeMatrix()
{
    int row = 0;

    b.setZero();

    for (int i = 0; i < (int)p.size()-1; i++, row += 2)
    {
        b(row, 0) = p[i][0];
        b(row, 1) = p[i][1];
        b(row, 2) = p[i][2];

        b(row+1, 0) = p[i+1][0];
        b(row+1, 1) = p[i+1][1];
        b(row+1, 2) = p[i+1][2];
    }

    A.row(4*N-1).setZero();

    if (p.size() >= 2)
    {
        A(4*N-1, 4*(p.size()-2)+2) = 2;
        A(4*N-1, 4*(p.size()-2)+3) = 6;
    }
}

void solveLinearSystem()
{
    // solve the 3 linear systems at once
    c = A.colPivHouseholderQr().solve(b);
}

// Draw the natural cubic spline
void drawNaturalCubicSpline()
{
    int N_POINTS_PER_SEGMENTS = 40;

    // Curve
    glLineWidth(3);
    glColor3f(0, 0, 0);
    for (int i = 0; i < (int)p.size()-1; i++)
    {
        // N_POINTS_PER_SEGMENTS points for each curve segment
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j < N_POINTS_PER_SEGMENTS; j++)
        {
            float t = (float)j / (N_POINTS_PER_SEGMENTS-1);

            float x = c(4*i+0, 0) + (c(4*i+1, 0) + (c(4*i+2, 0) + c(4*i+3, 0)*t)*t)*t;
            float y = c(4*i+0, 1) + (c(4*i+1, 1) + (c(4*i+2, 1) + c(4*i+3, 1)*t)*t)*t;
            float z = c(4*i+0, 2) + (c(4*i+1, 2) + (c(4*i+2, 2) + c(4*i+3, 2)*t)*t)*t;

            glVertex3f(x, y, z);
        }
        glEnd();
    }

    // Data points
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

    // Draw the natural cubic spline curve
    drawNaturalCubicSpline();

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
    float  minDist = 15;
    int    nearestPoint = -1;

    for (int i = 0; i < p.size(); i++)
    {
        float dist = sqrt((x-p[i][0]) * (x-p[i][0]) + (y-p[i][1]) * (y-p[i][1]));
        if (dist < minDist)
        {
            nearestPoint = i;
            minDist = dist;
        }
    }

    return nearestPoint;
}

void addDataPoint(float xPos, float yPos)
{
    Vector3f v(xPos, yPos, 0);
    p.push_back(v);
    arrangeMatrix();
    solveLinearSystem();
}

void removeDataPoint(int index)
{
    if (index >= 0)
    {
        p.erase(p.begin() + index);
        arrangeMatrix();
        solveLinearSystem();
    }
}

void dragDataPoint(int index)
{
    if (index >= 0)
    {
        dragPoint = index;
        isDragging = true;
    }
}

void insertDataPoint(float xPos, float yPos)
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
        arrangeMatrix();
        solveLinearSystem();
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
            if (p.size() < N+1)
            {
                glfwGetCursorPos(window, &xPos, &yPos);
                convertCoordinates(&xPos, &yPos);
                addDataPoint(xPos, yPos);
            }
            break;

        case 'r':
            glfwGetCursorPos(window, &xPos, &yPos);
            convertCoordinates(&xPos, &yPos);
            removeDataPoint(selectPoint(xPos, yPos));
            break;

        case 'd':
            glfwGetCursorPos(window, &xPos, &yPos);
            convertCoordinates(&xPos, &yPos);
            dragDataPoint(selectPoint(xPos, yPos));
            break;

        case 'i':
            if (p.size() < N+1)
            {
                glfwGetCursorPos(window, &xPos, &yPos);
                convertCoordinates(&xPos, &yPos);
                insertDataPoint(xPos, yPos);
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