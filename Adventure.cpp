#include <iostream>
#include <cmath>
#include <stdio.h>


#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/glut.h>

#include "myRobo.h"
#include "myBlock.h"
#include "myObstacle.h"
#include "myFireBall.h"
#include "myBallThrower.h"
#include "myObjConfig.h"
#include "functions.h"



using namespace std;

void drawScene();


int main(int argc, char **argv) {

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    int w = glutGet(GLUT_SCREEN_WIDTH);
    int h = glutGet(GLUT_SCREEN_HEIGHT);

	
    initMyBlocks();

    glutInitWindowSize(w, h);
    glutInitWindowPosition(w/2, h/2);

    glutCreateWindow("****Adventure World!****");  // Setup the window
    initRendering();
    SetUpLights();

    for(int i=0;i<12;i++)
	    InitMesh(i);

    // Register callbacks
    glutDisplayFunc(drawScene);
    glutIdleFunc(drawScene);
    glutKeyboardFunc(handleKeypress1);
    glutSpecialFunc(handleKeypress2);
    glutMouseFunc(handleMouseclick);
    glutReshapeFunc(handleResize);
    glutMotionFunc(handleMousePos);
    glutTimerFunc(5, update, 0);
    glutMainLoop();
    return 0;
}



// Function to draw objects on the screen
void drawScene() {
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    glLoadMatrixd(currCameraPos);
    glRotatef(dragNX/10,0,1,0);
    glRotatef(dragNY/10,1,0,0);
    if(storeCamera){
	    glGetDoublev(GL_MODELVIEW_MATRIX, currCameraPos);
	    dragNX=dragNY=0;
    }
    putCamera(currView);
    drawSkybox();	
    drawMyRobo();
    drawMyBlocks();
    drawMyObstacles();
    glPopMatrix();
    glFlush();
    glutSwapBuffers();
    glutPostRedisplay();
}
