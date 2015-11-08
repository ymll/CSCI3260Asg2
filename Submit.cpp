// Ass2.cpp : Defines the entry point for the console application.
//

#include <stdlib.h>
#include <stdio.h>
#include <gl\glut.h>
#include <windows.h>
#include <math.h>
#include <time.h>
#include <vector>
#include "bitmap.h"
using namespace std;

const float PI = 3.14159265;

int winWidth = 1000;
int winHeight = 800;

float carX, carZ;
float carOrientation;
float wheelOrientation;
float acceleration;
float velocity;
const float wheelRadius = 20.0;
const float keyboardAcceleration = 30000.0f;
const float frictionalAcceleration = 10000.0f;

float keyboardCameraMoveSpeed(10.0);
float mouseCameraMoveSpeed(0.5);
float mouseCameraMoveDirection[2] = {0, 0};

float light_X(-500), light_Y(300), light_Z(-1000);

float cam_X, cam_Y, cam_Z;
float cam_ViewX, cam_ViewY(0.0), cam_ViewZ;
const float defaultCamViewDeltaY(350.0f);
const float defaultCamViewDeltaZ(800.0f);

GLfloat mat_diffuse[] = {0.8,0.2,0.5,1.0};
GLfloat mat_specular[] = {1.0,1.0,1.0,1.0};
GLfloat high_shininess[] = {100.0};

float groundWidth(800.0), groundLong(18000.0);

struct image_texture
{
	int w;
	int h;
	GLubyte* bitmap;
	GLuint tex;
	GLUquadricObj *quad;
};

image_texture texture_ground;
image_texture texture_car;
image_texture texture_wheel;


//Load the BMP file
GLubyte* TextureLoadBitmap(char *filename, int *w, int *h)		/* I - Bitmap file to load */
{
	BITMAPINFO	*info;				/* Bitmap information */
	void		*bits;				/* Bitmap pixel bits */
	GLubyte	*rgb;				/* Bitmap RGB pixels */
	GLubyte   err = '0';

	/*
	* Try loading the bitmap and converting it to RGB...
	*/

	bits = LoadDIBitmap(filename, &info);
	if(bits==NULL) 
		return(NULL);
	rgb = ConvertRGB(info, bits);
	if (rgb == NULL)
	{
		free(info);
		free(bits);
	};

	printf("%s: %d %d\n", filename, info->bmiHeader.biWidth, info->bmiHeader.biHeight);
	printf("read %s successfully\n", filename);
	*w = info->bmiHeader.biWidth;
	*h = info->bmiHeader.biHeight;

	/*
	* Free the bitmap and RGB images, then return 0 (no errors).
	*/

	free(info);
	free(bits);
	return (rgb);

}

void loadImageTexture(char *filename, image_texture &image_texture)
{
	image_texture.bitmap = TextureLoadBitmap(filename, &image_texture.w, &image_texture.h);

	glClearColor(0,0,0,0);
	glGenTextures(1, &image_texture.tex);
	glBindTexture(GL_TEXTURE_2D, image_texture.tex);

	// Texture mapping parameters for filter and repeatance
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_texture.w, image_texture.h, 0, GL_RGB, GL_UNSIGNED_BYTE, image_texture.bitmap);

	image_texture.quad = gluNewQuadric();
	gluQuadricTexture(image_texture.quad, GL_TRUE);
}

void reset(void)
{
	carX = 0.0f;
	carZ = 0.0f;
	carOrientation = 0.0f;
	wheelOrientation = 0.0f;
	acceleration = 0.0f;
	velocity = 0.0f;
	cam_X = 0.0f;
	cam_Y = defaultCamViewDeltaY;
	cam_Z = defaultCamViewDeltaZ;
	cam_ViewX = 0.0f;
	cam_ViewY = 0.0f;
	cam_ViewZ = 0.0f;
}

void init(void) // All Setup For OpenGL Goes Here
{
	// Light 0 Settings
	static GLfloat light0pos[] = {200.f, 100.f, 400.f, 0.f};
	static GLfloat light0_mat1[] = {0.8, 0.8, 0.8, 1.f};
	static GLfloat light0_diff1[] = {0.9, 0.9, 0.9, 1.f};
	glLightfv(GL_LIGHT0, GL_POSITION, light0pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_mat1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diff1);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glEnable(GL_NORMALIZE);

	// Clear Screen color
	glEnable(GL_COLOR_MATERIAL);
	glClearColor(0,0,0,0);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Texture mapping setting for Microsoft's OpenGL implementation
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

	/* Add code here to initialize lighting, read bitmap images, initialize different GLU geometry
	* use glLight, TextureLoadBitmap, gluNewQuadric...etc
	*/
	loadImageTexture("ground.bmp", texture_ground);
	loadImageTexture("car.bmp", texture_car);
	loadImageTexture("wheel.bmp", texture_wheel);

	reset();
}

// Move camera to specified position without changing view angle
void moveCameraTo(float newCamX, float newCamY, float newCamZ)
{
	cam_ViewX = (cam_ViewX - cam_X) + newCamX;
	cam_ViewY = (cam_ViewY - cam_Y) + newCamY;
	cam_ViewZ = (cam_ViewZ - cam_Z) + newCamZ;
	cam_X = newCamX;
	cam_Y = newCamY;
	cam_Z = newCamZ;
	//printf("moveCameraTo: %f, %f, %f; %f, %f, %f\n", cam_X,cam_Y,cam_Z, cam_ViewX, cam_ViewY, cam_ViewZ);
}

// Move camera by magnitude of movement
void moveCameraBy(float deltaX, float deltaY, float deltaZ)
{
	moveCameraTo(cam_X + deltaX, cam_Y + deltaY, cam_Z + deltaZ);
}

void updateCamera() 
{
	gluLookAt(cam_X,cam_Y,cam_Z, cam_ViewX, cam_ViewY, cam_ViewZ, 0, 1, 0);
	moveCameraBy(mouseCameraMoveSpeed * mouseCameraMoveDirection[0], 0, mouseCameraMoveSpeed * mouseCameraMoveDirection[1]);
}

void drawOrigin()
{
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 0.0f);

	// X-axis
	glBegin(GL_LINES);
	{
		glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
		glVertex4f(1.0f, 0.0f, 0.0f, 0.0f);
	}
	glEnd();

	// Y-axis
	glBegin(GL_LINES);
	{
		glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
		glVertex4f(0.0f, 1.0f, 0.0f, 0.0f);
	}
	glEnd();

	// Z-axis
	glBegin(GL_LINES);
	{
		glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
		glVertex4f(0.0f, 0.0f, 1.0f, 0.0f);
	}
	glEnd();

	glPopMatrix();
}

void drawGround()
{
	// Setup
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glClearColor(0,0,0,0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture_ground.tex);

	// Maintain square tiles on floor
	float scale = 4.0;
	float ratioW = groundWidth / texture_ground.w;
	float ratioL = groundLong / texture_ground.h;

	// Draw ground
	glBegin(GL_QUAD_STRIP);
	{
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(-groundWidth/2, 0.0, 0);
		glTexCoord2f(ratioW / scale, 0.0f);
		glVertex3f(groundWidth/2, 0.0, 0);

		glTexCoord2f(0.0f, ratioL / scale);
		glVertex3f(-groundWidth/2, 0.0, -groundLong/2);
		glTexCoord2f(ratioW / scale, ratioL / scale);
		glVertex3f(groundWidth/2, 0.0, -groundLong/2);
	}
	glEnd();

	// Clean up
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

void drawCarBody()
{
	const float carBodyLong = 200.0f;
	const float carBodyWidth = 25.0f;
	const float carBodyCenter = -100.0f;
	
	glPushMatrix();

	// Transform whole car body
	glRotatef(90, 0.0f, 1.0f, 0.0f);
	glScalef(1.0f, 0.5f, 1.0f);
	glTranslatef(0.0, 0.0, carBodyCenter);

	// Draw main car body
	glColor3f(1.0f, 1.0f, 1.0f);
	glClearColor(0,0,0,0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture_car.tex);
	gluCylinder(texture_car.quad, 0.0, carBodyWidth, carBodyLong, 20, 20);
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
}

void drawWheel(float offsetX, float offsetZ, float radius, float width, float angle)
{
	const int slice = 100.0;
	const int stack = 100.0;
	const GLbyte grey = 30;

	// Setup
	glPushMatrix();
	glColor3b(grey, grey, grey);
	glClearColor(0,0,0,0);

	// Recenter
	glTranslatef(offsetX, 0, offsetZ-width/2.0);
	glRotatef(angle, 0, 0, 1);

	// Draw tyre
	gluCylinder(texture_wheel.quad, radius, radius, width, slice, stack);

	// Clear color and apply texture
	glColor3f(1.0f, 1.0f, 1.0f);
	glClearColor(0,0,0,0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture_wheel.tex);

	// Draw two wheel covers
	gluDisk(texture_wheel.quad, 0.0, radius, slice, stack);
	glTranslatef(0.0, 0.0, width);
	gluDisk(texture_wheel.quad, 0.0, radius, slice, stack);

	// Clean up
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

void drawCar()
{
	const float carWidth = 50.0;
	const float carLong = 100.0;
	const float wheelWidth = 20.0;

	glPushMatrix();

	glTranslatef(0, wheelRadius, 0);
	glTranslatef(carX, 0, carZ);
	glRotatef(carOrientation - 90.0f, 0, 1, 0);

	// Draw 4 wheels
	drawWheel(-carLong/2, -carWidth/2, wheelRadius, wheelWidth, wheelOrientation);
	drawWheel(-carLong/2, carWidth/2, wheelRadius, wheelWidth, wheelOrientation);
	drawWheel(carLong/2, -carWidth/2, wheelRadius, wheelWidth, wheelOrientation);
	drawWheel(carLong/2, carWidth/2, wheelRadius, wheelWidth, wheelOrientation);

	drawCarBody();

	glPopMatrix();
}

void display(void) // Here's Where We Do All The Drawing
{
	/* clear the window color before drawing is performed */
	glClearColor(0.0, 0.0, 0.0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	/* Add code here to transform the car and the ground & draw them
	* use glTranslate, glRotate, glLoadIdentity, glPushMatrix, glPopMatrix, glMaterial, 
	* glBegin, gluSphere...etc
	*
	* Add code for Texture Mapping for the car and the ground
	* use glTexImage2D..
	*/

	// Default material
	glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_diffuse);
	glMaterialfv(GL_FRONT,GL_SPECULAR,mat_specular);
	glMaterialfv(GL_FRONT,GL_SHININESS,high_shininess);

	glLoadIdentity();
	updateCamera();

	// TODO:
	// Draw grounds and objects here
	drawOrigin();
	drawGround();
	drawCar();

	glFlush();
	glutSwapBuffers();
}


void reshape(int w, int h) // Resize the GL Window. w=width, h=height
{
	winWidth = w;
	winHeight = h;
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(45, (float)w/(float)h, 100, 2000); 

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

}

/* keyboard CALLBACK for handling special keyboard event */
void keyspecial(int key, int x, int y)
{	
	/* Add code here to control the animation interactively */

	switch (key) 
	{
		// car control
		case GLUT_KEY_UP: // move front
			acceleration = keyboardAcceleration;
			break;
		case GLUT_KEY_DOWN: // move back
			acceleration = -keyboardAcceleration;
			break;
		case GLUT_KEY_LEFT: // move left
			moveCameraBy(-keyboardCameraMoveSpeed, 0, 0);
			break;
		case GLUT_KEY_RIGHT: // move right
			moveCameraBy(keyboardCameraMoveSpeed, 0, 0);
			break;
	}
}

/* keyboard up CALLBACK for handling special keyboard event */
void keyspecialup(int key, int x, int y)
{	
	/* Add code here to control the animation interactively */

	switch (key) 
	{
		// car control
		case GLUT_KEY_UP: // move front
		case GLUT_KEY_DOWN: // move back
			acceleration = 0.0f;
			break;
		case GLUT_KEY_LEFT: // move left
			break;
		case GLUT_KEY_RIGHT: // move right
			break;
	}
}

/* keyboard CALLBACK for handling keyboard event */
void keyboard(unsigned char key, int x, int y)
{
	/* Add code here to control the animation interactively */

	switch (key) 
	{
		case 'r': // Reset all parameters
			reset();
			break;
	}
}

void mouseclick(int button, int state, int x, int y) // Handle the mouse click events here
{
	printf("Button %d, State %d, Position %d, %d \n", button, state, x, y);	
}

void mousemove(int x, int y) // Handle the mouse movement events here
{
	printf("Mouse moved to position %d %d \n", x, y);
}

void timer(int t)
{
	/* Add code here to update the velocity, acceleration, position and rotation of car and wheels */
	float seconds = t / 1000.0f;
	float resultingAcceleration = acceleration;
	float movingDistance = 0.0f;

	if (velocity > 0.01f)
	{
		resultingAcceleration -= frictionalAcceleration;
	}
	else if (velocity < -0.01f)
	{
		resultingAcceleration += frictionalAcceleration;
	}

	movingDistance = velocity * seconds + 0.5 * resultingAcceleration * seconds * seconds;
	carX -= (movingDistance * sin(carOrientation * PI / 180.0f));
	carZ -= (movingDistance * cos(carOrientation * PI / 180.0f));
	wheelOrientation += (movingDistance * 180.0f / PI / wheelRadius);
	
	velocity += resultingAcceleration * seconds;
	if (fabs(velocity) < 0.01f)
	{
		velocity = 0.0f;
	}

	printf("%f, %f, V: %f, Aa: %f, Ra: %f, Wheel: %f\n", carX, carZ, velocity, acceleration, resultingAcceleration, wheelOrientation);
	moveCameraTo(carX, 350, carZ + 800.0f);

	// display after update and reset timer
	glutPostRedisplay();
    glutTimerFunc(10, timer, 1);
}

void drag(int ix, int iy)
{

}

void main(int argc, char** argv)
{
	/*Initialization of GLUT Library */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);

	/*Create a window with title specified */
	glutInitWindowSize(winWidth, winHeight);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Assignment 2");

	/*Register different CALLBACK function for GLUT to response 
	with different events, e.g. window sizing, mouse click or
	keyboard stroke */
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(keyspecial);
	glutSpecialUpFunc(keyspecialup);
	// call timer function every 10 ms
	glutTimerFunc(10, timer, 1);
	glutMouseFunc(mouseclick);
	glutMotionFunc(drag);
	init();	/*not GLUT call, initialize several parameters */

	/*Enter the GLUT event processing loop which never returns.
	it will call different registered CALLBACK according
	to different events. */
	glutMainLoop();
}
