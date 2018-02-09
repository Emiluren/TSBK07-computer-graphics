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

GLuint myTex = -1;
GLuint furTex = -1;
GLuint carTex = -1;

Model* bunnyModel = NULL;
Model* carModel = NULL;

// Reference to shader program
GLuint program = -1;

GLuint planeVertexArrayObj = -1;

const float near = 1.0;
const float far = 30.0;
const float right = 0.5;
const float left = -0.5;
const float top = 0.5;
const float bottom = -0.5;

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

    if (model->texCoordArray != NULL)
    {
        glBindBuffer(GL_ARRAY_BUFFER, model->tb);
        glVertexAttribPointer(glGetAttribLocation(program, "in_TexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(glGetAttribLocation(program, "in_TexCoord"));

        glBindTexture(GL_TEXTURE_2D, furTex);
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
}

void drawPlane(mat4* view, mat4* viewProj) {
    glBindVertexArray(planeVertexArrayObj);
    
    mat4 trans = T(-2, -1, -2);
    mat4 mvp = Mult(*viewProj, trans);
    mat4 mv = Mult(*view, trans);

    glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_TRUE, mvp.m);
    glUniformMatrix4fv(glGetUniformLocation(program, "mv"), 1, GL_TRUE, mv.m);

    glUniform1i(glGetUniformLocation(program, "texUnit"), 0);
    glBindTexture(GL_TEXTURE_2D, myTex);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void drawBunny(mat4* view, mat4* viewProj) {
    glBindVertexArray(bunnyModel->vao); // Select VAO
    GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME) / 300.0f;

    mat4 trans = T(0, 0, 0);
    mat4 rot = Ry(t);
    mat4 model = Mult(trans, rot);
    mat4 mvp = Mult(*viewProj, model);
    mat4 mv = Mult(*view, model);

    glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_TRUE, mvp.m);
    glUniformMatrix4fv(glGetUniformLocation(program, "mv"), 1, GL_TRUE, mv.m);
    glUniformMatrix4fv(glGetUniformLocation(program, "v"), 1, GL_TRUE, view->m);

    glUniform1i(glGetUniformLocation(program, "texUnit"), 0);
    glBindTexture(GL_TEXTURE_2D, furTex);
    glDrawElements(GL_TRIANGLES, bunnyModel->numIndices, GL_UNSIGNED_INT, 0);
}

void drawCar(mat4* view, mat4* viewProj) {
    glBindVertexArray(carModel->vao);

    mat4 trans = T(2, -1, 0);
    mat4 mvp = Mult(*viewProj, trans);
    mat4 mv = Mult(*view, trans);

    glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_TRUE, mvp.m);
    glUniformMatrix4fv(glGetUniformLocation(program, "mv"), 1, GL_TRUE, mv.m);
    glUniformMatrix4fv(glGetUniformLocation(program, "v"), 1, GL_TRUE, view->m);

    glUniform1i(glGetUniformLocation(program, "texUnit"), 0);
    glBindTexture(GL_TEXTURE_2D, carTex);
    glDrawElements(GL_TRIANGLES, carModel->numIndices, GL_UNSIGNED_INT, 0);
}

void display(void)
{
	printError("pre display");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    GLfloat cameraX = 5 * cos(t);
    GLfloat cameraZ = 5 * sin(t);

    mat4 frust = frustum(left, right, bottom, top, near, far);
    mat4 view = lookAt(cameraX, 3, cameraZ,
                       0, 0, 0,
                       0, 1, 0);
    mat4 viewProj = Mult(frust, view);
    drawPlane(&view, &viewProj);
    drawBunny(&view, &viewProj);
    drawCar(&view, &viewProj);
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

	// GL inits
	glClearColor(0.2,0.2,0.5,0);
	glDisable(GL_DEPTH_TEST);
	printError("GL inits");

    // Load and compile shader
	program = loadShaders("lab2-7.vert", "lab2-7.frag");
	printError("init shader");

    LoadTGATextureSimple("maskros512.tga", &myTex);
    LoadTGATextureSimple("fur.tga", &furTex);
    LoadTGATextureSimple("bilskissred.tga", &carTex);

	bunnyModel = init_model("bunnyplus.obj");
    carModel = init_model("bilskiss.obj");
    init_plane();

	glutMainLoop();
	return 0;
}
