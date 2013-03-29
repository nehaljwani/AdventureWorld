#include <assert.h>
#include <fstream>
#include "imageloader.h"


#define PI 3.141592653589
#define DEG2RAD(deg) (deg * PI / 180)
#define DIST3D(x,y,z) (pow(pow(x,2)+pow(y,2)+pow(z,2),0.5))
#define STATIC 0
#define OBSTACLE 1 
#define FLY 2
#define FRONT 1 
#define BACK 2
#define UP 3 
#define DOWN 4

#define THIRD_PERSON_VIEW 0
#define FIRST_PERSON_VIEW 1
#define TOWER_VIEW 2
#define TILE_VIEW 4
#define HELICOPTER_CAM 3

//Globals
GLuint vbo[12];
GLuint vinx[12];
GLint viewPort[4];
GLuint _textureId[16];
bool printEnable=false;
bool jumpMode=false;
bool flyMode=false;
bool canJump=true;
bool canWalk=true;
bool leftClick=false;
bool storeCamera=false;
bool roboAlive=true;
float jumpParabolicY=0.1f;
int landingTile[3];
float dragEX=0;
float dragEY=0;
float dragNX=0;
float dragNY=0;
GLdouble tempMatrix[16];
GLdouble currRoboPos[16]={0,1,0,0,-1,0,0,0,0,0,1,0,6,6,0,1};
GLdouble currCameraPos[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
GLdouble currModelViewMatrix[16]; 
GLdouble currProjectionMatrix[16]; 
int rotationParity=1;
int rotationCount=0;
GLfloat zoomFactor=1.0f;
GLdouble eyeX;
GLdouble eyeY;
GLdouble eyeZ;
GLdouble centerX;
GLdouble centerY;
GLdouble centerZ;
float currTileX=1;
float currTileY=1;
int viewAngle=180;
float viewDist=0;
float shiftSpeed=0.08f;
int obstRotAngle=0;
int currView=THIRD_PERSON_VIEW;
int Zfactor=0;

//Function Declarations
bool checkHit();
float getCurrentX();
float getCurrentY();
float getCurrentZ();
void drawOrigins(float);
void DrawMesh(GLuint, GLuint, int);
void translateMe(float, float, float, GLdouble *);
double getRelativeDist(GLfloat *, GLdouble *);
bool checkObsPos(GLfloat*, int);
void rotateMe(float, float, float, float, GLdouble *);
bool checkLanding();
GLuint loadTexture(Image* ); 



class worldObject {
	public:
		float myPos[3], myColor[3];
		int type;
		bool visibility;
		int movePos[3];
		float shiftPos;
		worldObject(){
			myColor[0]=myColor[1]=myColor[2]=1;
			myPos[0]=myPos[1]=myPos[2]=0;
			visibility=true;
			movePos[0]=movePos[1]=movePos[2]=0;
			shiftPos=0.00f;
			type=STATIC;
		}
		float *getXYZ(){
			return myPos;
		}
		void updateXYZ(float x, float y, float z){
			myPos[0]=x;
			myPos[1]=y;
			myPos[2]=z;
		}
		float *getColor(){
			return myColor;
		}
		void setColor(float R, float G, float B){
			myColor[0]=R;
			myColor[1]=G;
			myColor[2]=B;
		}
		~worldObject(){
		}
};

worldObject arena[15][15];

void initMyBlocks(){
	for(int i=0;i<15;i++){
		for(int j=0;j<15;j++){
			arena[i][j].updateXYZ((i*2+1)*6,(j*2+1)*6,-9.5);
			if(((i+1)%4==0)||((j+1)%4==0))
				arena[i][j].visibility=false;
		}
	}
	for(int i=0;i<4;i++){
		for(int j=0;j<4;j++){
			arena[1+4*i][1+4*j].type=FLY;
		}
	}
}

void putCamera(int currView){
	switch(currView){
		case THIRD_PERSON_VIEW:
			viewDist=20;
			centerX=currRoboPos[12];
			centerY=currRoboPos[13];
			centerZ=currRoboPos[14];
			eyeX=viewDist*cos((viewAngle*PI)/180.0)+centerX;
			eyeY=viewDist*sin((viewAngle*PI)/180.0)+centerY;
			eyeZ=10+centerZ+Zfactor;
			gluLookAt(eyeX,eyeY,eyeZ,centerX,centerY,centerZ,0,0,1);
			drawOrigins(10.0f); break;
		case  FIRST_PERSON_VIEW:
			glPushMatrix();
			glLoadMatrixd(currRoboPos);
			glTranslatef(0,-10,0);
			centerX = getCurrentX();
			centerY = getCurrentY();
			centerZ = getCurrentZ();
			glTranslatef(0,5,0);
			eyeX = getCurrentX();
			eyeY = getCurrentY();
			eyeZ = getCurrentZ();
			glPopMatrix();
			gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ,0,0,1);
			drawOrigins(10.0f); break;
		case  TOWER_VIEW:
			viewDist=136;
			glPushMatrix();
			glTranslatef(86,86,60);
			eyeX = getCurrentX()+viewDist*cos((viewAngle*PI)/180.0);
			eyeY = getCurrentY()+viewDist*sin((viewAngle*PI)/180.0);
			eyeZ = getCurrentZ()+Zfactor;
			glPopMatrix();
			gluLookAt(eyeX, eyeY, eyeZ, 86, 86, 0,0,0,1);
			drawOrigins(10.0f); break;
		case TILE_VIEW:
			glPushMatrix();
			glTranslatef((currTileX*2+1)*6,(currTileY*2+1)*6,-5);
			eyeX = getCurrentX();
			eyeY = getCurrentY();
			eyeZ = getCurrentZ();
			centerX = currRoboPos[12];
			centerY = currRoboPos[13];
			centerZ = currRoboPos[14];
			glPopMatrix();
			gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ,0,0,1);
			drawOrigins(10.0f); break;
		case HELICOPTER_CAM:
			gluLookAt(eyeX, eyeY, eyeZ+Zfactor, centerX, centerY, centerZ,0,0,1);
	}
}

bool endJump(){
	GLfloat newPos[3];
	/* Check with obstacles */
	for(int i=0;i<15;i++){
		for(int j=0;j<15;j++){
			if(i==0||i==6||i==8||i==10) {
				if(j==2||j==4||j==10||j==12){
					newPos[0]=arena[i][j].myPos[0];
					newPos[1]=arena[i][j].myPos[1];
					newPos[2]=arena[i][j].myPos[2]+6;
					if( getRelativeDist(newPos,currRoboPos)<10){
						return true;
					}
				}
			}
			else if((j%2) && !(i%2) && arena[i][j].visibility){
				newPos[0]=arena[i][j].myPos[0];
				newPos[1]=arena[i][j].myPos[1];
				newPos[2]=arena[i][j].myPos[2]+6;
				if( getRelativeDist(newPos,currRoboPos)<10) {
						return true;
				}
			}
		}
	}
	/* Check with static tiles */
	for(int i=0;i<15;i++){
		for(int j=0;j<15;j++){
			if(arena[i][j].type==STATIC){
				if((currRoboPos[14]-arena[i][j].myPos[2]<=9.5) && getRelativeDist(arena[i][j].myPos,currRoboPos)<10) {
					currRoboPos[14]=0;
					return true;
				}
			}
		}
	}
	return false;
}

void roboJump(){
	/* 0, -deltaX, deltaY
	 * deltaY=(4x-x^/6)-(4(x-deltaX)-(x-deltaX)^2/6)  */
	translateMe(0,-0.1f,0.401667-0.0333333*jumpParabolicY, currRoboPos);
	/*Li'l Animation*/
	rotateMe(-0.9375*rotationParity,1,0,0,origins[4]);/*Right Thigh*/
	rotateMe(0.9375*rotationParity,1,0,0,origins[5]); /*Left Thigh*/
	rotateMe(-.9375*rotationParity,1,0,0,origins[6]); /*Right Leg*/
	rotateMe(.9375*rotationParity,1,0,0,origins[7]);  /*Left Leg*/
	rotationCount++;
	if(!(rotationCount%6))
		if(((rotationCount/6)%2))
			rotationParity=rotationParity*-1;
	if(endJump()){
		jumpParabolicY=0.1;
		jumpMode=false;
	}
	else {
		jumpParabolicY+=0.1f;
	}
}

void drawSkybox(){
	glPushMatrix();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glTranslatef(90.0f, 90.0f, 0.0f);
	glRotatef(90,1,0,0);	
	drawOrigins(10);
	glEnable(GL_TEXTURE_2D);

        //Bottom
        glBindTexture(GL_TEXTURE_2D, _textureId[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);

        glNormal3f(0.0, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);        glVertex3f(256.0f, -256.0f, -256.0f);
        glTexCoord2f(1.0f, 0.0f);        glVertex3f(256.0f, -256.0f, 256.0f);
        glTexCoord2f(1.0f, 1.0f);        glVertex3f(-256.0f, -256.0f, 256.0f);
        glTexCoord2f(0.0f, 1.0f);        glVertex3f(-256.0f, -256.0f, -256.0f);

        glEnd();

        //Up
        glBindTexture(GL_TEXTURE_2D, _textureId[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);

        glNormal3f(0.0, -1.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);        glVertex3f(256, 256, -256);
        glTexCoord2f(1.0f, 0.0f);        glVertex3f(256, 256, 256);
        glTexCoord2f(1.0f, 1.0f);        glVertex3f(-256, 256, 256);
        glTexCoord2f(0.0f, 1.0f);        glVertex3f(-256, 256, -256);

        glEnd();

        //Front
        glBindTexture(GL_TEXTURE_2D, _textureId[4]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);

        glNormal3f(-1.0, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);        glVertex3f(256, -256, 256);
        glTexCoord2f(1.0f, 0.0f);        glVertex3f(256,- 256, -256);
        glTexCoord2f(1.0f, 1.0f);        glVertex3f(256, 256, -256);
        glTexCoord2f(0.0f, 1.0f);        glVertex3f(256, 256, 256);

        glEnd();
	
	 //Right
        glBindTexture(GL_TEXTURE_2D, _textureId[3]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);

        glNormal3f(0.0, 0.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f);        glVertex3f(-256, -256, 256);
        glTexCoord2f(1.0f, 0.0f);        glVertex3f(256, -256, 256);
        glTexCoord2f(1.0f, 1.0f);        glVertex3f(256, 256, 256);
        glTexCoord2f(0.0f, 1.0f);        glVertex3f(-256, 256, 256);

        glEnd();
	
	 //Back
        glBindTexture(GL_TEXTURE_2D, _textureId[5]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);

        glNormal3f(1.0, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f);        glVertex3f(-256, -256, -256);
        glTexCoord2f(1.0f, 0.0f);        glVertex3f(-256, -256, 256);
        glTexCoord2f(1.0f, 1.0f);        glVertex3f(-256, 256, 256);
        glTexCoord2f(0.0f, 1.0f);        glVertex3f(-256, 256,- 256);

        glEnd();

	//Left
        glBindTexture(GL_TEXTURE_2D, _textureId[2]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);

        glNormal3f(0.0, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);        glVertex3f(256, -256, -256);
        glTexCoord2f(1.0f, 0.0f);        glVertex3f(-256, -256, -256);
        glTexCoord2f(1.0f, 1.0f);        glVertex3f(-256, 256, -256);
        glTexCoord2f(0.0f, 1.0f);        glVertex3f(256, 256, -256);

        glEnd();



	glPopMatrix();
	glDisable (GL_TEXTURE_2D);
}

void drawMyRobo(){
	glPushMatrix();
	glMultMatrixd(currRoboPos);
	drawOrigins(5.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
	glDisable (GL_TEXTURE_2D);
	for(int i=0;i<8;i++){ 
		glPushMatrix();
		glMultMatrixd(origins[i]);
		DrawMesh(vbo[i], vinx[i], i); 
		glPopMatrix();
	}   
	glPopMatrix();
	if(jumpMode)
		roboJump();
	if((flyMode && !jumpMode) || (!flyMode && jumpMode)){
		if(checkLanding()){
			flyMode=true;
			jumpMode=false;
			jumpParabolicY=0.1;
		}
		else
			flyMode=false;
	}
}

void drawMyObstacles(){
	glPushMatrix();
	for(int i=0;i<15;i++){
		for(int j=0;j<15;j++){
			if(i==0||i==6||i==8||i==10) {
				if(j==2||j==4||j==10||j==12){
					float *blockPos=arena[i][j].getXYZ();
					glPushMatrix();
					glTranslatef(blockPos[0],blockPos[1],blockPos[2]);
					glTranslatef(0,0,2.5);
					glRotatef(obstRotAngle,0,0,1);
					glScalef(3,3,3);
					drawOrigins(4);
					glColor3f(1,1,1);
					DrawMesh(vbo[9], vinx[9], 9); 
					glPopMatrix();
				}
			}
			else if((j%2) && !(i%2) && arena[i][j].visibility){
				float *blockPos=arena[i][j].getXYZ();
				glPushMatrix();
				glTranslatef(blockPos[0],blockPos[1],blockPos[2]);
				glTranslatef(0,0,2.5);
				glRotatef(obstRotAngle,0,0,1);
				glScalef(3,3,3);
				drawOrigins(4);
				glColor3f(1,1,1);
				DrawMesh(vbo[9], vinx[9], 9); 
				glPopMatrix();
			}
			if(j==14 && (i==5||i==9)){
				/* Draw the ballThrower */
				float *blockPos=arena[i][j].getXYZ();
				glPushMatrix();
				glTranslatef(blockPos[0],blockPos[1],blockPos[2]);
				glRotatef(-90,0,0,1);
				glScalef(3,3,3);
				drawOrigins(4);
				glColor3f(1.0f,1.0f,1.0f);
				DrawMesh(vbo[10], vinx[10], 10);
				glPopMatrix();
				/* Draw the Ball */
				glPushMatrix();
				glTranslatef(blockPos[0],blockPos[1],blockPos[2]);
				glMultMatrixd(origins[11]);
				glTranslatef(0,0,7.5);
				drawOrigins(3);
				glColor3f(1,1,1);
				glScalef(1.41,1.41,1.41);
				DrawMesh(vbo[11], vinx[11], 11);
				glPopMatrix();
			}
		}
	}

	glPopMatrix();
}

void drawMyBlocks(){
	glPushMatrix();
	for(int i=0;i<15;i++){
		for(int j=0;j<15;j++){
			if(arena[i][j].visibility){
				float *blockPos=arena[i][j].getXYZ();
				glPushMatrix();
				glTranslatef(blockPos[0],blockPos[1],blockPos[2]);
				glScalef(3,3,1);
				drawOrigins(4);
				glColor3f(1,1,1);
				DrawMesh(vbo[8], vinx[8], 8); 
				glPopMatrix();
			}
		}
	}
	glPopMatrix();
}

void printCurrentMatrix(){
	glGetDoublev (GL_MODELVIEW_MATRIX, tempMatrix);
	for(int i=0;i<4;i++){
		for(int j=0;j<4;j++)
			printf("%f ",tempMatrix[i+4*j]);
		printf("\n");
	}
	printf("Matrix*Finished\n");
}

void drawLine(float x1, float y1, float z1, float x2, float y2, float z2){
	glBegin(GL_LINES);
	glVertex3f(x1, y1, z1);
	glVertex3f(x2, y2, z2);
	glEnd();
}

void clearMovePos(int i, int j){
	arena[i][j].movePos[0]=0;
	arena[i][j].movePos[1]=0;
	arena[i][j].movePos[2]=0;
}

void updateBlockPos(int i, int j){
	/* UP RIGHT DOWN */
	if((i==1||i==9)&&(j==1||j==9)){
		arena[i][j].shiftPos+=shiftSpeed;
		if(!arena[i][j].movePos[0]){
			arena[i][j].myPos[2]+=shiftSpeed;
			if(arena[i][j].shiftPos>=24){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[0]=1;
			}
		}
		else if(!arena[i][j].movePos[1]){
			arena[i][j].myPos[1]+=shiftSpeed;
			/* Check if robo is in my way */
			if(!jumpMode && !flyMode){
				if(checkObsPos(arena[i][j].myPos, FRONT) && getRelativeDist(arena[i][j].myPos,currRoboPos)<10) 
					translateMe(0,1,0,currRoboPos);
				else if(checkObsPos(arena[i][j].myPos, BACK) && getRelativeDist(arena[i][j].myPos,currRoboPos)<10) 
					translateMe(0,-1,0,currRoboPos);
			}
			if(arena[i][j].shiftPos>=48){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[1]=1;
			}
		}
		else if(!arena[i][j].movePos[2]){
			arena[i][j].myPos[2]-=shiftSpeed;
			if(arena[i][j].shiftPos>=24){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[2]=1;
				clearMovePos(i, j);
			}
		}
	}
	/* DOWN DOWN UP */
	if((i==1||i==9)&&(j==5||j==13)){
		arena[i][j].shiftPos+=shiftSpeed;
		if(!arena[i][j].movePos[0]){
			arena[i][j].myPos[2]-=shiftSpeed;
			if(arena[i][j].shiftPos>=24){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[0]=1;
			}
		}
		else if(!arena[i][j].movePos[1]){
			arena[i][j].myPos[0]+=shiftSpeed;
			if(arena[i][j].shiftPos>=48){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[1]=1;
			}
		}
		else if(!arena[i][j].movePos[2]){
			arena[i][j].myPos[2]+=shiftSpeed;
			if(arena[i][j].shiftPos>=24){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[2]=1;
				clearMovePos(i, j);
			}
		}
	}
	/* UP LEFT DOWN */
	if((i==5||i==13)&&(j==5||j==13)){
		arena[i][j].shiftPos+=shiftSpeed;
		if(!arena[i][j].movePos[0]){
			arena[i][j].myPos[2]+=shiftSpeed;
			if(arena[i][j].shiftPos>=24){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[0]=1;
			}
		}
		else if(!arena[i][j].movePos[1]){
			arena[i][j].myPos[1]-=shiftSpeed;
			/* Check if robo is in my way */
			if(!jumpMode && !flyMode){
				if(checkObsPos(arena[i][j].myPos, FRONT) && getRelativeDist(arena[i][j].myPos,currRoboPos)<10) 
					translateMe(0,1,0,currRoboPos);
				else if(checkObsPos(arena[i][j].myPos, BACK) && getRelativeDist(arena[i][j].myPos,currRoboPos)<10) 
					translateMe(0,-1,0,currRoboPos);
			}
			if(arena[i][j].shiftPos>=48){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[1]=1;
			}
		}
		else if(!arena[i][j].movePos[2]){
			arena[i][j].myPos[2]-=shiftSpeed;
			if(arena[i][j].shiftPos>=24){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[2]=1;
				clearMovePos(i, j);
			}
		}
	}
	/* DOWN UP UP */
	if((i==5||i==13)&&(j==1||j==9)){
		arena[i][j].shiftPos+=shiftSpeed;
		if(!arena[i][j].movePos[0]){
			arena[i][j].myPos[2]-=shiftSpeed;
			if(arena[i][j].shiftPos>=24){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[0]=1;
			}
		}
		else if(!arena[i][j].movePos[1]){
			arena[i][j].myPos[0]-=shiftSpeed;
			if(arena[i][j].shiftPos>=48){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[1]=1;
			}
		}
		else if(!arena[i][j].movePos[2]){
			arena[i][j].myPos[2]+=shiftSpeed;
			if(arena[i][j].shiftPos>=24){
				arena[i][j].shiftPos=0.00f;
				arena[i][j].movePos[2]=1;
				clearMovePos(i, j);
				initMyBlocks();
				glutPostRedisplay();
				flyMode=false;
			}
		}
	}
}


void roboFall(){
	currRoboPos[14]-=0.1f;
	canWalk=false;
	canJump=false;
	if(checkLanding()){
		canWalk=true;
		canJump=true;
		flyMode=true;
	}
}


void killRobo(){
	translateMe(0,-1,0,origins[0]); /*Head*/
	translateMe(0,1,0,origins[1]); /*Torso*/
	translateMe(0,-1,0,origins[2]);     /*Right Hand*/
	translateMe(0,1,0,origins[3]);    /*Left Hand*/
	translateMe(0,-1,0,origins[4]);/*Right Thigh*/
	translateMe(0,1,0,origins[5]); /*Left Thigh*/
	translateMe(0,-1,0,origins[6]); /*Right Leg*/
	translateMe(0,1,0,origins[7]);  /*Left Leg*/
}

void update(int value) {
	for(int i=0;i<4;i++){
		for(int j=0;j<4;j++){
			updateBlockPos(1+4*i, 1+4*j);
		}
	}
	/*Check for holes*/
	if(!flyMode && !jumpMode && ((!arena[int(currRoboPos[12])/12][int(currRoboPos[13])/12].visibility) ||  /* Is a hole */
			(currRoboPos[12]<0||currRoboPos[12]>180) ||				   /* Out of range */	
			(currRoboPos[13]<0||currRoboPos[13]>180) || 				   /* Out of range */
			((int)arena[int(currRoboPos[12])/12][int(currRoboPos[13])/12].myPos[0]!=(2*(int(currRoboPos[12])/12)+1)*6 ||
			 (int)arena[int(currRoboPos[12])/12][int(currRoboPos[13])/12].myPos[1]!=(2*(int(currRoboPos[13])/12)+1)*6 || 
			 arena[int(currRoboPos[12])/12][int(currRoboPos[13])/12].myPos[2]!=-9.5 
			 ))) {				  					   /* Block out of place */
		roboFall();
	}
	/*Move robo with flying tile*/
	if(flyMode && !jumpMode){
		currRoboPos[12]=arena[landingTile[0]][landingTile[1]].myPos[0];
		currRoboPos[13]=arena[landingTile[0]][landingTile[1]].myPos[1];
		currRoboPos[14]=arena[landingTile[0]][landingTile[1]].myPos[2]+9.5;
	}
	if(roboAlive && checkHit()){
		killRobo();	
		roboAlive=false;
	}
	if(!roboAlive){
                viewAngle=(viewAngle+5)%360;
	}
	translateMe(0,-0.2,0,origins[11]);
	if(origins[11][13]<-174)
		origins[11][13]=0;
	obstRotAngle=(obstRotAngle+1)%360;
	glutTimerFunc(5, update, 0);
}

void initRendering() {

    glEnable(GL_DEPTH_TEST);        // Enable objects to be drawn ahead/behind one another
    glEnable(GL_COLOR_MATERIAL);    // Enable coloring
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);   // Setting a background color
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	Image* image0 = loadBMP("Images/Bottom1.1.bmp");
	_textureId[0] = loadTexture(image0);
	delete image0;
	Image* image1 = loadBMP("Images/Up1.1.bmp");
	_textureId[1] = loadTexture(image1);
	delete image1;
	Image* image2 = loadBMP("Images/Side1.1.bmp");
	_textureId[2] = loadTexture(image2);
	delete image2;
	Image* image3 = loadBMP("Images/Side1.2.bmp");
	_textureId[3] = loadTexture(image3);
	delete image3;
	Image* image4 = loadBMP("Images/Side1.3.bmp");
	_textureId[4] = loadTexture(image4);
	delete image4;
	Image* image5 = loadBMP("Images/Side1.4.bmp");
	_textureId[5] = loadTexture(image5);
	delete image5;
  /*  glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
*/
}

void SetUpLights() {
        float pos[] = {10, 10, 10, 0};
        float ambient[] = {0.3, 0.3, 0.3, 1};
        float white[] = {1, 1, 1, 1};

        glEnable(GL_LIGHTING);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

        glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
        glLightfv(GL_LIGHT0, GL_POSITION, pos);

        glEnable(GL_LIGHT0);
}
#define BUFFER_OFFSET(x)((char *)NULL+(x))

void DrawMesh(GLuint _vbo, GLuint _inx, int index) {
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _inx);

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, sizeof (struct vertex_struct), BUFFER_OFFSET(0));

        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, sizeof (struct vertex_struct), BUFFER_OFFSET(3 * sizeof (float)));

        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof (struct vertex_struct), BUFFER_OFFSET(6 * sizeof (float)));

        glDrawElements(GL_TRIANGLES, FACES_COUNT[index] * 3, INX_TYPE, BUFFER_OFFSET(0));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
}
void InitMesh(int index) {
        glGenBuffers(1, &vbo[index]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[index]);
        glBufferData(GL_ARRAY_BUFFER, sizeof (struct vertex_struct) * VERTEX_COUNT[index], vertexs[index], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &vinx[index]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vinx[index]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof (indexes[index][0]) * FACES_COUNT[index] * 3, indexes[index], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// Function called when the window is resized
void handleResize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 600.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glutPostRedisplay();
}
float getCurrentX(){
        glGetDoublev (GL_MODELVIEW_MATRIX, tempMatrix);
        return tempMatrix[12];
}
float getCurrentY(){
        glGetDoublev (GL_MODELVIEW_MATRIX, tempMatrix);
        return tempMatrix[13];
}
float getCurrentZ(){
        glGetDoublev (GL_MODELVIEW_MATRIX, tempMatrix);
        return tempMatrix[14];
}

void handleKeypress1(unsigned char key, int x, int y) {
//	printf("%d\n", key);
	int w = glutGet(GLUT_SCREEN_WIDTH);
	int h = glutGet(GLUT_SCREEN_HEIGHT);
	switch(key){
                case 27: exit(0); break;
                case 54: viewAngle=(viewAngle+15)%360; break;
                case 52: viewAngle=(viewAngle-15)%360; break;
                case 56: Zfactor+=5; break;
                case 50: Zfactor-=5; break;
		case 'p': printf("%f\n",currRoboPos[12]);printf("%f\n",currRoboPos[13]); break;
		case 'c': currView=(currView+1)%5; viewAngle=180; break;
		case 'w': currTileX++; break;
		case 'a': currTileY++; break;
		case 's': currTileX--; break;
		case 'd': currTileY--; break;
		case 't': translateMe(0,-1,0,currCameraPos); break;
		case 'g': translateMe(0,1,0,currCameraPos); break;
		case 'f': translateMe(1,0,0,currCameraPos); break;
		case 'h': translateMe(-1,0,0,currCameraPos); break;
		case 32: if(canJump) jumpMode=true; break; 
		case 43: zoomFactor-=0.1f;
			 glMatrixMode(GL_PROJECTION);
			 glLoadIdentity();
			 gluPerspective(zoomFactor*45.0f, (float)w / (float)h, 0.1f, 600.0f);
			 break;
		case 45: zoomFactor+=0.1f;
			 glMatrixMode(GL_PROJECTION);
			 glLoadIdentity();
			 gluPerspective(zoomFactor*45.0f, (float)w / (float)h, 0.1f, 600.0f);
			 break;
        }
}

void rotateMe(float angle, float x, float y, float z, GLdouble *roboPart){
	glPushMatrix();
	glLoadMatrixd(roboPart);
	glRotatef(angle,x,y,z);
	glGetDoublev(GL_MODELVIEW_MATRIX, roboPart);
	glPopMatrix();
}

void drawOrigins(float axisLength){
	glColor3f(1.0f, 0.0f, 0.0f);
	drawLine(0,0,0,axisLength,0,0);
	glColor3f(0.0f, 1.0f, 0.0f);
	drawLine(0,0,0,0,axisLength,0);
	glColor3f(0.0f, 0.0f, 1.0f);
	drawLine(0,0,0,0,0,axisLength);
}

void translateMe(float x, float y, float z, GLdouble *roboPart){
	glPushMatrix();
	glLoadMatrixd(roboPart);
	glTranslatef(x,y,z);
	glGetDoublev (GL_MODELVIEW_MATRIX, roboPart);	
	glPopMatrix();
}

void roboWalk(){
	rotateMe(0.9375*rotationParity,1,0,0,origins[0]); /*Head*/
	rotateMe(0.9375*rotationParity,1,0,0,origins[1]); /*Torso*/
	rotateMe(15*rotationParity,1,0,0,origins[2]);     /*Right Hand*/
	rotateMe(-15*rotationParity,1,0,0,origins[3]);    /*Left Hand*/
	rotateMe(-0.9375*rotationParity,1,0,0,origins[4]);/*Right Thigh*/
	rotateMe(0.9375*rotationParity,1,0,0,origins[5]); /*Left Thigh*/
	rotateMe(-.9375*rotationParity,1,0,0,origins[6]); /*Right Leg*/
	rotateMe(.9375*rotationParity,1,0,0,origins[7]);  /*Left Leg*/
	rotationCount++;
	if(!(rotationCount%6))
		if(((rotationCount/6)%2))
			rotationParity=rotationParity*-1;
}


void flush2Identity(GLdouble *matrix){
	for(int i=0;i<16;i++)
		matrix[i]=0;
	matrix[0]=matrix[5]=matrix[10]=matrix[15]=1;
}

double getRelativeDist(GLfloat *pos1, GLdouble *pos2){
	return DIST3D(pos1[0]-pos2[12], pos1[1]-pos2[13], pos1[2]-pos2[14]); 
}

/* Check whether obstacle is ahead/back of Robo */
bool checkObsPos(GLfloat *pos, int relativePos){
	float roboX=currRoboPos[12];
	float roboY=currRoboPos[13];
	float roboZ=currRoboPos[14];
	glPushMatrix();
	glLoadMatrixd(currRoboPos);
	glTranslatef(0,-5,0);
	float vectorX1=getCurrentX()-roboX;
	float vectorY1=getCurrentY()-roboY;
	float vectorZ1=getCurrentZ()-roboZ;
	glPopMatrix();
	float vectorX2=-pos[0]+roboX;
	float vectorY2=-pos[1]+roboY;
	float vectorZ2=-pos[2]+roboZ;
	if((vectorX1*vectorX2+vectorY1*vectorY2+vectorZ1*vectorZ2)<0){
		if(relativePos==FRONT)
			return true;
		else
			return false;
	}
	else{
		if(relativePos==BACK)
			return true;
		else
			return false;
	}
}

bool checkLanding(){
	for(int i=0;i<4;i++){
		for(int j=0;j<4;j++){
			if((currRoboPos[12]>=arena[4*i+1][4*j+1].myPos[0]-6) &&	(currRoboPos[12]<=arena[4*i+1][4*j+1].myPos[0]+6)
				&&(currRoboPos[13]>=arena[4*i+1][4*j+1].myPos[1]-6) &&(currRoboPos[13]<=arena[4*i+1][4*j+1].myPos[1]+6)	
				&& (currRoboPos[14]-arena[4*i+1][4*j+1].myPos[2]<=9.5 && currRoboPos[14]-arena[4*i+1][4*j+1].myPos[2]>=0)){
				landingTile[0]=4*i+1;
				landingTile[1]=4*j+1;
				return true;
			}
		}
	}
	return false;
}


bool checkIntersection(int relativePos){
	GLfloat newPos[3];
	/* Check with flying blocks */
	if(!flyMode){
		for(int i=0;i<4;i++){
			for(int j=0;j<4;j++){
				if(getRelativeDist(arena[4*i+1][4*j+1].myPos,currRoboPos)<10) return true;
			}
		}
	}
	/* Check with obstacles */
	for(int i=0;i<15;i++){
		for(int j=0;j<15;j++){
			if(i==0||i==6||i==8||i==10) {
				if(j==2||j==4||j==10||j==12){
					newPos[0]=arena[i][j].myPos[0];
					newPos[1]=arena[i][j].myPos[1];
					newPos[2]=arena[i][j].myPos[2]+2.5;
					if(checkObsPos(newPos, relativePos) && getRelativeDist(newPos,currRoboPos)<10) return true;
				}
			}
			else if((j%2) && !(i%2) && arena[i][j].visibility){
					newPos[0]=arena[i][j].myPos[0];
					newPos[1]=arena[i][j].myPos[1];
					newPos[2]=arena[i][j].myPos[2]+2.5;
					if(checkObsPos(newPos, relativePos) && getRelativeDist(newPos,currRoboPos)<10) return true;
			}
		}
	}
	return false;
}
	
bool checkHit(){
	/* Check with fireBall */
	GLfloat newPos[3];
	newPos[1]=-origins[11][13];
	newPos[0]=(2*5+1)*6;
	newPos[2]=0;
	if(getRelativeDist(newPos, currRoboPos)<10) return true;
	newPos[0]=(2*9+1)*6;
	if(getRelativeDist(newPos, currRoboPos)<10) return true;
	return false;
}

void handleKeypress2(int key, int x, int y) {
	if(!jumpMode && canWalk){
		if(key==GLUT_KEY_UP){
			if(!checkIntersection(FRONT))
				translateMe(0,-1,0,currRoboPos);
			roboWalk();		
		}
		if(key==GLUT_KEY_DOWN){
			if(!checkIntersection(BACK))
				translateMe(0,1,0,currRoboPos);
			roboWalk();		
		}
	}
	if(key==GLUT_KEY_LEFT){
		rotateMe(15,0,0,1,currRoboPos);
		if(currView==THIRD_PERSON_VIEW)
			viewAngle=(viewAngle+15)%360;
	}
	if(key==GLUT_KEY_RIGHT){
		rotateMe(-15,0,0,1,currRoboPos);
		if(currView==THIRD_PERSON_VIEW)
			viewAngle=(viewAngle-15)%360;
	}
}

void handleMousePos(int x, int y) {
	if(leftClick){
		dragNX=-dragEX+x;
		dragNY=-dragEY+y;
	}
}
void handleMouseclick(int button, int state, int x, int y) {
	if(state==GLUT_DOWN && button==GLUT_LEFT_BUTTON){
		storeCamera=false;
		leftClick=true;
		dragEX=x;
		dragEY=y;
	}
	else{
		storeCamera=true;
		leftClick=false;
	}
	if(button==3){
		glPushMatrix();
		glLoadMatrixd(currCameraPos);
		glTranslatef(0,0,1);
		glGetDoublev(GL_MODELVIEW_MATRIX,currCameraPos);
	}
	if(button==4){
		glPushMatrix();
		glLoadMatrixd(currCameraPos);
		glTranslatef(0,0,-1);
		glGetDoublev(GL_MODELVIEW_MATRIX,currCameraPos);
	}
}
GLuint loadTexture(Image* image) {
        GLuint textureId;
        glGenTextures(1, &textureId); //Make room for our texture
        glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
        //Map the image to the texture
        glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
                                 0,                            //0 for now
                                 GL_RGB,                       //Format OpenGL uses for image
                                 image->width, image->height,  //Width and height
                                 0,                            //The border of the image
                                 GL_RGB, //GL_RGB, because pixels are stored in RGB format
                                 GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
                                                   //as unsigned numbers
                                 image->pixels);               //The actual pixel data
        return textureId; //Returns the id of the texture
}

using namespace std;

Image::Image(char* ps, int w, int h) : pixels(ps), width(w), height(h) {

}

Image::~Image() {
        delete[] pixels;
}
namespace {
        //Converts a four-character array to an integer, using little-endian form
        int toInt(const char* bytes) {
                return (int)(((unsigned char)bytes[3] << 24) |
                                         ((unsigned char)bytes[2] << 16) |
                                         ((unsigned char)bytes[1] << 8) |
                                         (unsigned char)bytes[0]);
        }

        //Converts a two-character array to a short, using little-endian form
        short toShort(const char* bytes) {
                return (short)(((unsigned char)bytes[1] << 8) |
                                           (unsigned char)bytes[0]);
        }

        //Reads the next four bytes as an integer, using little-endian form
        int readInt(ifstream &input) {
                char buffer[4];
                input.read(buffer, 4);
                return toInt(buffer);
        }

        //Reads the next two bytes as a short, using little-endian form
        short readShort(ifstream &input) {
                char buffer[2];
                input.read(buffer, 2);
                return toShort(buffer);
        }

        //Just like auto_ptr, but for arrays
        template<class T>
        class auto_array {
                private:
                        T* array;
                        mutable bool isReleased;
                public:
                        explicit auto_array(T* array_ = NULL) :
                                array(array_), isReleased(false) {
                        }

                        auto_array(const auto_array<T> &aarray) {
                                array = aarray.array;
                                isReleased = aarray.isReleased;
                                aarray.isReleased = true;
                        }

                        ~auto_array() {
                                if (!isReleased && array != NULL) {
                                        delete[] array;
                                }
                        }

                        T* get() const {
                                return array;
                        }

                        T &operator*() const {
                                return *array;
                        }

                        void operator=(const auto_array<T> &aarray) {
                                if (!isReleased && array != NULL) {
                                        delete[] array;
                                }
                                array = aarray.array;
                                isReleased = aarray.isReleased;
                                aarray.isReleased = true;
                        }

                        T* operator->() const {
                                return array;
                        }

                        T* release() {
                                isReleased = true;
                                return array;
                        }

                        void reset(T* array_ = NULL) {
                                if (!isReleased && array != NULL) {
                                        delete[] array;
                                }
                                array = array_;
                        }

                        T* operator+(int i) {
                                return array + i;
                        }

                        T &operator[](int i) {
                                return array[i];
                        }
        };
}


Image* loadBMP(const char* filename) {
        ifstream input;
        input.open(filename, ifstream::binary);
        assert(!input.fail() || !"Could not find file");
        char buffer[2];
        input.read(buffer, 2);
        assert((buffer[0] == 'B' && buffer[1] == 'M') || !"Not a bitmap file");
        input.ignore(8);
        int dataOffset = readInt(input);
        int headerSize = readInt(input);
        int width;
        int height;
        switch(headerSize) {
                case 40:
                        //V3
                        width = readInt(input);
                        height = readInt(input);
                        input.ignore(2);
                        assert(readShort(input) == 24 || !"Image is not 24 bits per pixel");
                        assert(readShort(input) == 0 || !"Image is compressed");
                        break;
                case 12:
                        //OS/2 V1
                        width = readShort(input);
                        height = readShort(input);
                        input.ignore(2);
                        assert(readShort(input) == 24 || !"Image is not 24 bits per pixel");
                        break;
                case 64:
                        //OS/2 V2
                        assert(!"Can't load OS/2 V2 bitmaps");
                        break;
                case 108:
                        //Windows V4
                        assert(!"Can't load Windows V4 bitmaps");
                        break;
                case 124:
                        //Windows V5
                        assert(!"Can't load Windows V5 bitmaps");
                        break;
                default:
                        assert(!"Unknown bitmap format");
        }

        //Read the data
        int bytesPerRow = ((width * 3 + 3) / 4) * 4 - (width * 3 % 4);
        int size = bytesPerRow * height;
        auto_array<char> pixels(new char[size]);
        input.seekg(dataOffset, ios_base::beg);
        input.read(pixels.get(), size);

        //Get the data into the right format
        auto_array<char> pixels2(new char[width * height * 3]);
        for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                        for(int c = 0; c < 3; c++) {
                                pixels2[3 * (width * y + x) + c] =
                                        pixels[bytesPerRow * y + 3 * x + (2 - c)];
                        }
                }
        }

        input.close();
        return new Image(pixels2.release(), width, height);
}



