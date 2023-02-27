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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
typedef enum { POINTSMODE, LINESMODE, TRIANGLESMODE, SMOOTHINGMODE, LINESANDTRIANGLESMODE } DISPLAY_MODE;
DISPLAY_MODE displayMode = POINTSMODE;

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
ImageIO * heightmapImage;

// VBO and VAO handles.
//LinesAndTriangles only hold line coordinates, just all the colors are set to black so it can be seen on top of the object
GLuint pointsVAO, linesVAO, trianglesVAO, smoothingVAO, linesAndTrianglesVAO;
GLuint pointsVBO, linesVBO, trianglesVBO, upVBO, downVBO, leftVBO, rightVBO, centerVBO, linesAndTrianglesVBO;

vector<float> pointsCoordinates, pointsColors;
vector<float> linesCoordinates, linesColors, linesAndTrianglesColors;
vector<float> trianglesCoordinates, trianglesColors;
vector<float> upCoors, downCoors, leftCoors, rightCoors, centerCoors, centerColors;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

int imageHeight = 0;
int imageWidth = 0;
//Used for color correcting in smoothing mode to avoid white spikes
int scaleConstant = 1;

//Used to handle colored images
struct ColorAndHeight {
    float height;
    float red;
    float green;
    float blue;
};


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
  /*matrix.LookAt(0.0, 0.0, 5.0,
                0.0, 0.0, 0.0, 
                0.0, 1.0, 0.0);
*/

  matrix.LookAt(1.0 * imageWidth / 2, 1.0 * imageWidth / 1.25, 1.0 * imageWidth / 5,
      1.0 * imageWidth / 2, 0.0, -1.0 * imageWidth / 2,
      0.0, 1.0, 0.0);
      
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
  switch (displayMode) {
    case POINTSMODE:
        glBindVertexArray(pointsVAO);
        glDrawArrays(GL_POINTS, 0, pointsCoordinates.size() / 3);
        glBindVertexArray(0);
        break;
    case LINESMODE:
        glBindVertexArray(linesVAO);
        glDrawArrays(GL_LINES, 0, linesCoordinates.size() / 3);
        glBindVertexArray(0);
        break;
    case TRIANGLESMODE:
        glBindVertexArray(trianglesVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, trianglesCoordinates.size() / 3);
        glBindVertexArray(0);
        break;
    case SMOOTHINGMODE:
        glBindVertexArray(smoothingVAO);
        glDrawArrays(GL_TRIANGLES, 0, centerCoors.size() / 3);
        glBindVertexArray(0);
        break;
    case LINESANDTRIANGLESMODE:
        //Polygon offset is needed so the lines appear slightly above the triangles, otherwise they're constantly intersecting each other
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        glBindVertexArray(trianglesVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, trianglesCoordinates.size() / 3);
        glDisable(GL_POLYGON_OFFSET_FILL);

        glBindVertexArray(linesAndTrianglesVAO);
        glDrawArrays(GL_LINES, 0, linesCoordinates.size() / 3);
        glBindVertexArray(0);
        break;
  }
  // Swap the double-buffers.
  glutSwapBuffers();
}

void idleFunc()
{
	// Do some stuff... 

	// For example, here, you can save the screenshots to disk (to make the animation).

	// Send the signal that we should call displayFunc.

	if (playAnimation) {
		GLint mode = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode"); //Use to change between first mode for 1, 2, 3, and second mode for 4
		GLint constant = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "constant");

        //Rotate while going through modes
        if (numScreenshots < 100) {
            terrainRotate[1] += 0.3;
            terrainRotate[0] += 0.2;
        }
        else if (numScreenshots < 200) {
            terrainRotate[1] -= 0.3;
            terrainRotate[0] -= 0.2;

        }

        //40 screenshots per mode, last 100 show scaling and translating
		if (numScreenshots < 200) {
			if (numScreenshots < 40) {
				displayMode = POINTSMODE;
				glUniform1i(mode, 0);
			}
			else if (numScreenshots < 80) {
				displayMode = LINESMODE;
				glUniform1i(mode, 0);
			}
			else if (numScreenshots < 120) {
				displayMode = TRIANGLESMODE;
				glUniform1i(mode, 0);
			}
			else if (numScreenshots < 160) {
				displayMode = SMOOTHINGMODE;
				glUniform1i(mode, 1);
				glUniform1i(constant, scaleConstant);
			}
			else if (numScreenshots < 200) {
				displayMode = LINESANDTRIANGLESMODE;
				glUniform1i(mode, 0);
			}
		}
		else if (numScreenshots < 300) {
			displayMode = TRIANGLESMODE;
			glUniform1i(mode, 0);
			if (numScreenshots < 212) {
				terrainTranslate[0] += 3.0;
				terrainTranslate[1] += 3.0;
			}
			else if (numScreenshots < 224) {
				terrainTranslate[0] -= 3.0;
				terrainTranslate[1] -= 3.0;
			}
			else if (numScreenshots < 236) {
				terrainScale[0] += 0.1;
			}
			else if (numScreenshots < 248) {
				terrainScale[0] -= 0.1;
			}
			else if (numScreenshots < 260) {
				terrainScale[1] += 0.1;
			}
			else if (numScreenshots < 272) {
				terrainScale[1] -= 0.1;
			}
			else if (numScreenshots < 286) {
				terrainScale[0] += 0.1;
				terrainScale[1] += 0.1;
			}
			else if (numScreenshots < 300) {
				terrainScale[0] -= 0.1;
				terrainScale[1] -= 0.1;
			}
		}

		if (numScreenshots < 300) {
			char filename[8] = "";
			if (numScreenshots < 10) {
				strcat(filename, "00");
				strcat(filename, to_string(numScreenshots).c_str());
				strcat(filename, ".jpg");
			}
			else if (numScreenshots < 100) {
				strcat(filename, "0");
				strcat(filename, to_string(numScreenshots).c_str());
                strcat(filename, ".jpg");
            }
            else {
                strcat(filename, to_string(numScreenshots).c_str());
                strcat(filename, ".jpg");
            }
            saveScreenshot(filename);
            numScreenshots++;
        }
	}

	
	if (numScreenshots >= 300) {
		playAnimation = false;
		numScreenshots = 0;
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

    case '1':
        displayMode = POINTSMODE;   
        glUniform1i(mode, 0);
        break;
    case '2':
        displayMode = LINESMODE;
        glUniform1i(mode, 0);
        break;
    case '3':
        displayMode = TRIANGLESMODE;
        glUniform1i(mode, 0);
        break;
    case '4':
        displayMode = SMOOTHINGMODE;
        glUniform1i(mode, 1);
        glUniform1i(constant, scaleConstant);
        break;
    case '5':
        displayMode = LINESANDTRIANGLESMODE;
        glUniform1i(mode, 0);
        break;
    
    case 'a':
        playAnimation = true;
        break;
  }
}

//Use formula Grayscale = 0.299R + 0.587G + 0.114B from https://www.dynamsoft.com/blog/insights/image-processing/image-processing-101-color-space-conversion/
ColorAndHeight getHeightFromColor(int i, int j, bool blackAndWhiteImage) {
    ColorAndHeight pixel;
    if (blackAndWhiteImage) {
        float heightOfVertex = heightmapImage->getPixel(i, j, 0);
        pixel.height = heightOfVertex;
        pixel.red = heightOfVertex;
        pixel.green = heightOfVertex;
        pixel.blue = heightOfVertex;
    }
    else {
        float red = heightmapImage->getPixel(i, j, 0);
        float green = heightmapImage->getPixel(i, j, 1);
        float blue = heightmapImage->getPixel(i, j, 2);

        pixel.red = red;
        pixel.green = green;
        pixel.blue = blue;
        pixel.height = 0.299 * red + 0.587 * green + 0.114 * blue;
    }
    return pixel;
}

void getHeightsFromImage() {
    imageHeight = heightmapImage->getHeight();
    imageWidth = heightmapImage->getWidth();
    
    float scale = (1.0 * imageWidth / 128) * 0.1;
    scaleConstant = scale * 300;

    bool blackAndWhiteImage = heightmapImage->getBytesPerPixel() == 1;
    
    for (int i = 0; i < imageWidth; i++) {
        for (int j = 0; j < imageHeight; j++) {
            ColorAndHeight heightOfVertex = getHeightFromColor(i, j, blackAndWhiteImage);

            //Load x, y, z coordinates of that float into point vector (will be used in VBO)
            pointsCoordinates.push_back((float) i);
            pointsCoordinates.push_back(heightOfVertex.height * scale);
            pointsCoordinates.push_back((float) -j);

            //Load r, g, b, and alpha
            pointsColors.push_back(heightOfVertex.red / 255.0);
            pointsColors.push_back(heightOfVertex.green / 255.0);
            pointsColors.push_back(heightOfVertex.blue / 255.0);
            pointsColors.push_back(1.0);

            //Make sure you're not looking at point off the image/out of bounds check
            if (j < imageHeight - 1) {
                ColorAndHeight heightOfNextVertex = getHeightFromColor(i, j + 1, blackAndWhiteImage);
                linesCoordinates.push_back((float)i);
                linesCoordinates.push_back(heightOfVertex.height * scale);
                linesCoordinates.push_back((float)-j);
                //Load r, g, b, and alpha
                linesColors.push_back(heightOfVertex.red / 255.0);
                linesColors.push_back(heightOfVertex.green / 255.0);
                linesColors.push_back(heightOfVertex.blue / 255.0);
                linesColors.push_back(1.0);

                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(1.0);

                linesCoordinates.push_back((float)i);
                linesCoordinates.push_back(heightOfNextVertex.height * scale);
                linesCoordinates.push_back((float)-(j + 1));
                //Load r, g, b, and alpha
                linesColors.push_back(heightOfNextVertex.red / 255.0);
                linesColors.push_back(heightOfNextVertex.green / 255.0);
                linesColors.push_back(heightOfNextVertex.blue / 255.0);
                linesColors.push_back(1.0);

                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(1.0);
            }
            if (i < imageWidth - 1) {
                ColorAndHeight heightOfNextVertex = getHeightFromColor(i + 1, j, blackAndWhiteImage);
                linesCoordinates.push_back((float)i);
                linesCoordinates.push_back(heightOfVertex.height * scale);
                linesCoordinates.push_back((float)-j);
                //Load r, g, b, and alpha
                linesColors.push_back(heightOfVertex.red / 255.0);
                linesColors.push_back(heightOfVertex.green / 255.0);
                linesColors.push_back(heightOfVertex.blue / 255.0);
                linesColors.push_back(1.0);

                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(1.0);

                linesCoordinates.push_back((float)(i + 1));
                linesCoordinates.push_back(heightOfNextVertex.height * scale);
                linesCoordinates.push_back((float)-j);
                //Load r, g, b, and alpha
                linesColors.push_back(heightOfNextVertex.red / 255.0);
                linesColors.push_back(heightOfNextVertex.green / 255.0);
                linesColors.push_back(heightOfNextVertex.blue / 255.0);
                linesColors.push_back(1.0);

                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(0);
                linesAndTrianglesColors.push_back(1.0);
            }
            /* Old way when using GL_TRIANGLES, right below this comment is new code for GL_TRIANGLE_STRIP
            if (i < imageWidth - 1 && j < imageHeight - 1) {
                float heightOfRightVertex = heightmapImage->getPixel(i + 1, j, 0);
                float heightOfUpVertex = heightmapImage->getPixel(i, j + 1, 0);
                float heightOfUpRightVertex = heightmapImage->getPixel(i + 1, j + 1, 0);

                //Creates bottom left triangle
                trianglesCoordinates.push_back((float)i);
                trianglesCoordinates.push_back(heightOfVertex * scale);
                trianglesCoordinates.push_back((float)-j);
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfVertex / 255.0);
                trianglesColors.push_back(heightOfVertex / 255.0);
                trianglesColors.push_back(heightOfVertex / 255.0);
                trianglesColors.push_back(1.0);

                trianglesCoordinates.push_back((float)i + 1);
                trianglesCoordinates.push_back(heightOfRightVertex * scale);
                trianglesCoordinates.push_back((float)-j);
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfRightVertex / 255.0);
                trianglesColors.push_back(heightOfRightVertex / 255.0);
                trianglesColors.push_back(heightOfRightVertex / 255.0);
                trianglesColors.push_back(1.0);

                trianglesCoordinates.push_back((float)i);
                trianglesCoordinates.push_back(heightOfUpVertex * scale);
                trianglesCoordinates.push_back((float)-(j + 1));
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfUpVertex / 255.0);
                trianglesColors.push_back(heightOfUpVertex / 255.0);
                trianglesColors.push_back(heightOfUpVertex / 255.0);
                trianglesColors.push_back(1.0);

                //Creates top right triangle
                trianglesCoordinates.push_back((float)(i + 1));
                trianglesCoordinates.push_back(heightOfUpRightVertex * scale);
                trianglesCoordinates.push_back((float)-(j + 1));
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfUpRightVertex / 255.0);
                trianglesColors.push_back(heightOfUpRightVertex / 255.0);
                trianglesColors.push_back(heightOfUpRightVertex / 255.0);
                trianglesColors.push_back(1.0);

                trianglesCoordinates.push_back((float)i + 1);
                trianglesCoordinates.push_back(heightOfRightVertex * scale);
                trianglesCoordinates.push_back((float)-j);
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfRightVertex / 255.0);
                trianglesColors.push_back(heightOfRightVertex / 255.0);
                trianglesColors.push_back(heightOfRightVertex / 255.0);
                trianglesColors.push_back(1.0);

                trianglesCoordinates.push_back((float)i);
                trianglesCoordinates.push_back(heightOfUpVertex * scale);
                trianglesCoordinates.push_back((float)-(j + 1));
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfUpVertex / 255.0);
                trianglesColors.push_back(heightOfUpVertex / 255.0);
                trianglesColors.push_back(heightOfUpVertex / 255.0);
                trianglesColors.push_back(1.0);
            } */
		}
    }
    //Add starting vertex
    ColorAndHeight heightOfVertex = getHeightFromColor(0, 0, blackAndWhiteImage);
    trianglesCoordinates.push_back((float)0);
    trianglesCoordinates.push_back(heightOfVertex.height * scale);
    trianglesCoordinates.push_back((float)0);
    //Load r, g, b, and alpha
    trianglesColors.push_back(heightOfVertex.red / 255.0);
    trianglesColors.push_back(heightOfVertex.green / 255.0);
    trianglesColors.push_back(heightOfVertex.blue / 255.0);
    trianglesColors.push_back(1.0);
    //Use a snaking pattern to avoid triangles spanning across the whole image
    for (int i = 0; i < imageWidth - 1; i++) {
        if (i % 2 == 0) {
            for (int j = 0; j < imageHeight - 1; j++) {
                heightOfVertex = getHeightFromColor(i, j, blackAndWhiteImage);
                ColorAndHeight heightOfRightVertex = getHeightFromColor(i + 1, j, blackAndWhiteImage);
                ColorAndHeight heightOfUpVertex = getHeightFromColor(i, j + 1, blackAndWhiteImage);

                trianglesCoordinates.push_back((float)i + 1);
                trianglesCoordinates.push_back(heightOfRightVertex.height * scale);
                trianglesCoordinates.push_back((float)-j);
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfRightVertex.red / 255.0);
                trianglesColors.push_back(heightOfRightVertex.green / 255.0);
                trianglesColors.push_back(heightOfRightVertex.blue / 255.0);
                trianglesColors.push_back(1.0);

                trianglesCoordinates.push_back((float)i);
                trianglesCoordinates.push_back(heightOfUpVertex.height * scale);
                trianglesCoordinates.push_back((float)-(j + 1));
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfUpVertex.red / 255.0);
                trianglesColors.push_back(heightOfUpVertex.green / 255.0);
                trianglesColors.push_back(heightOfUpVertex.blue / 255.0);
                trianglesColors.push_back(1.0);

            }
        }
        else {
            for (int j = imageHeight - 1; j > 0; j--) {
                heightOfVertex = getHeightFromColor(i, j, blackAndWhiteImage);
                ColorAndHeight heightOfRightVertex = getHeightFromColor(i + 1, j, blackAndWhiteImage);
                ColorAndHeight heightOfDownVertex = getHeightFromColor(i, j - 1, blackAndWhiteImage);

                trianglesCoordinates.push_back((float)i + 1);
                trianglesCoordinates.push_back(heightOfRightVertex.height * scale);
                trianglesCoordinates.push_back((float)-j);
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfRightVertex.red / 255.0);
                trianglesColors.push_back(heightOfRightVertex.green / 255.0);
                trianglesColors.push_back(heightOfRightVertex.blue / 255.0);
                trianglesColors.push_back(1.0);

                trianglesCoordinates.push_back((float)i);
                trianglesCoordinates.push_back(heightOfDownVertex.height * scale);
                trianglesCoordinates.push_back((float)-(j - 1));
                //Load r, g, b, and alpha
                trianglesColors.push_back(heightOfDownVertex.red / 255.0);
                trianglesColors.push_back(heightOfDownVertex.green / 255.0);
                trianglesColors.push_back(heightOfDownVertex.blue / 255.0);
                trianglesColors.push_back(1.0);

            }
        }
    }
    //Add ending vertex
    heightOfVertex = getHeightFromColor(imageWidth - 1, imageHeight - 1, blackAndWhiteImage);;
    trianglesCoordinates.push_back((float)(imageWidth - 1));
    trianglesCoordinates.push_back(heightOfVertex.height * scale);
    trianglesCoordinates.push_back((float)-(imageHeight - 1));
    //Load r, g, b, and alpha
    trianglesColors.push_back(heightOfVertex.red / 255.0);
    trianglesColors.push_back(heightOfVertex.green / 255.0);
    trianglesColors.push_back(heightOfVertex.blue / 255.0);
    trianglesColors.push_back(1.0);



    for (int i = 0; i < imageWidth - 1; i++) {
        for (int j = 0; j < imageHeight - 1; j++) {
            heightOfVertex = getHeightFromColor(i, j, blackAndWhiteImage);
            ColorAndHeight heightOfRightVertex = getHeightFromColor(i + 1, j, blackAndWhiteImage);
            ColorAndHeight heightOfUpVertex = getHeightFromColor(i, j + 1, blackAndWhiteImage);
            ColorAndHeight heightOfUpRightVertex = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);

            //Variables used when adding to up, left, right, and down vectors for smoothing
            ColorAndHeight heightOfRight;
            ColorAndHeight heightOfLeft;
            ColorAndHeight heightOfUp;
            ColorAndHeight heightOfDown;

            //Adding current coordinate
            centerCoors.push_back((float)i);
            centerCoors.push_back(heightOfVertex.height * scale);
            centerCoors.push_back((float)-j);

            centerColors.push_back(heightOfVertex.red / 255.0);
            centerColors.push_back(heightOfVertex.green / 255.0);
            centerColors.push_back(heightOfVertex.blue / 255.0);
            centerColors.push_back(0);

            //If all the way on left
            if (i == 0) {
                heightOfRight = getHeightFromColor(i + 1, j, blackAndWhiteImage);
                heightOfLeft = getHeightFromColor(i + 1, j, blackAndWhiteImage);;

                rightCoors.push_back((float)(i + 1));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-j);

                leftCoors.push_back((float)(i + 1));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-j);

            }
            else {
                heightOfRight = getHeightFromColor(i + 1, j, blackAndWhiteImage);
                heightOfLeft = getHeightFromColor(i - 1, j, blackAndWhiteImage);

                rightCoors.push_back((float)(i + 1));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-j);

                leftCoors.push_back((float)(i - 1));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-j);

            }
            //If all the way at the bottom
            if (j == 0) {
                heightOfUp = getHeightFromColor(i, j + 1, blackAndWhiteImage);
                heightOfDown = getHeightFromColor(i, j + 1, blackAndWhiteImage);

                upCoors.push_back((float)i);
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j + 1));

                downCoors.push_back((float)i);
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j + 1));

            }
            else {
                heightOfUp = getHeightFromColor(i, j + 1, blackAndWhiteImage);
                heightOfDown = getHeightFromColor(i, j - 1, blackAndWhiteImage);

                upCoors.push_back((float)i);
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j + 1));

                downCoors.push_back((float)i);
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j - 1));

            }

            //Adding right coordinate
            centerCoors.push_back((float)(i + 1));
            centerCoors.push_back(heightOfRightVertex.height * scale);
            centerCoors.push_back((float)-j);

            centerColors.push_back(heightOfRightVertex.red / 255.0);
            centerColors.push_back(heightOfRightVertex.green / 255.0);
			centerColors.push_back(heightOfRightVertex.blue / 255.0);
			centerColors.push_back(0);

            //If all the way to the right
            if ((i + 1) == imageWidth - 1) {
                heightOfLeft = getHeightFromColor(i, j, blackAndWhiteImage);
                heightOfRight = getHeightFromColor(i, j, blackAndWhiteImage);

                rightCoors.push_back((float)(i));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-j);

                leftCoors.push_back((float)(i));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-j);
            }
            else {
                heightOfRight = getHeightFromColor(i + 2, j, blackAndWhiteImage);
                heightOfLeft = getHeightFromColor(i, j, blackAndWhiteImage);

                rightCoors.push_back((float)(i + 2));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-j);

                leftCoors.push_back((float)(i));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-j);
            }
            
            //If all the way at bottom
            if (j == 0) {
                heightOfUp = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);
                heightOfDown = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);

                upCoors.push_back((float)(i + 1));
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j + 1));

                downCoors.push_back((float)(i + 1));
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j + 1));

            }
            else {
                heightOfUp = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);
                heightOfDown = getHeightFromColor(i + 1, j - 1, blackAndWhiteImage);

                upCoors.push_back((float)(i + 1));
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j + 1));

                downCoors.push_back((float)(i + 1));
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j - 1));

            }

            //Adding up coordinate
            centerCoors.push_back((float)i);
            centerCoors.push_back(heightOfUpVertex.height * scale);
            centerCoors.push_back((float)-(j + 1));

            centerColors.push_back(heightOfUpVertex.red / 255.0);
            centerColors.push_back(heightOfUpVertex.green / 255.0);
            centerColors.push_back(heightOfUpVertex.blue / 255.0);
            centerColors.push_back(0);

            //If all the way on the left
            if (i == 0) {
                heightOfRight = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);
                heightOfLeft = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);

                rightCoors.push_back((float)(i + 1));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-(j + 1));

                leftCoors.push_back((float)(i + 1));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-(j + 1));

            }
            else {
                heightOfRight = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);
                heightOfLeft = getHeightFromColor(i - 1, j + 1, blackAndWhiteImage);

                rightCoors.push_back((float)(i + 1));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-(j + 1));

                leftCoors.push_back((float)(i - 1));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-(j + 1));

			}

            //If all the way at the top
            if (j + 1 == imageHeight - 1) {
                heightOfDown = getHeightFromColor(i, j, blackAndWhiteImage);
                heightOfUp = getHeightFromColor(i, j, blackAndWhiteImage);

                upCoors.push_back((float)i);
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j));

                downCoors.push_back((float)i);
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j));

            }
            else {
                heightOfUp = getHeightFromColor(i, j + 2, blackAndWhiteImage);
                heightOfDown = getHeightFromColor(i, j, blackAndWhiteImage);

                upCoors.push_back((float)i);
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j + 2));

                downCoors.push_back((float)i);
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j));

            }
			
            //Adding upper right coordinate
			centerCoors.push_back((float)(i + 1));
			centerCoors.push_back(heightOfUpRightVertex.height * scale);
			centerCoors.push_back((float)-(j + 1));

			centerColors.push_back(heightOfUpRightVertex.red / 255.0);
			centerColors.push_back(heightOfUpRightVertex.green / 255.0);
			centerColors.push_back(heightOfUpRightVertex.blue / 255.0);
			centerColors.push_back(0);

            //If all the way to the right
            if ((i + 1) == imageWidth - 1) {
                heightOfLeft = getHeightFromColor(i, j + 1, blackAndWhiteImage);
                heightOfRight = getHeightFromColor(i, j + 1, blackAndWhiteImage);

                rightCoors.push_back((float)(i));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-(j + 1));

                leftCoors.push_back((float)(i));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-(j + 1));
            }
            else {
                heightOfRight = getHeightFromColor(i + 2, j + 1, blackAndWhiteImage);
                heightOfLeft = getHeightFromColor(i, j + 1, blackAndWhiteImage);

                rightCoors.push_back((float)(i + 2));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-(j + 1));

                leftCoors.push_back((float)(i));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-(j + 1));

            }

            //If all the way at the top
            if (j + 1 == imageHeight - 1) {
                heightOfDown = getHeightFromColor(i + 1, j, blackAndWhiteImage);
                heightOfUp = getHeightFromColor(i + 1, j, blackAndWhiteImage);

                upCoors.push_back((float)(i + 1));
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j));

                downCoors.push_back((float)(i + 1));
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j));

            }
            else {
                heightOfUp = getHeightFromColor(i + 1, j + 2, blackAndWhiteImage);
                heightOfDown = getHeightFromColor(i + 1, j, blackAndWhiteImage);

                upCoors.push_back((float)(i + 1));
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j + 2));

                downCoors.push_back((float)(i + 1));
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j));

            }

			
            //Adding right coordinate
			centerCoors.push_back((float)(i + 1));
			centerCoors.push_back(heightOfRightVertex.height * scale);
			centerCoors.push_back((float)-j);

            centerColors.push_back(heightOfRightVertex.red / 255.0);
            centerColors.push_back(heightOfRightVertex.green / 255.0);
            centerColors.push_back(heightOfRightVertex.blue / 255.0);
            centerColors.push_back(0);

            //If all the way to the right
            if ((i + 1) == imageWidth - 1) {
                heightOfLeft = getHeightFromColor(i, j, blackAndWhiteImage);
                heightOfRight = getHeightFromColor(i, j, blackAndWhiteImage);

                rightCoors.push_back((float)(i));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-j);

                leftCoors.push_back((float)(i));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-j);
            }
            else {
                heightOfRight = getHeightFromColor(i + 2, j, blackAndWhiteImage);
                heightOfLeft = getHeightFromColor(i, j, blackAndWhiteImage);

                rightCoors.push_back((float)(i + 2));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-j);

                leftCoors.push_back((float)(i));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-j);
            }
            //If all the way on bottom
            if (j == 0) {
                heightOfUp = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);
                heightOfDown = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);

                upCoors.push_back((float)(i + 1));
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j + 1));

                downCoors.push_back((float)(i + 1));
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j + 1));

            }
            else {
                heightOfUp = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);
                heightOfDown = getHeightFromColor(i + 1, j - 1, blackAndWhiteImage);

                upCoors.push_back((float)(i + 1));
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j + 1));

                downCoors.push_back((float)(i + 1));
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j - 1));

            }

            //Adding upper coordinate
            centerCoors.push_back((float)i);
            centerCoors.push_back(heightOfUpVertex.height * scale);
            centerCoors.push_back((float)-(j + 1));

            centerColors.push_back(heightOfUpVertex.red / 255.0);
            centerColors.push_back(heightOfUpVertex.green / 255.0);
            centerColors.push_back(heightOfUpVertex.blue / 255.0);
            centerColors.push_back(0);

            //If all the way on the left
            if (i == 0) {
                heightOfRight = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);
                heightOfLeft = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);

                rightCoors.push_back((float)(i + 1));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-(j + 1));

                leftCoors.push_back((float)(i + 1));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-(j + 1));

            }
            else {
                heightOfRight = getHeightFromColor(i + 1, j + 1, blackAndWhiteImage);
                heightOfLeft = getHeightFromColor(i - 1, j + 1, blackAndWhiteImage);

                rightCoors.push_back((float)(i + 1));
                rightCoors.push_back(heightOfRight.height * scale);
                rightCoors.push_back((float)-(j + 1));

                leftCoors.push_back((float)(i - 1));
                leftCoors.push_back(heightOfLeft.height * scale);
                leftCoors.push_back((float)-(j + 1));

			}

            //If all the way at the top
            if (j + 1 == imageHeight - 1) {
                heightOfDown = getHeightFromColor(i, j, blackAndWhiteImage);
                heightOfUp = getHeightFromColor(i, j, blackAndWhiteImage);

                upCoors.push_back((float)i);
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j));

                downCoors.push_back((float)i);
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j));
            }
            else {
                heightOfUp = getHeightFromColor(i, j + 2, blackAndWhiteImage);
                heightOfDown = getHeightFromColor(i, j, blackAndWhiteImage);

                upCoors.push_back((float)i);
                upCoors.push_back(heightOfUp.height * scale);
                upCoors.push_back((float)-(j + 2));

                downCoors.push_back((float)i);
                downCoors.push_back(heightOfDown.height * scale);
                downCoors.push_back((float)-(j));

            }
        }
    }
}

void initScene(int argc, char *argv[])
{
  // Load the image from a jpeg disk file into main memory.
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.
  getHeightsFromImage();
  
  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

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
  const GLuint locationOfColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color"); // Obtain a handle to the shader variable "color".

  //GLuints for smoothing

  const GLuint locationOfUpPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "upCoors");
  const GLuint locationOfDownPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "downCoors");
  const GLuint locationOfLeftPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "leftCoors");
  const GLuint locationOfRightPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "rightCoors");


  //Originally had this in a switch, but it's in the init so switching between things didn't work since the other VAOs and VBOs weren't loaded, must load all of them
  
  /*POINT VBOS AND VAOS*/
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &pointsVBO);
  glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
  // First, allocate an empty VBO of the correct size to hold positions and colors.
  int numBytesPointsCoordinates = sizeof(float) * pointsCoordinates.size();
  int numBytesPointsColors = sizeof(float) * pointsColors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesPointsCoordinates + numBytesPointsColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesPointsCoordinates, (float*)pointsCoordinates.data());
  glBufferSubData(GL_ARRAY_BUFFER, numBytesPointsCoordinates, numBytesPointsColors, (float*)pointsColors.data());
  // Create the VAOs. There is a single VAO in this example.
  glGenVertexArrays(1, &pointsVAO);
  glBindVertexArray(pointsVAO);
  glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);

  // Set up the relationship between the "position" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfColor, 4, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesPointsCoordinates); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  glBindBuffer(GL_ARRAY_BUFFER, 0); //Unbind in order to do lines next

  /*LINE VBOS AND VAOS*/
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &linesVBO);
  glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
  // First, allocate an empty VBO of the correct size to hold positions and colors.
  int numBytesLinesCoordinates = sizeof(float) * linesCoordinates.size();
  int numBytesLinesColors = sizeof(float) * linesColors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesLinesCoordinates + numBytesLinesColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesLinesCoordinates, (float*)linesCoordinates.data());
  glBufferSubData(GL_ARRAY_BUFFER, numBytesLinesCoordinates, numBytesLinesColors, (float*)linesColors.data());
  // Create the VAOs. There is a single VAO in this example.
  glGenVertexArrays(1, &linesVAO);
  glBindVertexArray(linesVAO);
  glBindBuffer(GL_ARRAY_BUFFER, linesVBO);

  // Set up the relationship between the "position" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfColor, 4, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesLinesCoordinates); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  glBindBuffer(GL_ARRAY_BUFFER, 0); //Unbind in order to do triangles next

  /*TRIANGLE VAOS AND VBOS*/
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &trianglesVBO);
  glBindBuffer(GL_ARRAY_BUFFER, trianglesVBO);
  // First, allocate an empty VBO of the correct size to hold positions and colors.
  int numBytesTrianglesCoordinates = sizeof(float) * trianglesCoordinates.size();
  int numBytesTrianglesColors = sizeof(float) * trianglesColors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesTrianglesCoordinates + numBytesTrianglesColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesTrianglesCoordinates, (float*)trianglesCoordinates.data());
  glBufferSubData(GL_ARRAY_BUFFER, numBytesTrianglesCoordinates, numBytesTrianglesColors, (float*)trianglesColors.data());
  // Create the VAOs. There is a single VAO in this example.
  glGenVertexArrays(1, &trianglesVAO);
  glBindVertexArray(trianglesVAO);
  glBindBuffer(GL_ARRAY_BUFFER, trianglesVBO);

  // Set up the relationship between the "position" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfColor, 4, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesTrianglesCoordinates); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  /*SMOOTHING VAOS AND VBOS*/
  // Create the VBOs. There is 4 VBOs in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &upVBO);
  glBindBuffer(GL_ARRAY_BUFFER, upVBO);
  // First, allocate an empty VBO of the correct size to hold positions.
  int numBytesUpCoordinates = sizeof(float) * upCoors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesUpCoordinates, (float*)upCoors.data(), GL_STATIC_DRAW);
  
  glGenVertexArrays(1, &smoothingVAO);
  glBindVertexArray(smoothingVAO);
  glBindBuffer(GL_ARRAY_BUFFER, upVBO);

  // Set up the relationship between the "upCoors" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfUpPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfUpPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  glGenBuffers(1, &downVBO);
  glBindBuffer(GL_ARRAY_BUFFER, downVBO);
  // First, allocate an empty VBO of the correct size to hold positions.
  int numBytesDownCoordinates = sizeof(float) * downCoors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesDownCoordinates, (float*)downCoors.data(), GL_STATIC_DRAW);
  // Set up the relationship between the "downCoors" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfDownPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfDownPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  glGenBuffers(1, &leftVBO);
  glBindBuffer(GL_ARRAY_BUFFER, leftVBO);
  // First, allocate an empty VBO of the correct size to hold positions.
  int numBytesLeftCoordinates = sizeof(float) * leftCoors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesLeftCoordinates, (float*)leftCoors.data(), GL_STATIC_DRAW);
  // Set up the relationship between the "upCoors" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfLeftPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfLeftPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  glGenBuffers(1, &rightVBO);
  glBindBuffer(GL_ARRAY_BUFFER, rightVBO);
  // First, allocate an empty VBO of the correct size to hold positions.
  int numBytesRightCoordinates = sizeof(float) * rightCoors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesRightCoordinates, (float*)rightCoors.data(), GL_STATIC_DRAW);
  // Set up the relationship between the "rightCoors" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfRightPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfRightPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  glGenBuffers(1, &centerVBO);
  glBindBuffer(GL_ARRAY_BUFFER, centerVBO);
  // First, allocate an empty VBO of the correct size to hold positions and colors.
  int numBytesCenterCoordinates = sizeof(float) * centerCoors.size();
  int numBytesCenterColors = sizeof(float) * centerColors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesCenterCoordinates + numBytesCenterColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesCenterCoordinates, (float*)centerCoors.data());
  glBufferSubData(GL_ARRAY_BUFFER, numBytesCenterCoordinates, numBytesCenterColors, (float*)centerColors.data());
  // Set up the relationship between the "position" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfColor, 4, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesCenterCoordinates); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 


  glBindBuffer(GL_ARRAY_BUFFER, 0);




  /*LINEANDTRIANGLES VBOS AND VAOS*/
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &linesAndTrianglesVBO);
  glBindBuffer(GL_ARRAY_BUFFER, linesAndTrianglesVBO);
  // First, allocate an empty VBO of the correct size to hold positions and colors.
  int numBytesLinesAndTrianglesCoordinates = sizeof(float) * linesCoordinates.size();
  int numBytesLinesAndTrianglesColors = sizeof(float) * linesAndTrianglesColors.size();
  glBufferData(GL_ARRAY_BUFFER, numBytesLinesAndTrianglesCoordinates + numBytesLinesAndTrianglesColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesLinesAndTrianglesCoordinates, (float*)linesCoordinates.data());
  glBufferSubData(GL_ARRAY_BUFFER, numBytesLinesAndTrianglesCoordinates, numBytesLinesAndTrianglesColors, (float*)linesAndTrianglesColors.data());
  // Create the VAOs. There is a single VAO in this example.
  glGenVertexArrays(1, &linesAndTrianglesVAO);
  glBindVertexArray(linesAndTrianglesVAO);
  glBindBuffer(GL_ARRAY_BUFFER, linesAndTrianglesVBO);

  // Set up the relationship between the "position" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void*)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  glEnableVertexAttribArray(locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(locationOfColor, 4, GL_FLOAT, normalized, stride, (const void*)(unsigned long)numBytesLinesAndTrianglesCoordinates); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  glBindBuffer(GL_ARRAY_BUFFER, 0); //Unbind in order to do triangles next


  // Check for any OpenGL errors.
  std::cout << "GL error: " << glGetError() << std::endl;
}




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

    return 0;

  // Perform the initialization.
  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}


