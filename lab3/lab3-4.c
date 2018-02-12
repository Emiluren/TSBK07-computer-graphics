// Lab 1-1.
// This is the same as the first simple example in the course book,
// but with a few error checks.
// Remember to copy your file to a new on appropriate places during the lab so you keep old results.
// Note that the files "lab1-1.frag", "lab1-1.vert" are required.

// Should work as is on Linux and Mac. MS Windows needs GLEW or glee.
// See separate Visual Studio version of my demos.
#ifdef __APPLE__
	#include <OpenGL/gl3.h>
	// Linking hint for Lightweight IDE
	// uses framework Cocoa
#endif
#include "MicroGlut.h"
#include "GL_utilities.h"
#include "VectorUtils3.h"
#include "loadobj.h"
#include "LoadTGA.h"
#include <math.h>

Model* carModel = NULL;

// Reference to shader program
GLuint program = -1;

const float near = 1.0;
const float far = 200.0;
const float right = 0.5;
const float left = -0.5;
const float top = 0.5;
const float bottom = -0.5;

GLfloat cameraX = 0;
GLfloat cameraY = 0;
GLfloat cameraZ = 10;

GLfloat cameraYaw = 0;
GLfloat cameraPitch = 0;

Point3D lightSourcesColorsArr[] = {
    {1.0f, 0.0f, 0.0f}, // Red light
    {0.0f, 1.0f, 0.0f}, // Green light
    {0.0f, 0.0f, 1.0f}, // Blue light
    {1.0f, 1.0f, 1.0f} // White light
};

GLint isDirectional[] = {0,0,1,1};

Point3D lightSourcesDirectionsPositions[] = {
    {10.0f, 5.0f, 0.0f}, // Red light, positional
    {0.0f, 5.0f, 10.0f}, // Green light, positional
    {-1.0f, 0.0f, 0.0f}, // Blue light along X
    {0.0f, 0.0f, -1.0f} // White light along Z
};

GLfloat specularExponent[] = {100.0, 200.0, 60.0, 50.0, 300.0, 150.0};

Model* init_model(const char* modelname, GLuint program)
{
    Model* model = LoadModelPlus(modelname);
    glBindVertexArray(model->vao);
	
    // VBO for vertex data
    glBindBuffer(GL_ARRAY_BUFFER, model->vb);
    glVertexAttribPointer(glGetAttribLocation(program, "in_Position"), 3, GL_FLOAT, GL_FALSE, 0, 0); 
    glEnableVertexAttribArray(glGetAttribLocation(program, "in_Position"));
    printError("in_Position");

    // VBO for normal data
    glBindBuffer(GL_ARRAY_BUFFER, model->nb);
    glVertexAttribPointer(glGetAttribLocation(program, "in_Normal"), 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(glGetAttribLocation(program, "in_Normal"));
    printError("in_Normal");

    if (model->texCoordArray != NULL)
    {
        glBindBuffer(GL_ARRAY_BUFFER, model->tb);
        glVertexAttribPointer(glGetAttribLocation(program, "in_TexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(glGetAttribLocation(program, "in_TexCoord"));
    }
    printError("in_TexCoord");
    return model;
}

void drawModel(Model* model, mat4* modelView, mat4* modelViewProj, mat4* view) {
    glUseProgram(program);
    glBindVertexArray(model->vao);

    glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_TRUE, modelViewProj->m);
    glUniformMatrix4fv(glGetUniformLocation(program, "mv"), 1, GL_TRUE, modelView->m);
    glUniformMatrix4fv(glGetUniformLocation(program, "v"), 1, GL_TRUE, view->m);

    glUniform3fv(glGetUniformLocation(program, "lightSourcesDirPosArr"), 4, &lightSourcesDirectionsPositions[0].x);
    glUniform3fv(glGetUniformLocation(program, "lightSourcesColorArr"), 4, &lightSourcesColorsArr[0].x);

    // TODO: draw several models for all exponents
    glUniform1f(glGetUniformLocation(program, "specularExponent"), specularExponent[0]);
    glUniform1iv(glGetUniformLocation(program, "isDirectional"), 4, isDirectional);
    printError("upload light data");

    glDrawElements(GL_TRIANGLES, model->numIndices, GL_UNSIGNED_INT, 0);
}

float clamp(float min, float v, float max) {
    if (v < min)
        v = min;
    if (v > max)
        v = max;
    return v;
}

void cameraCallback(int x, int y) {
    int dx = x - 200;
    int dy = y - 200;

    cameraYaw += dx * 0.005;
    cameraPitch = clamp(-M_PI / 2, cameraPitch + dy * 0.005, M_PI / 2);
}

void updateCamera(void) {
    glutWarpPointer(200, 200);

    int dx = 0, dy = 0;
    if (glutKeyIsDown('a')) {
        dx -= 1;
    }
    if (glutKeyIsDown('d')) {
        dx += 1;
    }
    if (glutKeyIsDown('w')) {
        dy += 1;
    }
    if (glutKeyIsDown('s')) {
        dy -= 1;
    }

    cameraX += dx * cos(cameraYaw);
    cameraZ += dx * sin(cameraYaw);

    cameraX += dy * cos(cameraPitch) * sin(cameraYaw);
    cameraY += dy * sin(-cameraPitch);
    cameraZ += dy * cos(cameraPitch) * cos(cameraYaw) * -1;
}

void display(void)
{
	printError("pre display");

    updateCamera();

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 frust = frustum(left, right, bottom, top, near, far);
    mat4 cameraOffset = T(-cameraX, -cameraY, -cameraZ);
    mat4 cameraRotation = Mult(Rx(cameraPitch), Ry(cameraYaw));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    mat4 view = Mult(cameraRotation, cameraOffset);
    mat4 viewProj = Mult(frust, view);

    drawModel(carModel, &view, &viewProj, &view);

	printError("display");
    glFinish();
	
	glutSwapBuffers();
}

void OnTimer(int value)
{
    glutPostRedisplay();
    glutTimerFunc(20, &OnTimer, value);
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitContextVersion(3, 2);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow ("GL3 white triangle example");
	glutDisplayFunc(display);
    glutTimerFunc(20, &OnTimer, 0);
    //glutRepeatingTimer(20);
	dumpInfo();

    glutHideCursor();
    glutWarpPointer(200, 200);
    glutPassiveMotionFunc(cameraCallback);

	// GL inits
	glClearColor(0.2, 0.2, 0.2, 0);
	glDisable(GL_DEPTH_TEST);
	printError("GL inits");

    // Load and compile shader
	program = loadShaders("lab3-4.vert", "lab3-4.frag");
	printError("init diffuse shader");

    carModel = init_model("bilskiss.obj", program);

	glutMainLoop();
	return 0;
}
