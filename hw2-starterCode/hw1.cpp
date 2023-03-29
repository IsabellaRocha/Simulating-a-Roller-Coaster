/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++ starter code

  Student username: irocha
*/

#include "basicPipelineProgram.h"
#include "texturePipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glm/gtc/type_ptr.hpp>

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

//For animation
bool isMoving = true;
int numScreenshots = 0;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f };
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

// Stores the image loaded from disk.
//ImageIO * heightmapImage;

//START OF HW2 CODE
// represents one control point along the spline
struct Point
{
    double x;
    double y;
    double z;
};

// spline struct
// contains how many control points the spline has, and an array of control points
struct Spline
{
    int numControlPoints;
    Point* points;
};

// the spline array
Spline* splines;
// total number of splines
int numSplines;

GLuint splineVAO, railVAO, groundVAO;
GLuint splineVBO, railVBO, groundVBO;
GLuint groundTexture;

vector<float> splineColors;
vector<glm::vec3> splineCoordinates, tangentCoordinates, normals, binormals;
vector<glm::vec3> railCoordinates, railNormals;
vector<glm::vec3> groundCoordinates;
vector<glm::vec2> groundTextureCoordinates;
vector<glm::vec4> railColors, groundColors;

const float s = 0.5;

const float basisMatrix[16] = { -s, 2.0 - s, s - 2.0, s,
                                2.0 * s, s - 3.0, 3.0 - 2.0 * s, -s,
                                -s, 0.0, s, 0.0,
                                0.0, 1.0, 0.0, 0.0 };

int camPos = 0;
bool forwards = false;


// CSCI 420 helper classes.
OpenGLMatrix matrix;
BasicPipelineProgram* pipelineProgram;
// Second pipeline program for texture floor
TexturePipelineProgram* texturePipelineProgram;

// Write a screenshot to the specified filename.
void saveScreenshot(const char* filename)
{
    unsigned char* screenshotData = new unsigned char[windowWidth * windowHeight * 3];
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

    ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

    if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
        cout << "File " << filename << " saved successfully." << endl;
    else cout << "Failed to save file " << filename << '.' << endl;

    delete[] screenshotData;
}

/*MULTIPLY MATRICES HELPERS*/
//Used to multiply u vector by basis matrix in loadSplineVertices()
void multiply1x4by4x4Matrices(float* result, const float* matrix1, const float* matrix2)
{
    // Perform the multiplication of matrix1 and matrix2
    float x = 0, y = 0, z = 0, w = 0;
    for (int col = 0; col < 4; col++) {
        float m1 = matrix1[col];
        x += m1 * matrix2[col * 4 + 0];
        y += m1 * matrix2[col * 4 + 1];
        z += m1 * matrix2[col * 4 + 2];
        w += m1 * matrix2[col * 4 + 3];
    }
    result[0] = x;
    result[1] = y;
    result[2] = z;
    result[3] = w;
}

//Used to multiply result of u vector and basis matrix by control matrix in loadSplineVertices()
void multiply1x4by4x3Matrices(float* result, const float* matrix1, const float* matrix2)
{
    float x = 0, y = 0, z = 0;
    for (int row = 0; row < 4; row++) {
        float m1 = matrix1[row];
        x += m1 * matrix2[row * 3 + 0];
        y += m1 * matrix2[row * 3 + 1];
        z += m1 * matrix2[row * 3 + 2];
    }
    result[0] = x;
    result[1] = y;
    result[2] = z;
}

//Used in displayfunc for calculating viewLightDirection
void multiply4x4by4x1Matrices(float* result, const float* matrix1, const float* matrix2)
{
    // Perform the multiplication of matrix1 and matrix2
    for (int i = 0; i < 4; i++) {
        result[i] = matrix1[i * 4 + 0] * matrix2[0] +
            matrix1[i * 4 + 1] * matrix2[1] +
            matrix1[i * 4 + 2] * matrix2[2] +
            matrix1[i * 4 + 3] * matrix2[3];
    }
}

void displayFunc()
{
    // This function performs the actual rendering.

    // First, clear the screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the camera position, focus point, and the up vector.
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.LoadIdentity();
    matrix.LookAt(splineCoordinates[camPos].x + normals[camPos].x, splineCoordinates[camPos].y + normals[camPos].y, splineCoordinates[camPos].z + normals[camPos].z,
        splineCoordinates[camPos].x + tangentCoordinates[camPos].x, splineCoordinates[camPos].y + tangentCoordinates[camPos].y, splineCoordinates[camPos].z + tangentCoordinates[camPos].z,
        normals[camPos].x, normals[camPos].y, normals[camPos].z);

    // Read the current modelview and projection matrices.
    float modelViewMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetMatrix(modelViewMatrix);

    float projectionMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.GetMatrix(projectionMatrix);

    
    // Bind the pipeline program.
    // If an incorrect pipeline program is bound, then the modelview and projection matrices
    // will be sent to the wrong pipeline program, causing shader
    // malfunction (typically, nothing will be shown on the screen).
    // In this homework, there is only one pipeline program, and it is already bound.
    // So technically speaking, this call is redundant in hw1.
    // However, in more complex programs (such as hw2), there will be more than one
    // pipeline program. And so we will need to bind the pipeline program that we want to use.
    pipelineProgram->Bind(); // This call is redundant in hw1, but it is good to keep for consistency.

    // Upload the modelview and projection matrices to the GPU.
    pipelineProgram->SetModelViewMatrix(modelViewMatrix);
    pipelineProgram->SetProjectionMatrix(projectionMatrix);

    texturePipelineProgram->Bind();
    // Upload the modelview and projection matrices to the GPU.
    texturePipelineProgram->SetModelViewMatrix(modelViewMatrix);
    texturePipelineProgram->SetProjectionMatrix(projectionMatrix);


    pipelineProgram->Bind();
    /*PHONG LIGHTING*/
    //Handle to viewLightDirection shader variable
    GLint h_viewLightDirection = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "viewLightDirection");
    //Direction towards light
    float homogenousLightDirection[4] = { 0, 1, 0, 0 };
    vector<float> viewLightDirection;
    float result[4];
    multiply4x4by4x1Matrices(result, modelViewMatrix, homogenousLightDirection);
    viewLightDirection.push_back(result[0]);
    viewLightDirection.push_back(result[1]);
    viewLightDirection.push_back(result[2]);
    //Upload viewLightDirection to the GPU
    glUniform3fv(h_viewLightDirection, 1, &viewLightDirection[0]);
    //Phong Constants
    float La[4] = { 0.5f, 0.5f, 0.5f, 0.0f };
    float ka[4] = { 0.3f, 0.3f, 0.3f, 0.0f };
    GLint h_La = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "La");
    glUniform4fv(h_La, 1, La);
    GLint h_ka = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ka");
    glUniform4fv(h_ka, 1, ka);

    float Ld[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
    float kd[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
    GLint h_Ld = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ld");
    glUniform4fv(h_Ld, 1, Ld);
    GLint h_kd = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "kd");
    glUniform4fv(h_kd, 1, kd);

    float Ls[4] = { 0.4f, 0.4f, 0.4f, 0.0f };
    float ks[4] = { 0.2f, 0.2f, 0.2f, 0.0f };
    GLint h_Ls = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ls");
    glUniform4fv(h_Ls, 1, Ls);
    GLint h_ks = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ks");
    glUniform4fv(h_ks, 1, ks);

    GLint h_alpha = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "alpha");
    glUniform1f(h_alpha, 0.9f);


    /*UPLOADING NORMAL MATRIX*/
    GLint h_normalMatrix = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "normalMatrix");

    float n[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetNormalMatrix(n);
    // upload n to the GPU
    GLboolean isRowMajor = GL_FALSE;
    glUniformMatrix4fv(h_normalMatrix, 1, isRowMajor, n);

    glBindVertexArray(railVAO);
    glDrawArrays(GL_TRIANGLES, 0, railCoordinates.size());

    texturePipelineProgram->Bind();
    glBindVertexArray(groundVAO);
    glDrawArrays(GL_TRIANGLES, 0, groundCoordinates.size());

    glBindVertexArray(0);

    // Swap the double-buffers.
    glutSwapBuffers();
}

void idleFunc()
{
    if (isMoving) {
        if (camPos < splineCoordinates.size() - 1) camPos++;
        else camPos = 0;
    }

    glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
    glViewport(0, 0, w, h);

    // When the window has been resized, we need to re-set our projection matrix.
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.LoadIdentity();
    // You need to be careful about setting the zNear and zFar.
    // Anything closer than zNear, or further than zFar, will be culled.
    const float zNear = 0.1f;
    const float zFar = 10000.0f;
    const float humanFieldOfView = 60.0f;
    matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
    // Mouse has moved, and one of the mouse buttons is pressed (dragging).

    // the change in mouse position since the last invocation of this function
    int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

    switch (controlState)
    {
        // translate the terrain
    case TRANSLATE:
        if (leftMouseButton)
        {
            // control x,y translation via the left mouse button
            terrainTranslate[0] += mousePosDelta[0] * 0.01f;
            terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
        }
        if (middleMouseButton)
        {
            // control z translation via the middle mouse button
            terrainTranslate[2] += mousePosDelta[1] * 0.01f;
        }
        break;

        // rotate the terrain
    case ROTATE:
        if (leftMouseButton)
        {
            // control x,y rotation via the left mouse button
            terrainRotate[0] += mousePosDelta[1];
            terrainRotate[1] += mousePosDelta[0];
        }
        if (middleMouseButton)
        {
            // control z rotation via the middle mouse button
            terrainRotate[2] += mousePosDelta[1];
        }
        break;

        // scale the terrain
    case SCALE:
        if (leftMouseButton)
        {
            // control x,y scaling via the left mouse button
            terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
            terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
        }
        if (middleMouseButton)
        {
            // control z scaling via the middle mouse button
            terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
        }
        break;
    }

    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
    // Mouse has moved.
    // Store the new mouse position.
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
    // A mouse button has has been pressed or depressed.

    // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
    switch (button)
    {
    case GLUT_LEFT_BUTTON:
        leftMouseButton = (state == GLUT_DOWN);
        break;

    case GLUT_MIDDLE_BUTTON:
        middleMouseButton = (state == GLUT_DOWN);
        break;

    case GLUT_RIGHT_BUTTON:
        rightMouseButton = (state == GLUT_DOWN);
        break;
    }

    // Keep track of whether CTRL and SHIFT keys are pressed.
    switch (glutGetModifiers())
    {
    case GLUT_ACTIVE_CTRL:
        controlState = TRANSLATE;
        break;

    case GLUT_ACTIVE_SHIFT:
        controlState = SCALE;
        break;

        // If CTRL and SHIFT are not pressed, we are in rotate mode.
    default:
        controlState = ROTATE;
        break;
    }

    // Store the new mouse position.
    mousePos[0] = x;
    mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
    GLint mode = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode"); //Use to change between first mode for 1, 2, 3, and second mode for 4
    GLint constant = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "constant");
    switch (key)
    {
    case 27: // ESC key
        exit(0); // exit the program
        break;

    case ' ':
        cout << "You pressed the spacebar." << endl;
        break;

    case 'x':
        // Take a screenshot.
        saveScreenshot("screenshot.jpg");
        break;

    case 'm':
        isMoving = !isMoving;
        break;
    }
}




int loadSplines(char* argv)
{
    char* cName = (char*)malloc(128 * sizeof(char));
    FILE* fileList;
    FILE* fileSpline;
    int iType, i = 0, j, iLength;

    // load the track file
    fileList = fopen(argv, "r");
    if (fileList == NULL)
    {
        printf("can't open file\n");
        exit(1);
    }

    // stores the number of splines in a global variable
    fscanf(fileList, "%d", &numSplines);

    splines = (Spline*)malloc(numSplines * sizeof(Spline));

    // reads through the spline files
    for (j = 0; j < numSplines; j++)
    {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");

        if (fileSpline == NULL)
        {
            printf("can't open file\n");
            exit(1);
        }

        // gets length for spline file
        fscanf(fileSpline, "%d %d", &iLength, &iType);

        // allocate memory for all the points
        splines[j].points = (Point*)malloc(iLength * sizeof(Point));
        splines[j].numControlPoints = iLength;

        // saves the data to the struct
        while (fscanf(fileSpline, "%lf %lf %lf",
            &splines[j].points[i].x,
            &splines[j].points[i].y,
            &splines[j].points[i].z) != EOF)
        {
            i++;
        }

    }
    free(cName);

    return 0;
}

int initTexture(const char* imageFilename, GLuint textureHandle)
{
    // read the texture image
    ImageIO img;
    ImageIO::fileFormatType imgFormat;
    ImageIO::errorType err = img.load(imageFilename, &imgFormat);

    if (err != ImageIO::OK)
    {
        printf("Loading texture from %s failed.\n", imageFilename);
        return -1;
    }

    // check that the number of bytes is a multiple of 4
    if (img.getWidth() * img.getBytesPerPixel() % 4)
    {
        printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
        return -1;
    }

    // allocate space for an array of pixels
    int width = img.getWidth();
    int height = img.getHeight();
    unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

    // fill the pixelsRGBA array with the image pixels
    memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
    for (int h = 0; h < height; h++)
        for (int w = 0; w < width; w++)
        {
            // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
            pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
            pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
            pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
            pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

            // set the RGBA channels, based on the loaded image
            int numChannels = img.getBytesPerPixel();
            for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
                pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
        }

    // bind the texture
    glBindTexture(GL_TEXTURE_2D, textureHandle);

    // initialize the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

    // generate the mipmaps for this texture
    glGenerateMipmap(GL_TEXTURE_2D);

    // set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // query support for anisotropic texture filtering
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    printf("Max available anisotropic samples: %f\n", fLargest);
    // set anisotropic texture filtering
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

    // query for any errors
    GLenum errCode = glGetError();
    if (errCode != 0)
    {
        printf("Texture initialization error. Error code: %d.\n", errCode);
        return -1;
    }

    // de-allocate the pixel array -- it is no longer needed
    delete[] pixelsRGBA;

    return 0;
}


void loadVerticesSpline() {

    for (int i = 0; i < numSplines; i++) {
        //Control matrix takes four points, hence -3
        for (int j = 1; j < splines[i].numControlPoints - 2; j++) {
            //control matrix
            float controlMatrix[12] = { splines[i].points[j - 1].x, splines[i].points[j - 1].y, splines[i].points[j - 1].z,
                                        splines[i].points[j].x, splines[i].points[j].y, splines[i].points[j].z,
                                        splines[i].points[j + 1].x, splines[i].points[j + 1].y, splines[i].points[j + 1].z,
                                        splines[i].points[j + 2].x, splines[i].points[j + 2].y, splines[i].points[j + 2].z };

            for (float u = 0.0; u <= 1.0; u += 0.001) {
                float initialMatrix[4];
                float rowVector[4] = { pow(u, 3), pow(u, 2), u, 1 };
                multiply1x4by4x4Matrices(initialMatrix, rowVector, basisMatrix);

                float result[3];
                multiply1x4by4x3Matrices(result, initialMatrix, controlMatrix);
                glm::vec3 finalSplineCoor = glm::make_vec3(result);
                splineCoordinates.push_back(finalSplineCoor);
                /*
                splineColors.push_back(1.0);
                splineColors.push_back(1.0);
                splineColors.push_back(1.0);
                splineColors.push_back(1.0);
                */
                float initialTangentMatrix[4];
                float tangentVector[4] = { 3 * pow(u, 2), 2 * u, 1, 0 };
                multiply1x4by4x4Matrices(initialTangentMatrix, tangentVector, basisMatrix);

                float tangent[3];
                multiply1x4by4x3Matrices(tangent, initialTangentMatrix, controlMatrix);
                glm::vec3 tan = glm::make_vec3(tangent);
                tangentCoordinates.push_back(glm::normalize(tan));
            }
        }
    }
}

//Using Sloan's method: https://viterbi-web.usc.edu/~jbarbic/cs420-s23/assign2/assign2_camera.html
void loadNormalsAndBinormals() {
    //Starting coordinates
    //Arbitrary vector (typical y up vector) to start
    glm::vec3 V = glm::vec3(0.0, 1.0, 0.0);
    //N0 = unit(T0 x V)
    glm::vec3 N0 = glm::cross(tangentCoordinates[0], V);
    glm::vec3 N = glm::normalize(N0);
    //B0 = unit(T0 x N0)
    glm::vec3 B0 = glm::cross(tangentCoordinates[0], N);
    glm::vec3 B = glm::normalize(B0);
    normals.push_back(N);
    binormals.push_back(B);

    for (int i = 1; i < tangentCoordinates.size(); i++) {
        //N1 = unit(B0 x T1)
        N0 = glm::cross(binormals[i - 1], tangentCoordinates[i]);
        N = glm::normalize(N0);
        //B1 = unit(T1 x N1)
        B0 = glm::cross(tangentCoordinates[i], N);
        B = glm::normalize(B0);
        normals.push_back(N);
        binormals.push_back(B);
    }
}

//Makes rail into continuous rectangle not just a line
//triangleColor used in level 3 to make it rainbow, no longer used now
//triangleNormal now used for Phong lighting
void loadRailCoordinates() {
    for (int i = 0; i < splineCoordinates.size() - 1; i++) {
        glm::vec3 currCoor = splineCoordinates[i];
        glm::vec3 currTan = tangentCoordinates[i];
        glm::vec3 currNormal = normals[i];
        glm::vec3 currBinormal = binormals[i];

        glm::vec3 nextCoor = splineCoordinates[i + 1];
        glm::vec3 nextTan = tangentCoordinates[i + 1];
        glm::vec3 nextNormal = normals[i + 1];
        glm::vec3 nextBinormal = binormals[i + 1];

        glm::vec3 v0 = currCoor + 0.1f * (-currNormal + currBinormal);
        glm::vec3 v1 = currCoor + 0.1f * (currNormal + currBinormal);
        glm::vec3 v2 = currCoor + 0.1f * (currNormal - currBinormal);
        glm::vec3 v3 = currCoor + 0.1f * (-currNormal - currBinormal);

        glm::vec3 v4 = nextCoor + 0.1f * (-nextNormal + nextBinormal);
        glm::vec3 v5 = nextCoor + 0.1f * (nextNormal + nextBinormal);
        glm::vec3 v6 = nextCoor + 0.1f * (nextNormal - nextBinormal);
        glm::vec3 v7 = nextCoor + 0.1f * (-nextNormal - nextBinormal);

        //First face
        //Calculate triangle normal for color (lvl 3)
        glm::vec3 a = v1 - v0;
        glm::vec3 b = v2 - v0;
        glm::vec3 triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        glm::vec4 triangleColor = glm::make_vec4(triangleColorArray);
        railCoordinates.push_back(v0);
        railNormals.push_back(triangleNormal);
        railCoordinates.push_back(v1);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v2);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        a = v3 - v2;
        b = v0 - v2;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray1[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray1);
        railCoordinates.push_back(v2);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v3);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v0);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        //Second face
        a = v5 - v4;
        b = v6 - v4;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray2[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray2);
        railCoordinates.push_back(v4);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v5);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v6);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        a = v7 - v6;
        b = v4 - v6;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray3[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray3);
        railCoordinates.push_back(v6);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v7);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v4);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        //Connect faces on left side
        a = v6 - v7;
        b = v2 - v7;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray4[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray4);
        railCoordinates.push_back(v7);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v6);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v2);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        a = v3 - v2;
        b = v7 - v2;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray5[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray5);
        railCoordinates.push_back(v2);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v3);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v7);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        //Connect faces on right side
        a = v5 - v4;
        b = v1 - v4;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray6[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray6);
        railCoordinates.push_back(v4);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v5);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v1);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        a = v0 - v1;
        b = v4 - v1;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray7[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray7);
        railCoordinates.push_back(v1);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v0);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v4);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        //Connect faces on top
        a = v5 - v1;
        b = v6 - v1;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray8[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray8);
        railCoordinates.push_back(v1);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v5);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v6);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        a = v2 - v6;
        b = v1 - v6;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray9[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray9);
        railCoordinates.push_back(v6);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v2);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v1);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        //Connect faces on bottom
        a = v4 - v0;
        b = v7 - v0;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray10[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray10);
        railCoordinates.push_back(v0);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v4);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v7);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);

        a = v3 - v7;
        b = v0 - v7;
        triangleNormal = glm::normalize(glm::cross(a, b));
        float triangleColorArray11[4] = { triangleNormal.x, triangleNormal.y, triangleNormal.z, 1 };
        triangleColor = glm::make_vec4(triangleColorArray11);
        railCoordinates.push_back(v7);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v3);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
        railCoordinates.push_back(v0);
        railNormals.push_back(triangleNormal);
        railColors.push_back(triangleColor);
    }
}

void loadGroundCoordinates() {
    groundCoordinates.push_back(glm::vec3(256.0, -256.0, -300.0));
    groundCoordinates.push_back(glm::vec3(256.0, 256.0, -300.0));
    groundCoordinates.push_back(glm::vec3(-256.0, 256.0, -300.0));
    groundCoordinates.push_back(glm::vec3(-256.0, 256.0, -300.0));
    groundCoordinates.push_back(glm::vec3(-256.0, -256.0, -300.0));
    groundCoordinates.push_back(glm::vec3(256.0, -256.0, -300.0));

    groundTextureCoordinates.push_back(glm::vec2(0, 0));
    groundTextureCoordinates.push_back(glm::vec2(1, 0));
    groundTextureCoordinates.push_back(glm::vec2(1, 1));
    groundTextureCoordinates.push_back(glm::vec2(1, 1));
    groundTextureCoordinates.push_back(glm::vec2(0, 1));
    groundTextureCoordinates.push_back(glm::vec2(0, 0));
}

void initScene(int argc, char* argv[])
{
    // Set the background color.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.
    // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
    glEnable(GL_DEPTH_TEST);
    loadVerticesSpline();
    loadNormalsAndBinormals();
    loadRailCoordinates();
    loadGroundCoordinates();

    // Create and bind the pipeline program. This operation must be performed BEFORE we initialize any VAOs.
    pipelineProgram = new BasicPipelineProgram;
    int ret = pipelineProgram->Init(shaderBasePath);
    if (ret != 0)
    {
        abort();
    }
    pipelineProgram->Bind();

    const GLuint locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".

    const int stride = 0; // Stride is 0, i.e., data is tightly packed in the VBO.
    const GLboolean normalized = GL_FALSE; // Normalization is off.
    const GLuint locationOfNormal = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal"); // Obtain a handle to the shader variable "normal".

    /*RAIL VBOS AND VAOS*/
    // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
    glGenBuffers(1, &railVBO);
    glBindBuffer(GL_ARRAY_BUFFER, railVBO);
    // First, allocate an empty VBO of the correct size to hold positions and colors.
    int numBytesRailCoordinates = sizeof(glm::vec3) * railCoordinates.size();
    int numBytesNormals = sizeof(glm::vec3) * railNormals.size();
    glBufferData(GL_ARRAY_BUFFER, numBytesRailCoordinates + numBytesNormals, nullptr, GL_STATIC_DRAW);
    // Next, write the position and color data into the VBO.
    glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesRailCoordinates, &railCoordinates[0]);
    glBufferSubData(GL_ARRAY_BUFFER, numBytesRailCoordinates, numBytesNormals, &railNormals[0]);
    // Create the VAOs. There is a single VAO in this example.
    glGenVertexArrays(1, &railVAO);
    glBindVertexArray(railVAO);
    glBindBuffer(GL_ARRAY_BUFFER, railVBO);

    // Set up the relationship between the "position" shader variable and the VAO.
    glEnableVertexAttribArray(locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
    glVertexAttribPointer(locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates).

    // Set up the relationship between the "normal" shader variable and the VAO.
    glEnableVertexAttribArray(locationOfNormal); // Must always enable the vertex attribute. By default, it is disabled.
    glVertexAttribPointer(locationOfNormal, 3, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesRailCoordinates); // The shader variable "normal" receives its data from the currently bound VBO 

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create and bind the pipeline program. This operation must be performed BEFORE we initialize any VAOs.
    texturePipelineProgram = new TexturePipelineProgram;
    ret = texturePipelineProgram->Init(shaderBasePath);
    if (ret != 0)
    {
        abort();
    }
    texturePipelineProgram->Bind();

    const GLuint locationOfTexturePosition = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
    const GLuint locationOfTextureCoors = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord"); // Obtain a handle to the shader variable "position".

    /*GROUND VBOS AND VAOS*/
    // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
    glGenBuffers(1, &groundVBO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    // First, allocate an empty VBO of the correct size to hold positions and colors.
    int numBytesGroundCoordinates = sizeof(glm::vec3) * groundCoordinates.size();
    int numBytesGroundTextureCoordinates = sizeof(glm::vec2) * groundTextureCoordinates.size();
    glBufferData(GL_ARRAY_BUFFER, numBytesGroundCoordinates + numBytesGroundTextureCoordinates, nullptr, GL_STATIC_DRAW);
    // Next, write the position and color data into the VBO.
    glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesGroundCoordinates, &groundCoordinates[0]);
    glBufferSubData(GL_ARRAY_BUFFER, numBytesGroundCoordinates, numBytesGroundTextureCoordinates, &groundTextureCoordinates[0]);
    // Create the VAOs. There is a single VAO in this example.
    glGenVertexArrays(1, &groundVAO);
    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);

    // Set up the relationship between the "position" shader variable and the VAO.
    glEnableVertexAttribArray(locationOfTexturePosition); // Must always enable the vertex attribute. By default, it is disabled.
    glVertexAttribPointer(locationOfTexturePosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates).

    // Set up the relationship between the "color" shader variable and the VAO.
    glEnableVertexAttribArray(locationOfTextureCoors); // Must always enable the vertex attribute. By default, it is disabled.
    glVertexAttribPointer(locationOfTextureCoors, 2, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesGroundCoordinates); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels).

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    // Check for any OpenGL errors.
    std::cout << "GL error: " << glGetError() << std::endl;
}



int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cout << "The arguments are incorrect." << endl;
        cout << "usage: ./hw1 <heightmap file>" << endl;
        exit(EXIT_FAILURE);
    }
    if (argc < 2)
    {
        printf("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }

    cout << "Initializing GLUT..." << endl;
    glutInit(&argc, argv);

    cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(windowTitle);

    cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

#ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
#endif

    // tells glut to use a particular display function to redraw
    glutDisplayFunc(displayFunc);
    // perform animation inside idleFunc
    glutIdleFunc(idleFunc);
    // callback for mouse drags
    glutMotionFunc(mouseMotionDragFunc);
    // callback for idle mouse movement
    glutPassiveMotionFunc(mouseMotionFunc);
    // callback for mouse button changes
    glutMouseFunc(mouseButtonFunc);
    // callback for resizing the window
    glutReshapeFunc(reshapeFunc);
    // callback for pressing the keys on the keyboard
    glutKeyboardFunc(keyboardFunc);

    // init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
        cout << "error: " << glewGetErrorString(result) << endl;
        exit(EXIT_FAILURE);
    }
#endif

    // load the splines from the provided filename
    loadSplines(argv[1]);

    printf("Loaded %d spline(s).\n", numSplines);
    for (int i = 0; i < numSplines; i++)
        printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

    // Perform the initialization.
    initScene(argc, argv);

    glGenTextures(1, &groundTexture);

    initTexture("grounds/snow.jpg", groundTexture);

    // Sink forever into the GLUT loop.
    glutMainLoop();
}
