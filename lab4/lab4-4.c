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

GLfloat cameraX = 10;
GLfloat cameraY = 5;
GLfloat cameraZ = 10;

GLfloat cameraYaw = 0;
GLfloat cameraPitch = 0;

const GLfloat gridSize = 0.1;

TextureData terrainTexture;

GLfloat heightAtVertex(int x, int z) {
    if (x < 0)
        x = 0;
    else if (x >= terrainTexture.width)
        x = terrainTexture.width - 1;

    if (z < 0)
        z = 0;
    else if (z >= terrainTexture.height)
        z = terrainTexture.height - 1;

    return terrainTexture.imageData[(x + z * terrainTexture.width) * (terrainTexture.bpp/8)] / 100.0;
}

GLfloat heightAtPoint(GLfloat x, GLfloat z) {
    if (x < 0 || x >= terrainTexture.width)
        return 0;
    if (z < 0 || z >= terrainTexture.height)
        return 0;

    int xInt = (int)x;
    GLfloat xFrac = x - xInt;
    int zInt = (int)z;
    GLfloat zFrac = z - zInt;

    GLfloat tl, tr, bl, br;
    tl = heightAtVertex(xInt, zInt);
    tr = heightAtVertex(xInt + 1, zInt);
    bl = heightAtVertex(xInt, zInt + 1);
    br = heightAtVertex(xInt + 1, zInt + 1);

    return
        tl * (1 - xFrac) * (1 - zFrac) +
        tr * xFrac * (1 - zFrac) +
        bl * (1 - xFrac) * zFrac +
        br * xFrac * zFrac;
}

Model* GenerateTerrain(TextureData* tex)
{
    int vertexCount = tex->width * tex->height;
    int triangleCount = (tex->width-1) * (tex->height-1) * 2;
    int x, z;

    GLfloat* vertexArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
    GLfloat* normalArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
    GLfloat* texCoordArray = malloc(sizeof(GLfloat) * 2 * vertexCount);
    GLuint* indexArray = malloc(sizeof(GLuint) * triangleCount*3);

    printf("bpp %d\n", tex->bpp);
    for (x = 0; x < tex->width; x++) {
        for (z = 0; z < tex->height; z++) {
            // Vertex coordinates
            vertexArray[(x + z * tex->width)*3 + 0] = x * gridSize;
            vertexArray[(x + z * tex->width)*3 + 1] = heightAtVertex(x, z);
            vertexArray[(x + z * tex->width)*3 + 2] = z * gridSize;

            // Texture coordinates
            texCoordArray[(x + z * tex->width)*2 + 0] = x;
            texCoordArray[(x + z * tex->width)*2 + 1] = z;

            // Calculate vertex normals
            GLfloat leftHeight = heightAtVertex(x - 1, z);
            GLfloat rightHeight = heightAtVertex(x + 1, z);
            GLfloat aboveHeight = heightAtVertex(x, z - 1);
            GLfloat belowHeight = heightAtVertex(x, z + 1);

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


Model* terrainModel;
Model* octagon;
Model* groundSphere;

// Reference to shader program
GLuint program, untexturedProgram;
GLuint tex1, tex2;

Model* init_model(const char* modelname, GLuint program)
{
    Model* model = LoadModelPlus(modelname);
    glBindVertexArray(model->vao);
	
    // VBO for vertex positions
    glBindBuffer(GL_ARRAY_BUFFER, model->vb);
    glVertexAttribPointer(glGetAttribLocation(program, "inPosition"), 3, GL_FLOAT, GL_FALSE, 0, 0); 
    glEnableVertexAttribArray(glGetAttribLocation(program, "inPosition"));
    printError("inPosition");

    // VBO for normal vectors
    glBindBuffer(GL_ARRAY_BUFFER, model->nb);
    glVertexAttribPointer(glGetAttribLocation(program, "inNormal"), 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(glGetAttribLocation(program, "inNormal"));
    printError("inNormal");

    if (model->texCoordArray != NULL)
    {
        glBindBuffer(GL_ARRAY_BUFFER, model->tb);
        glVertexAttribPointer(glGetAttribLocation(program, "inTexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(glGetAttribLocation(program, "inTexCoord"));
    }
    printError("inTexCoord");
    return model;
}

void init()
{
    // GL inits
    glClearColor(0.8, 0.8, 1.0, 1.0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    printError("GL inits");

    projectionMatrix = frustum(-0.1, 0.1, -0.1, 0.1, 0.2, 50.0);

    // Load and compile shader
    program = loadShaders("terrain_diffuse.vert", "terrain_diffuse.frag");
    printError("init shader");

    // Load untextured shader
    untexturedProgram = loadShaders("untextured.vert", "untextured.frag");
    printError("init shader");

    glUniform1i(glGetUniformLocation(program, "tex"), 0); // Texture unit 0
    LoadTGATextureSimple("grass.tga", &tex1);

    // Load terrain data
    LoadTGATextureData("fft-terrain.tga", &terrainTexture);
    terrainModel = GenerateTerrain(&terrainTexture);
    printError("init terrain");

    // Load other models
    octagon = init_model("octagon.obj", program);
    groundSphere = init_model("groundsphere.obj", program);
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

    float cameraSpeed = 0.02;
    if (glutKeyIsDown(GLUT_KEY_LEFT_SHIFT)) {
        cameraSpeed *= 2;
    }
    dx *= cameraSpeed;
    dy *= cameraSpeed;

    cameraX += dx * cos(cameraYaw);
    cameraZ += dx * sin(cameraYaw);

    cameraX += dy * sin(cameraYaw);
    cameraZ += dy * cos(cameraYaw) * -1;

    cameraY = heightAtPoint(cameraX / gridSize, cameraZ / gridSize) + 0.4;
}

void display()
{
    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    printError("pre display");

    updateCamera();

    mat4 cameraOffset = T(-cameraX, -cameraY, -cameraZ);
    mat4 cameraRotation = Mult(Rx(cameraPitch), Ry(cameraYaw));
    mat4 camMatrix = Mult(cameraRotation, cameraOffset);

    mat4 total = Mult(projectionMatrix, camMatrix);

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "worldMatrix"), 1, GL_TRUE, IdentityMatrix().m);
    glUniformMatrix4fv(glGetUniformLocation(program, "totalMatrix"), 1, GL_TRUE, total.m);

    glBindTexture(GL_TEXTURE_2D, tex1);     // Bind Our Texture tex1
    DrawModel(terrainModel, program, "inPosition", "inNormal", "inTexCoord");

    glUseProgram(untexturedProgram);

    GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    GLfloat octagonZ = sin(t) * 5 + 5;
    mat4 octagonWorld = Mult(T(5, heightAtPoint(5 / gridSize, octagonZ / gridSize), octagonZ),
                             S(0.2, 0.2, 0.2));
    glBindVertexArray(octagon->vao);
    glUniform3f(glGetUniformLocation(untexturedProgram, "inColor"), 1.0, 0.0, 0.0);
    glUniformMatrix4fv(glGetUniformLocation(untexturedProgram, "worldMatrix"), 1, GL_TRUE, octagonWorld.m);
    glUniformMatrix4fv(glGetUniformLocation(untexturedProgram, "totalMatrix"), 1, GL_TRUE, Mult(total, octagonWorld).m);
    glDrawElements(GL_TRIANGLES, octagon->numIndices, GL_UNSIGNED_INT, 0);

    mat4 sphereWorld = Mult(T(5, heightAtPoint(5 / gridSize, 5 / gridSize), 5), S(0.2, 0.2, 0.2));
    glBindVertexArray(groundSphere->vao);
    glUniform3f(glGetUniformLocation(untexturedProgram, "inColor"), 0.0, 1.0, 0.0);
    glUniformMatrix4fv(glGetUniformLocation(untexturedProgram, "worldMatrix"), 1, GL_TRUE, sphereWorld.m);
    glUniformMatrix4fv(glGetUniformLocation(untexturedProgram, "totalMatrix"), 1, GL_TRUE, Mult(total, sphereWorld).m);
    glDrawElements(GL_TRIANGLES, groundSphere->numIndices, GL_UNSIGNED_INT, 0);

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
