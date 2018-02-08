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

//GLuint myTex = -1;
//GLuint furTex = -1;
//GLuint windowTex = -1;

Model* wallsModel = NULL;
Model* roofModel = NULL;
Model* bladeModel = NULL;
Model* balconyModel = NULL;

// Reference to shader program
GLuint program = -1;

//GLuint planeVertexArrayObj = -1;

const float near = 1.0;
const float far = 100.0;
const float right = 0.5;
const float left = -0.5;
const float top = 0.5;
const float bottom = -0.5;

GLfloat cameraX = 0;
GLfloat cameraY = 0;
GLfloat cameraZ = 50;

GLfloat cameraYaw = 0;
GLfloat cameraPitch = 0;

Model* init_model(const char* modelname)
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

    // if (model->texCoordArray != NULL)
    // {
    //     glBindBuffer(GL_ARRAY_BUFFER, model->tb);
    //     glVertexAttribPointer(glGetAttribLocation(program, "in_TexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
    //     glEnableVertexAttribArray(glGetAttribLocation(program, "in_TexCoord"));
    // 
    //     glBindTexture(GL_TEXTURE_2D, furTex);
    // }
    // printError("in_TexCoord");
    return model;
}


/*void init_plane() {
    GLuint vertexArrayObjID;
    GLuint planeVertexBufferObjID;
    GLuint planeNormalBufferObjID;
    GLuint planeTexCoordBufferObjID;
    GLuint planeIndexBufferObjID;

    // Upload geometry to the GPU:
	
	// Allocate and activate Vertex Array Object
    glGenVertexArrays(1, &vertexArrayObjID);
    glGenBuffers(1, &planeVertexBufferObjID);
    glGenBuffers(1, &planeNormalBufferObjID);
    glGenBuffers(1, &planeTexCoordBufferObjID);
    glGenBuffers(1, &planeIndexBufferObjID);
    
    glBindVertexArray(vertexArrayObjID);

    GLfloat vertexArray[] = {
         0.0, 0.0, 0.0,
         5.0, 0.0, 0.0,
         0.0, 0.0, 5.0,
         5.0, 0.0, 5.0
    };

    GLfloat normalArray[] = {
        0.0, 1.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 1.0, 0.0
    };

    GLuint indexArray[] = {
        0, 2, 1,
        1, 2, 3
    };

    GLfloat texCoords[] = {
        0.0, 0.0,
        1.0, 0.0,
        0.0, 1.0,
        1.0, 1.0
    };

    // VBO for vertex data
    glBindBuffer(GL_ARRAY_BUFFER, planeVertexBufferObjID);
    glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), vertexArray, GL_STATIC_DRAW);
    glVertexAttribPointer(glGetAttribLocation(program, "in_Position"), 3, GL_FLOAT, GL_FALSE, 0, 0); 
    glEnableVertexAttribArray(glGetAttribLocation(program, "in_Position"));
    printError("plane in_Position");

    // VBO for normal data
    glBindBuffer(GL_ARRAY_BUFFER, planeNormalBufferObjID);
    glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), normalArray, GL_STATIC_DRAW);
    glVertexAttribPointer(glGetAttribLocation(program, "in_Normal"), 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(glGetAttribLocation(program, "in_Normal"));
    printError("in_Normal");

    glBindBuffer(GL_ARRAY_BUFFER, planeTexCoordBufferObjID);
    glBufferData(GL_ARRAY_BUFFER, 8*sizeof(GLfloat), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(glGetAttribLocation(program, "in_TexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(glGetAttribLocation(program, "in_TexCoord"));

    glBindTexture(GL_TEXTURE_2D, myTex);
    printError("plane in_TexCoord");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeIndexBufferObjID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLuint), indexArray, GL_STATIC_DRAW);    
	printError("plane init arrays");

    planeVertexArrayObj = vertexArrayObjID;
}*/

// void drawPlane(mat4* view, mat4* viewProj) {
//     glBindVertexArray(planeVertexArrayObj);
//     
//     mat4 trans = T(-2, -1, -2);
//     mat4 mvp = Mult(*viewProj, trans);
//     mat4 mv = Mult(*view, trans);
// 
//     glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_TRUE, mvp.m);
//     glUniformMatrix4fv(glGetUniformLocation(program, "mv"), 1, GL_TRUE, mv.m);
// 
//     glUniform1i(glGetUniformLocation(program, "texUnit"), 0);
//     glBindTexture(GL_TEXTURE_2D, myTex);
//     glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
// }

void drawModel(Model* model, mat4* modelView, mat4* modelViewProj) {
    glBindVertexArray(model->vao);

    glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_TRUE, modelViewProj->m);
    glUniformMatrix4fv(glGetUniformLocation(program, "mv"), 1, GL_TRUE, modelView->m);

    //glUniform1i(glGetUniformLocation(program, "texUnit"), 0);
    //glBindTexture(GL_TEXTURE_2D, windowTex);
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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    updateCamera();

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    mat4 frust = frustum(left, right, bottom, top, near, far);
    mat4 cameraOffset = T(-cameraX, -cameraY, -cameraZ);
    mat4 cameraRotation = Mult(Rx(cameraPitch), Ry(cameraYaw));
    mat4 view = Mult(cameraRotation, cameraOffset);
    mat4 viewProj = Mult(frust, view);
    drawModel(wallsModel, &view, &viewProj);
    drawModel(roofModel, &view, &viewProj);
    drawModel(balconyModel, &view, &viewProj);
    mat4 bladeOffset = T(4.5, 9.25, 0);

    for(int i = 0; i < 4; i++) {
        mat4 bladeRotation = Rx(i * M_PI / 2 + t);
        mat4 bladeModelMatrix = Mult(bladeOffset, bladeRotation);

        mat4 bladeView = Mult(view, bladeModelMatrix);
        mat4 bladeProj = Mult(viewProj, bladeModelMatrix);
        drawModel(bladeModel, &bladeView, &bladeProj);
    }
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
	glClearColor(0.2,0.2,0.5,0);
	glDisable(GL_DEPTH_TEST);
	printError("GL inits");

    // Load and compile shader
	program = loadShaders("lab3-2.vert", "lab3-2.frag");
	printError("init shader");

    //LoadTGATextureSimple("maskros512.tga", &myTex);
    //LoadTGATextureSimple("fur.tga", &furTex);
    //LoadTGATextureSimple("rutor.tga", &windowTex);

	wallsModel = init_model("windmill/windmill-walls.obj");
    roofModel = init_model("windmill/windmill-roof.obj");
    bladeModel = init_model("windmill/blade.obj");
    balconyModel = init_model("windmill/windmill-balcony.obj");
    //init_plane();

	glutMainLoop();
	return 0;
}
