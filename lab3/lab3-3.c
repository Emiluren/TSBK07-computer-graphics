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

GLuint skyboxTex = -1;
GLuint grassTex = -1;
GLuint gridTex = -1;
GLuint carTex = -1;

Model* wallsModel = NULL;
Model* roofModel = NULL;
Model* bladeModel = NULL;
Model* balconyModel = NULL;

Model* skyboxModel = NULL;
Model* carModel = NULL;

// Reference to shader program
GLuint diffuse_program = -1;
GLuint skybox_program = -1;

GLuint planeVertexArrayObj = -1;

const float near = 1.0;
const float far = 200.0;
const float right = 0.5;
const float left = -0.5;
const float top = 0.5;
const float bottom = -0.5;

GLfloat cameraX = 0;
GLfloat cameraY = 10;
GLfloat cameraZ = 50;

GLfloat cameraYaw = 0;
GLfloat cameraPitch = 0;

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

void init_plane() {
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
         -100.0, 0.0, -100.0,
         100.0, 0.0, -100.0,
         -100.0, 0.0, 100.0,
         100.0, 0.0, 100.0
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
        20.0, 0.0,
        0.0, 20.0,
        20.0, 20.0
    };

    GLuint program = diffuse_program;

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
    printError("plane in_Normal");

    glBindBuffer(GL_ARRAY_BUFFER, planeTexCoordBufferObjID);
    glBufferData(GL_ARRAY_BUFFER, 8*sizeof(GLfloat), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(glGetAttribLocation(program, "in_TexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(glGetAttribLocation(program, "in_TexCoord"));
    printError("plane in_TexCoord");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeIndexBufferObjID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLuint), indexArray, GL_STATIC_DRAW);    
	printError("plane init arrays");

    planeVertexArrayObj = vertexArrayObjID;
}

void drawPlane(mat4* view, mat4* viewProj) {
    glUseProgram(diffuse_program);
    glBindVertexArray(planeVertexArrayObj);
    
    glUniformMatrix4fv(glGetUniformLocation(diffuse_program, "mvp"), 1, GL_TRUE, viewProj->m);
    glUniformMatrix4fv(glGetUniformLocation(diffuse_program, "mv"), 1, GL_TRUE, view->m);
    glUniformMatrix4fv(glGetUniformLocation(diffuse_program, "v"), 1, GL_TRUE, view->m);

    glUniform1i(glGetUniformLocation(diffuse_program, "texUnit"), 0);
    glBindTexture(GL_TEXTURE_2D, grassTex);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void drawModel(Model* model, mat4* modelView, mat4* modelViewProj, mat4* view) {
    glUseProgram(diffuse_program);
    glBindVertexArray(model->vao);

    glUniformMatrix4fv(glGetUniformLocation(diffuse_program, "mvp"), 1, GL_TRUE, modelViewProj->m);
    glUniformMatrix4fv(glGetUniformLocation(diffuse_program, "mv"), 1, GL_TRUE, modelView->m);
    glUniformMatrix4fv(glGetUniformLocation(diffuse_program, "v"), 1, GL_TRUE, view->m);

    glUniform1i(glGetUniformLocation(diffuse_program, "texUnit"), 0);
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

    GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    mat4 frust = frustum(left, right, bottom, top, near, far);
    mat4 cameraOffset = T(-cameraX, -cameraY, -cameraZ);
    mat4 cameraRotation = Mult(Rx(cameraPitch), Ry(cameraYaw));

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(skybox_program);
    glBindVertexArray(skyboxModel->vao);
    glUniformMatrix4fv(glGetUniformLocation(skybox_program, "v"), 1, GL_TRUE, Mult(frust, cameraRotation).m);
    glUniform1i(glGetUniformLocation(skybox_program, "texUnit"), 0);
    glBindTexture(GL_TEXTURE_2D, skyboxTex);
    glDrawElements(GL_TRIANGLES, skyboxModel->numIndices, GL_UNSIGNED_INT, 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    mat4 view = Mult(cameraRotation, cameraOffset);
    mat4 viewProj = Mult(frust, view);

    drawPlane(&view, &viewProj);

    glBindTexture(GL_TEXTURE_2D, gridTex);
    drawModel(wallsModel, &view, &viewProj, &view);
    drawModel(roofModel, &view, &viewProj, &view);
    drawModel(balconyModel, &view, &viewProj, &view);
    mat4 bladeOffset = T(4.5, 9.25, 0);

    for(int i = 0; i < 4; i++) {
        mat4 bladeRotation = Rx(i * M_PI / 2 + t);
        mat4 bladeWorldMatrix = Mult(bladeOffset, bladeRotation);

        mat4 bladeView = Mult(view, bladeWorldMatrix);
        mat4 bladeViewProj = Mult(viewProj, bladeWorldMatrix);
        drawModel(bladeModel, &bladeView, &bladeViewProj, &view);
    }

    mat4 carOffset = T(15, 0, 0);
    mat4 carRotation = Ry(t);
    mat4 carMatrix = Mult(carRotation, carOffset);
    mat4 carViewModel = Mult(view, carMatrix);
    mat4 carViewProject = Mult(viewProj, carMatrix);
    glBindTexture(GL_TEXTURE_2D, carTex);
    drawModel(carModel, &carViewModel, &carViewProject, &view);

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
	glClearColor(0.84, 0.96, 0.99, 0);
	glDisable(GL_DEPTH_TEST);
	printError("GL inits");

    // Load and compile shader
	diffuse_program = loadShaders("lab3-3.vert", "lab3-3.frag");
	printError("init diffuse shader");
	wallsModel = init_model("windmill/windmill-walls.obj", diffuse_program);
    roofModel = init_model("windmill/windmill-roof.obj", diffuse_program);
    bladeModel = init_model("windmill/blade.obj", diffuse_program);
    balconyModel = init_model("windmill/windmill-balcony.obj", diffuse_program);

    carModel = init_model("bilskiss.obj", diffuse_program);
    init_plane();

    skybox_program = loadShaders("lab3-3_skybox.vert", "lab3-3_skybox.frag");
    printError("init skybox shader");

    skyboxModel = init_model("skybox.obj", skybox_program);

    LoadTGATextureSimple("SkyBox512.tga", &skyboxTex);
    LoadTGATextureSimple("grass.tga", &grassTex);
    LoadTGATextureSimple("rutor.tga", &gridTex);
    LoadTGATextureSimple("bilskissred.tga", &carTex);

	glutMainLoop();
	return 0;
}
