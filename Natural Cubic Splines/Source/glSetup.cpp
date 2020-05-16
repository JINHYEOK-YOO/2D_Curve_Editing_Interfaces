#include "glSetup.h"

#include <string.h>
#include <iostream>
using namespace std;

double screenScale = 0.5;           // Port of the screen when not using full screen
int    screenW = 0, screenH = 0;    // screenScale portion of the screen
int    windowW, windowH;            // Framebuffer size
int    dpi;                         // Square root of the number of dots per inch
int    coordinateX, coordinateY;    // The coordinates

void errorCallback(int error, const char* description)
{
    cerr << "####" << description << endl;
}

void reshape(GLFWwindow* window, int w, int h)
{
    //cerr << "reshape(" << w << ", " << h << ")" << endl;
    windowW = w;
    windowH = h;

    glViewport(0, 0, w, h);

    float nearDist = 1.0;
    float farDist = 20.0;

    coordinateX = windowW / (2 * dpi);
    coordinateY = windowH / (2 * dpi);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(-coordinateX, coordinateX, -coordinateY, coordinateY, nearDist, farDist);
}

GLFWwindow* initializeOpenGL(int argc, char* argv[], GLfloat bgColor[4])
{
    glfwSetErrorCallback(errorCallback);

    // Init GLFW
    if(!glfwInit()) exit(EXIT_FAILURE);

    // Enable OpenGL 2.1 in OS X
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // Create the window
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    int monitorW, monitorH;
    glfwGetMonitorPhysicalSize(monitor, &monitorW, &monitorH);
    cerr << "Status: Monitor " << monitorW << "mm x " << monitorH << "mm" << endl;

    const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
    if (screenW == 0)  screenW = videoMode->width  * screenScale;
    if (screenH == 0)  screenH = videoMode->height * screenScale;

    GLFWwindow* window = glfwCreateWindow(screenW, screenH, "Natural Cubic Spline", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        cerr << "Failed in glfwCreateWindow()" << endl;
        return NULL;
    }

    // Context
    glfwMakeContextCurrent(window);

    // Clear the background ASAP
    glClearColor(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    glfwSwapBuffers(window);

    // Check the size of the window
    glfwGetWindowSize(window, &screenW, &screenH);
    cerr << "Status: Screen " << screenW << " x " << screenH << endl;

    glfwGetFramebufferSize(window, &windowW, &windowH);
    cerr << "Status: Framebuffer " << windowW << " x " << windowH << endl;

    dpi = windowW / screenW;

    // Callbacks
    glfwSetFramebufferSizeCallback(window, reshape);

    // Get the OpenGL version and renderer
    cout << "Status: Renderer " << glGetString(GL_RENDERER) << endl;
    cout << "Status: Vendor " << glGetString(GL_VENDOR) << endl;
    cout << "Status: OpenGL " << glGetString(GL_VERSION) << endl;

    // GLSL version for shader loading
    char glslVersion[256];
    strcpy(glslVersion, (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    cout << "Status: GLSL " << glslVersion << endl;

    // Vertical sync ...
    glfwSwapInterval(0);    // Immediate mode (Tearing possible)
    //glfwSwapInterval(1);

    // GLEW: Supported version can be verified in http://glew.sourceforge.net/basic.html
    cerr << "Status: GLEW " << glewGetString(GLEW_VERSION) << endl;

    // Initializing GLEW
    GLenum error = glewInit();
    if (error != GLEW_OK)
    {
        cerr << "ERROR: " << glewGetErrorString(error) << endl;
        return 0;
    }
    return window;
}

// Draw the coordinate frame with the xyz axes
void drawAxes(float l, float w)
{
    glLineWidth(w);

    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(l, 0, 0); // x-axis
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, l, 0); // y-axis
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, l); // z-axis
    glEnd();
}