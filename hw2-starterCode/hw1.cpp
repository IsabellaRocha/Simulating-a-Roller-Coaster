/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++ starter code

  Student username: irocha
*/

#include "basicPipelineProgram.h"
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
bool playAnimation = false;
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

GLuint splineVAO;
GLuint splineVBO;

vector<float> splineColors;
vector<glm::vec3> splineCoordinates, tangentCoordinates, normals, binormals;

const float s = 0.5;
//4x4 matrix, column major
const float basisMatrixTranspose[16] = { -s, 2.0 * s, -s, 0.0,
                                2.0 - s, s - 3.0, 0.0, 1.0,
                                s - 2.0, 3.0 - 2.0 * s, s, 0.0,
                                s, -s, 0.0, 0.0 };

const float basisMatrix[16] = { -s, 2.0 - s, s - 2.0, s,
                                2.0 * s, s - 3.0, 3.0 - 2.0 * s, -s,
                                -s, 0.0, s, 0.0,
                                0.0, 1.0, 0.0, 0.0 };

int camPos = 0;
bool forwards = false;


// CSCI 420 helper classes.
OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

int imageHeight = 0;
int imageWidth = 0;
//Used for color correcting in smoothing mode to avoid white spikes
int scaleConstant = 1;

// Write a screenshot to the specified filename.
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
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

  /*
  matrix.LookAt(1.0 * imageWidth / 2, 1.0 * imageWidth / 1.25, 1.0 * imageWidth / 5,
      1.0 * imageWidth / 2, 0.0, -1.0 * imageWidth / 2,
      0.0, 1.0, 0.0);
     */ 
  // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
  // ...

  matrix.Rotate(terrainRotate[0], 1.0, 0.0, 0.0);
  matrix.Rotate(terrainRotate[1], 0.0, 1.0, 0.0);
  matrix.Rotate(terrainRotate[2], 0.0, 0.0, 1.0);
  matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
  matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);

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

  // Execute the rendering.
  //glBindVertexArray(triangleVAO); // Bind the VAO that we want to render.
  //glDrawArrays(GL_TRIANGLES, 0, numVertices); // Render the VAO, by rendering "numVertices", starting from vertex 0.
  
  glBindVertexArray(splineVAO);
  glDrawArrays(GL_LINE_STRIP, 0, splineCoordinates.size());
  glBindVertexArray(0);
 
  // Swap the double-buffers.
  glutSwapBuffers();
}

void idleFunc()
{
    if (camPos < splineCoordinates.size() - 1) camPos++;
    else camPos = 0;

	// Do some stuff... 

	// For example, here, you can save the screenshots to disk (to make the animation).

	// Send the signal that we should call displayFunc.

	
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

    case 'a':
        playAnimation = true;
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

//Used to multiply u vector by basis matrix
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

//Used to multiply result of u vector and basis matrix by control matrix
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



void loadVerticesSpline() {
    
    for (int i = 0; i < numSplines; i++) {
        //Control matrix takes four points, hence -3
        for (int j = 1; j < splines[i].numControlPoints - 2; j++) {
            //control matrix
            float controlMatrix[12] = { splines[i].points[j - 1].x, splines[i].points[j - 1].y, splines[i].points[j - 1].z,
                                        splines[i].points[j].x, splines[i].points[j].y, splines[i].points[j].z, 
                                        splines[i].points[j + 1].x, splines[i].points[j + 1].y, splines[i].points[j + 1].z, 
                                        splines[i].points[j + 2].x, splines[i].points[j + 2].y, splines[i].points[j + 2].z };
            
            for (float u = 0.0; u < 1.0; u += 0.001) {
                float initialMatrix[4];
                float rowVector[4] = { pow(u, 3), pow(u, 2), u, 1 };
                multiply1x4by4x4Matrices(initialMatrix, rowVector, basisMatrix);

                float result[3];
                multiply1x4by4x3Matrices(result, initialMatrix, controlMatrix);
                glm::vec3 finalSplineCoor = glm::make_vec3(result);
                splineCoordinates.push_back(finalSplineCoor);

                splineColors.push_back(1.0);
                splineColors.push_back(1.0);
                splineColors.push_back(1.0);
                splineColors.push_back(1.0);

                float initialTangentMatrix[4];
                float tangentVector[4] = { 3 * pow(u, 2), 2 * u, 1, 0 };
                multiply1x4by4x4Matrices(initialTangentMatrix, tangentVector, basisMatrix);

                float tangent[3];
                multiply1x4by4x3Matrices(tangent, initialTangentMatrix, controlMatrix);
                glm::vec3 tan = glm::make_vec3(tangent);
                tangentCoordinates.push_back(glm::normalize(tan));

/*
                float uNext = u + 0.001;
                
                float initialMatrixNext[4];
                float rowVectorNext[4] = { pow(uNext, 3), pow(uNext, 2), uNext, 1 };
                multiply1x4by4x4Matrices(initialMatrixNext, rowVectorNext, basisMatrix);

                float resultNext[3];
                multiply1x4by4x3Matrices(resultNext, initialMatrixNext, controlMatrix);
                splineCoordinates.push_back(resultNext[0]);
                splineCoordinates.push_back(resultNext[1]);
                splineCoordinates.push_back(resultNext[2]);

                splineColors.push_back(1.0);
                splineColors.push_back(1.0);
                splineColors.push_back(1.0);
                splineColors.push_back(1.0);
                */
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
    glm::vec3 N = glm::normalize(glm::cross(tangentCoordinates[0], V));
    //B0 = unit(T0 x N0)
    glm::vec3 B = glm::normalize(glm::cross(tangentCoordinates[0], N));
    normals.push_back(N);
    binormals.push_back(B);

    for (int i = 1; i < tangentCoordinates.size(); i++) {
        //N1 = unit(B0 x T1)
        N = glm::normalize(glm::cross(binormals[i - 1], tangentCoordinates[i]));
        //B1 = unit(T1 x N1)
        B = glm::normalize(glm::cross(tangentCoordinates[i], N));
        normals.push_back(N);
        binormals.push_back(B);
    }
}


void initScene(int argc, char *argv[])
{
  // Load the image from a jpeg disk file into main memory.
  /*heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  */
  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.  
  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);
  loadVerticesSpline();
  loadNormalsAndBinormals();

  // Create and bind the pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) 
  { 
    abort();
  }
  pipelineProgram->Bind();
  /*
  for (int idx = 0; idx < splineColors.size(); idx++) {
      cout << splineColors[idx] << ", ";
      if (idx % 4 == 0) {
          cout << endl;
      }
  }
  */


  const GLuint locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".

  const int stride = 0; // Stride is 0, i.e., data is tightly packed in the VBO.
  const GLboolean normalized = GL_FALSE; // Normalization is off.
  const GLuint locationOfColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color"); // Obtain a handle to the shader variable "color".


  //Originally had this in a switch, but it's in the init so switching between things didn't work since the other VAOs and VBOs weren't loaded, must load all of them
  
  /*POINT VBOS AND VAOS*/
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &splineVBO);
  glBindBuffer(GL_ARRAY_BUFFER, splineVBO);
  // First, allocate an empty VBO of the correct size to hold positions and colors.
  int numBytesSplineCoordinates = sizeof(glm::vec3) * splineCoordinates.size();
  int numBytesSplineColors = sizeof(float) * splineColors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesSplineCoordinates + numBytesSplineColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesSplineCoordinates, (float*)splineCoordinates.data());
  glBufferSubData(GL_ARRAY_BUFFER, numBytesSplineCoordinates, numBytesSplineColors, (float*)splineColors.data());
  // Create the VAOs. There is a single VAO in this example.
  glGenVertexArrays(1, &splineVAO);
  glBindVertexArray(splineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, splineVBO);

  // Set up the relationship between the "position" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfColor, 4, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesSplineCoordinates); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  glBindBuffer(GL_ARRAY_BUFFER, 0); //Unbind in order to do lines next



  // Check for any OpenGL errors.
  std::cout << "GL error: " << glGetError() << std::endl;
}



int main(int argc, char *argv[])
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
  glutInit(&argc,argv);

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

  // Sink forever into the GLUT loop.
  glutMainLoop();
}


