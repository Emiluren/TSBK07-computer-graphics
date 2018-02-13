// Lab 4, terrain generation

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

mat4 projectionMatrix;

GLfloat cameraX = 0;
GLfloat cameraY = 5;
GLfloat cameraZ = 10;

GLfloat cameraYaw = 1.5;
GLfloat cameraPitch = 0.8;

GLfloat heightAtPoint(int x, int z, int width, int height, GLfloat* vertexArray) {
    if (x < 0)
        x = 0;
    else if (x >= width)
        x = width - 1;

    if (z < 0)
        z = 0;
    else if (z >= height)
        z = height - 1;

    return vertexArray[(x + z * width)*3 + 1];
}

Model* GenerateTerrain(TextureData *tex)
{
    int vertexCount = tex->width * tex->height;
    int triangleCount = (tex->width-1) * (tex->height-1) * 2;
    int x, z;

    GLfloat* vertexArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
    GLfloat* normalArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
    GLfloat* texCoordArray = malloc(sizeof(GLfloat) * 2 * vertexCount);
    GLuint* indexArray = malloc(sizeof(GLuint) * triangleCount*3);

    GLfloat gridSize = 1.0 / 10.0;
    printf("bpp %d\n", tex->bpp);
    for (x = 0; x < tex->width; x++) {
        for (z = 0; z < tex->height; z++) {
            // Vertex coordinates
            vertexArray[(x + z * tex->width)*3 + 0] = x * gridSize;
            vertexArray[(x + z * tex->width)*3 + 1] = tex->imageData[(x + z * tex->width) * (tex->bpp/8)] / 100.0;
            vertexArray[(x + z * tex->width)*3 + 2] = z * gridSize;

            // Texture coordinates
            texCoordArray[(x + z * tex->width)*2 + 0] = x; // (float)x / tex->width;
            texCoordArray[(x + z * tex->width)*2 + 1] = z; // (float)z / tex->height;
        }
    }

    // Calculate vertex normals
    for (x = 0; x < tex->width; x++) {
        for (z = 0; z < tex->height; z++) {
            GLfloat leftHeight = heightAtPoint(x - 1, z, tex->width, tex->height, vertexArray);
            GLfloat rightHeight = heightAtPoint(x + 1, z, tex->width, tex->height, vertexArray);
            GLfloat aboveHeight = heightAtPoint(x, z - 1, tex->width, tex->height, vertexArray);
            GLfloat belowHeight = heightAtPoint(x, z + 1, tex->width, tex->height, vertexArray);

            vec3 normal = Normalize(SetVector(
                leftHeight - rightHeight,
                2 * gridSize,
                aboveHeight - belowHeight
            ));

            normalArray[(x + z * tex->width)*3 + 0] = normal.x;
            normalArray[(x + z * tex->width)*3 + 1] = normal.y;
            normalArray[(x + z * tex->width)*3 + 2] = normal.z;
        }
    }
    for (x = 0; x < tex->width-1; x++) {
        for (z = 0; z < tex->height-1; z++) {
            // Triangle 1
            indexArray[(x + z * (tex->width-1))*6 + 0] = x + z * tex->width;
            indexArray[(x + z * (tex->width-1))*6 + 1] = x + (z+1) * tex->width;
            indexArray[(x + z * (tex->width-1))*6 + 2] = x+1 + z * tex->width;
            // Triangle 2
            indexArray[(x + z * (tex->width-1))*6 + 3] = x+1 + z * tex->width;
            indexArray[(x + z * (tex->width-1))*6 + 4] = x + (z+1) * tex->width;
            indexArray[(x + z * (tex->width-1))*6 + 5] = x+1 + (z+1) * tex->width;
        }
    }

    // End of terrain generation

    // Create Model and upload to GPU:

    Model* model = LoadDataToModel(
        vertexArray,
        normalArray,
        texCoordArray,
        NULL,
        indexArray,
        vertexCount,
        triangleCount*3
    );

    return model;
}


// vertex array object
Model *m, *m2, *tm;
// Reference to shader program
GLuint program;
GLuint tex1, tex2;
TextureData ttex; // terrain

void init()
{
    // GL inits
    glClearColor(0.8, 0.8, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    printError("GL inits");

    projectionMatrix = frustum(-0.1, 0.1, -0.1, 0.1, 0.2, 50.0);

    // Load and compile shader
    program = loadShaders("terrain4-3.vert", "terrain4-3.frag");
    glUseProgram(program);
    printError("init shader");

    glUniform1i(glGetUniformLocation(program, "tex"), 0); // Texture unit 0
    LoadTGATextureSimple("grass.tga", &tex1);

    // Load terrain data

    LoadTGATextureData("fft-terrain.tga", &ttex);
    tm = GenerateTerrain(&ttex);
    printError("init terrain");
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

void updateCamera() {
    glutWarpPointer(200, 200);

    float dx = 0, dy = 0;
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

    float cameraSpeed = 0.2;
    dx *= cameraSpeed;
    dy *= cameraSpeed;

    cameraX += dx * cos(cameraYaw);
    cameraZ += dx * sin(cameraYaw);

    cameraX += dy * cos(cameraPitch) * sin(cameraYaw);
    cameraY += dy * sin(-cameraPitch);
    cameraZ += dy * cos(cameraPitch) * cos(cameraYaw) * -1;
}

void display()
{
    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    printError("pre display");

    updateCamera();

    glUseProgram(program);

    mat4 cameraOffset = T(-cameraX, -cameraY, -cameraZ);
    mat4 cameraRotation = Mult(Rx(cameraPitch), Ry(cameraYaw));
    mat4 camMatrix = Mult(cameraRotation, cameraOffset);

    mat4 total = Mult(projectionMatrix, camMatrix);

    glUniformMatrix4fv(glGetUniformLocation(program, "worldMatrix"), 1, GL_TRUE, IdentityMatrix().m);
    glUniformMatrix4fv(glGetUniformLocation(program, "totalMatrix"), 1, GL_TRUE, total.m);

    glBindTexture(GL_TEXTURE_2D, tex1);     // Bind Our Texture tex1
    DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");

    printError("display 2");

    glutSwapBuffers();
}

void timer(int i)
{
    glutTimerFunc(20, &timer, i);
    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitContextVersion(3, 2);
    glutInitWindowSize(600, 600);
    glutCreateWindow("TSBK07 Lab 4");
    glutDisplayFunc(display);
    init();
    glutTimerFunc(20, &timer, 0);

    glutHideCursor();
    glutWarpPointer(200, 200);
    glutPassiveMotionFunc(cameraCallback);

    glutMainLoop();
    exit(0);
}
