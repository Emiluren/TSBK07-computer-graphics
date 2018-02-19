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

#include <stdio.h>

mat4 projectionMatrix;

GLfloat cameraX = 7;
GLfloat cameraY = 0;
GLfloat cameraZ = 4;

GLfloat cameraYaw = M_PI;
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

vec3 normalAtVertex(int x, int z, int width, GLfloat* normalArray) {
    return SetVector(normalArray[(x + z * width)*3 + 0],
                     normalArray[(x + z * width)*3 + 1],
                     normalArray[(x + z * width)*3 + 2]);
}

vec3 normalAtPoint(GLfloat x, GLfloat z, int width, GLfloat* normalArray) {
    if (x < 0 || x >= terrainTexture.width)
        return SetVector(0, 1, 0);
    if (z < 0 || z >= terrainTexture.height)
        return SetVector(0, 1, 0);

    int xInt = (int)x;
    GLfloat xFrac = x - xInt;
    int zInt = (int)z;
    GLfloat zFrac = z - zInt;

    float tlFac, trFac, blFac, brFac;
    tlFac = (1 - xFrac) * (1 - zFrac);
    trFac = xFrac * (1 - zFrac);
    blFac = (1 - xFrac) * zFrac;
    brFac = xFrac * zFrac;

    vec3 tl, tr, bl, br;
    tl = ScalarMult(normalAtVertex(xInt, zInt, width, normalArray), tlFac);
    tr = ScalarMult(normalAtVertex(xInt + 1, zInt, width, normalArray), trFac);
    bl = ScalarMult(normalAtVertex(xInt, zInt + 1, width, normalArray), blFac);
    br = ScalarMult(normalAtVertex(xInt + 1, zInt + 1, width, normalArray), brFac);

    vec3 interpolatedNormal = VectorAdd(VectorAdd(tl, tr), VectorAdd(bl, br));
    return Normalize(interpolatedNormal);
}

mat4 rotationFromNormal(vec3 normal) {
    GLfloat xrot = atan2(normal.z, normal.y);
    GLfloat zrot = atan2(-normal.x, normal.y);
    return Mult(Rx(xrot), Rz(zrot));
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

/*
Model* GenerateWater(TextureData* terrainTexture) {
    int vertexCount = tex->width * tex->height;
    int triangleCount = (tex->width-1) * (tex->height-1) * 2;

    GLfloat* vertexArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
    GLfloat* normalArray = malloc(sizeof(GLfloat) * 3 * vertexCount);
    GLfloat* texCoordArray = malloc(sizeof(GLfloat) * 2 * vertexCount);
    GLuint* indexArray = malloc(sizeof(GLuint) * triangleCount*3);

    for (int x = 0; x < tex->width; x++) {
        for (int z = 0; z < tex->height; z++) {
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

    Model* model = LoadDataToModel(
        vertexArray,
        normalArray,
        texCoordArray,
        NULL,
        indexArray,
        vertexCount,
        triangleCount*3
    );
}
*/

Model* terrainModel = NULL;
Model* octagon = NULL;
Model* groundSphere = NULL;
Model* skyboxModel = NULL;
Model* personModel = NULL;

// Reference to shader program
GLuint program, untexturedProgram, skyboxProgram;
GLuint grassTexture, soilTexture, skyboxTex;

Model* init_model(const char* modelname, GLuint program)
{
    glUseProgram(program);
    Model* model = LoadModelPlus(modelname);
    glBindVertexArray(model->vao);
	
    // VBO for vertex positions
    GLint positionLocation = glGetAttribLocation(program, "inPosition");
    if (positionLocation != -1) {
        glBindBuffer(GL_ARRAY_BUFFER, model->vb);
        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0); 
        glEnableVertexAttribArray(positionLocation);
    }
    printError("inPosition");

    // VBO for normal vectors
    GLint normalLocation = glGetAttribLocation(program, "inNormal");
    if (normalLocation != -1) {
        glBindBuffer(GL_ARRAY_BUFFER, model->nb);
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normalLocation);
    }
    printError("inNormal");

    GLint textureCoordsLocation = glGetAttribLocation(program, "inTexCoord");
    if (model->texCoordArray != NULL)
    {
        glBindBuffer(GL_ARRAY_BUFFER, model->tb);
        glVertexAttribPointer(textureCoordsLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(textureCoordsLocation);
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

    // Load and compile shaders
    program = loadShaders("terrain_diffuse.vert", "terrain_diffuse.frag");
    printError("init terrain shader");
    untexturedProgram = loadShaders("untextured.vert", "untextured.frag");
    printError("init untextured shader");
    skyboxProgram = loadShaders("skybox.vert", "skybox.frag");
    printError("init skybox shader");

    LoadTGATextureSimple("grass.tga", &grassTexture);
    LoadTGATextureSimple("soil.tga", &soilTexture);
    LoadTGATextureSimple("SkyBox512.tga", &skyboxTex);

    // Load terrain data
    LoadTGATextureData("fft-terrain.tga", &terrainTexture);
    terrainModel = GenerateTerrain(&terrainTexture);
    printError("init terrain");

    // Load other models
    octagon = init_model("octagon.obj", untexturedProgram);
    groundSphere = init_model("groundsphere.obj", untexturedProgram);
    skyboxModel = init_model("skybox.obj", skyboxProgram);
    personModel = init_model("person.obj", untexturedProgram);
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

GLint findUniform(GLuint program, const char* name) {
    GLint location = glGetUniformLocation(program, name);
    if (location == -1) {
        printf("Could not find uniform %s in program %u", name, program);
    }
    return location;
}

void renderPerson(GLfloat x, GLfloat z, mat4 total) {
    GLfloat y = heightAtPoint(x / gridSize, z / gridSize);
    vec3 groundNormal = normalAtPoint(x / gridSize, z / gridSize, terrainTexture.width, terrainModel->normalArray);
    mat4 scale = S(0.1, 0.1, 0.1);
    mat4 trans = T(x, y, z);
    mat4 rot = rotationFromNormal(groundNormal);
    mat4 world = Mult(trans, Mult(rot, scale));
    glBindVertexArray(personModel->vao);
    glUniform3f(findUniform(untexturedProgram, "inColor"), 0.7, 0.7, 0.7);
    glUniformMatrix4fv(findUniform(untexturedProgram, "worldMatrix"), 1, GL_TRUE, world.m);
    glUniformMatrix4fv(findUniform(untexturedProgram, "totalMatrix"), 1, GL_TRUE, Mult(total, world).m);
    glDrawElements(GL_TRIANGLES, personModel->numIndices, GL_UNSIGNED_INT, 0);
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

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(skyboxProgram);
    glBindVertexArray(skyboxModel->vao);
    glUniformMatrix4fv(findUniform(skyboxProgram, "cameraRotation"), 1, GL_TRUE, Mult(projectionMatrix, cameraRotation).m);
    glUniform1i(findUniform(skyboxProgram, "texUnit"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyboxTex);
    glDrawElements(GL_TRIANGLES, skyboxModel->numIndices, GL_UNSIGNED_INT, 0);
    printError("skybox rendering");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    glUseProgram(program);
    glUniformMatrix4fv(findUniform(program, "worldMatrix"), 1, GL_TRUE, IdentityMatrix().m);
    glUniformMatrix4fv(findUniform(program, "totalMatrix"), 1, GL_TRUE, total.m);

    glUniform1i(findUniform(program, "tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    DrawModel(terrainModel, program, "inPosition", "inNormal", "inTexCoord");

    glUseProgram(untexturedProgram);

    GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    GLfloat octagonX = 5;
    GLfloat octagonZ = sin(t / 2.0) * 5 + 5;
    GLfloat octagonY = heightAtPoint(octagonX / gridSize, octagonZ / gridSize);
    vec3 normalUnderOctagon = normalAtPoint(octagonX / gridSize, octagonZ / gridSize, terrainTexture.width, terrainModel->normalArray);
    mat4 octagonWorld = Mult(T(octagonX, octagonY, octagonZ), S(0.2, 0.2, 0.2));

    glBindVertexArray(octagon->vao);
    glUniform3f(findUniform(untexturedProgram, "inColor"), 1.0, 0.0, 0.0);
    glUniformMatrix4fv(findUniform(untexturedProgram, "worldMatrix"), 1, GL_TRUE, octagonWorld.m);
    glUniformMatrix4fv(findUniform(untexturedProgram, "totalMatrix"), 1, GL_TRUE, Mult(total, octagonWorld).m);
    glDrawElements(GL_TRIANGLES, octagon->numIndices, GL_UNSIGNED_INT, 0);

    mat4 sphereWorld = Mult(T(5, heightAtPoint(5 / gridSize, 5 / gridSize), 5), S(0.2, 0.2, 0.2));
    glBindVertexArray(groundSphere->vao);
    glUniform3f(findUniform(untexturedProgram, "inColor"), 0.0, 1.0, 0.0);
    glUniformMatrix4fv(findUniform(untexturedProgram, "worldMatrix"), 1, GL_TRUE, sphereWorld.m);
    glUniformMatrix4fv(findUniform(untexturedProgram, "totalMatrix"), 1, GL_TRUE, Mult(total, sphereWorld).m);
    glDrawElements(GL_TRIANGLES, groundSphere->numIndices, GL_UNSIGNED_INT, 0);

    renderPerson(7, 5 + 5 * sin(t / 5), total);

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
