/**
 * @file Main.cpp
 * @developed by Raviteja Poosala
 * @Seattle University on , date Dec 5, 2021
 * @Animation of Indian Flag Waiving 
 */

#include <glad.h>
#include <glfw/glfw3.h>
#include <stdio.h>
#include <vector>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "Misc.h"

GLFWwindow* window;


// display parameters
int screenWidth = 1366;
int screenHeight = 768;

//For Flag

//For Shader Program Object;
GLint gProgramFlag;


//For Shapes
GLuint vBuffer;

// Flag Grid
#define MESH_WIDTH 20
#define MESH_HEIGHT 20


//For Texture
GLuint textureName;
int textureUnit = 0;


//For Particales
GLfloat partical_CurrPos[MESH_WIDTH * MESH_HEIGHT * 3];
GLfloat partical_OldPos[MESH_WIDTH * MESH_WIDTH * 3];
GLfloat partical_Normal[MESH_WIDTH * MESH_HEIGHT * 3];
GLfloat partical_Texcoord[MESH_WIDTH * MESH_HEIGHT * 2];
GLfloat partical_Acc[MESH_WIDTH * MESH_HEIGHT * 3];
GLuint partical_Elements[(MESH_WIDTH - 1) * (MESH_HEIGHT - 1) * 6];
GLshort partical_Move[MESH_WIDTH * MESH_HEIGHT];

GLfloat gParticalMass = 1.0f;
GLint gGrid_NumOfElements = 0;
GLfloat DAMPING = 0.01f;
GLfloat dtSquare = 0.4f;
GLint CONSTRAINT_ITERATIONS = 5;


//For Springs / Constrains
struct Constrains {
    int index0;
    int index1;
    int move0;
    int move1;
    float rest_distance;
};


struct Constrains* pConstrains = NULL;
GLint gTotalConstrains = 4 * (MESH_WIDTH - 1) * (MESH_HEIGHT - 1) + (MESH_HEIGHT - 1) + (MESH_WIDTH - 1);


//For Projection
mat4 persp;

const char* vertexShaderFlag = R"(
	#version 400
	
	in vec3 position;
	in vec3 normal;
	in vec2 uv;

	out vec3 vPosition;
	out vec3 vNormal;
	out vec2 vUv;

	uniform mat4 modelviewMatrix;
	uniform mat4 perspMatrix;

	void main() {
		vPosition = (modelviewMatrix * vec4(position, 1.0f)).xyz;
		vNormal = (modelviewMatrix * vec4(normal, 0.0f)).xyz;
		gl_Position = perspMatrix * vec4(vPosition, 1.0f);
		vUv = uv;
	}
)";

const char* pixelShaderFlag = R"(
	#version 400

	in vec3 vPosition;
	in vec3 vNormal;
	in vec2 vUv;

	out vec4 pColor;
		
	uniform sampler2D samplerTexture;

	uniform vec3 lightPos = vec3(0,0, 10);
	uniform float a = .05;

	void main() 
	{
		
		vec3 N = normalize(vNormal);				// surface normal
		vec3 L = normalize(lightPos - vPosition);	// light vector
		vec3 E = normalize(vPosition);				// eye vertex
		vec3 R = reflect(L, N);						// highlight vector
		float d = max(0, dot(N, L));				// one-sided diffuse
		float h = max(0, dot(R, E));				// highlight term
		float s = pow(h, 100);						// specular term
		float intensity = clamp(a + d + s, 0, 1);

		vec4 tex = texture(samplerTexture, vUv);
		pColor = vec4(intensity * vec3(1.0f), 1) * tex;
	}
)";






int WindowHeight(GLFWwindow *w) {
    int width, height;
    glfwGetWindowSize(w, &width, &height);
    return height;
}



// Application
void Resize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, screenWidth = width, screenHeight = height);
}



void LoadGrid(GLfloat fWidth, GLfloat fHeight, GLfloat startX, GLfloat startY) {

	void ShowAllConstrains(struct Constrains*);
	void Uninitialize_Flag(void);

	GLfloat dX = startX;
	GLfloat dY = startY;
	GLfloat widthFactor = fWidth / (MESH_WIDTH - 1);
	GLfloat heightFactor = fHeight / (MESH_HEIGHT - 1);

	GLfloat texX_Factor = 1.0f / (MESH_WIDTH - 1);
	GLfloat texY_Factor = 1.0f / (MESH_HEIGHT - 1);

	GLfloat texX = 0.0f;
	GLfloat texY = 1.0f;

	pConstrains = (struct Constrains*)malloc(sizeof(struct Constrains) * gTotalConstrains);
	if (pConstrains == NULL) {
		Uninitialize_Flag();
		glfwSwapBuffers(window);
	}

	int i = 0;

	for (int y = 0; y < MESH_HEIGHT; y++)
	{

		dX = startX;
		texX = 0.0f;

		for (int x = 0; x < MESH_WIDTH; x++) {


			partical_CurrPos[(y * MESH_WIDTH * 3) + (x * 3) + 0] = dX;
			partical_CurrPos[(y * MESH_WIDTH * 3) + (x * 3) + 1] = dY;
			partical_CurrPos[(y * MESH_WIDTH * 3) + (x * 3) + 2] = 0.0f;


			partical_OldPos[(y * MESH_WIDTH * 3) + (x * 3) + 0] = dX;
			partical_OldPos[(y * MESH_WIDTH * 3) + (x * 3) + 1] = dY;
			partical_OldPos[(y * MESH_WIDTH * 3) + (x * 3) + 2] = 0.0f;

			partical_Normal[(y * MESH_WIDTH * 3) + (x * 3) + 0] = dX;
			partical_Normal[(y * MESH_WIDTH * 3) + (x * 3) + 1] = dY;
			partical_Normal[(y * MESH_WIDTH * 3) + (x * 3) + 2] = 1.0f;


			partical_Texcoord[(y * MESH_WIDTH * 2) + (x * 2) + 0] = texX;
			partical_Texcoord[(y * MESH_WIDTH * 2) + (x * 2) + 1] = texY;

			partical_Acc[(y * MESH_WIDTH * 3) + (x * 3) + 0] = 0.0f;
			partical_Acc[(y * MESH_WIDTH * 3) + (x * 3) + 1] = 0.0f;
			partical_Acc[(y * MESH_WIDTH * 3) + (x * 3) + 2] = 0.0f;


			//x dir
			if (x < (MESH_WIDTH - 1)) {
				pConstrains[i].index0 = (y * MESH_WIDTH * 3) + (x * 3);
				pConstrains[i].index1 = (y * MESH_WIDTH * 3) + ((x + 1) * 3);
				pConstrains[i].move0 = (y * MESH_WIDTH) + x;
				pConstrains[i].move1 = (y * MESH_WIDTH) + (x + 1);
				pConstrains[i].rest_distance = widthFactor;
				printf("%d , %d , %d , %d , %f\n" , pConstrains[i].index0 ,  pConstrains[i].index1 , pConstrains[i].move0 , pConstrains[i].move1 , pConstrains[i].rest_distance
				);
				i++;
			}
			
			//y dir conn
			if (y < (MESH_HEIGHT - 1)) {
				pConstrains[i].index0 = (y * MESH_WIDTH * 3) + (x * 3);
				pConstrains[i].index1 = ((y + 1) * MESH_WIDTH * 3) + (x * 3);
				pConstrains[i].move0 = (y * MESH_WIDTH) + x;
				pConstrains[i].move1 = ((y + 1) * MESH_WIDTH) + x;
				pConstrains[i].rest_distance = heightFactor;
				i++;
			}

			//diag
			if (x < (MESH_WIDTH - 1) && y < (MESH_HEIGHT - 1)) {

				//diag to the left to right
				pConstrains[i].index0 = (y * MESH_WIDTH * 3) + (x * 3);
				pConstrains[i].index1 = ((y + 1) * MESH_WIDTH * 3) + ((x + 1) * 3);
				pConstrains[i].move0 = (y * MESH_WIDTH) + x;
				pConstrains[i].move1 = ((y + 1) * MESH_WIDTH) + (x + 1);
				pConstrains[i].rest_distance = sqrt(widthFactor * widthFactor + heightFactor * heightFactor);
				i++;

				//diag downwards
				pConstrains[i].index0 = (y * MESH_WIDTH * 3) + ((x + 1) * 3);
				pConstrains[i].index1 = ((y + 1) * MESH_WIDTH * 3) + (x * 3);
				pConstrains[i].move0 = (y * MESH_WIDTH) + (x + 1);
				pConstrains[i].move1 = ((y + 1) * MESH_WIDTH) + (x);
				pConstrains[i].rest_distance = sqrt(widthFactor * widthFactor + heightFactor * heightFactor);
				i++;

			}

			texX = texX + texX_Factor;

			dX = dX + widthFactor;
		}

		texY = texY - texY_Factor;
		dY = dY - heightFactor;
	}


	int index = 0;

	unsigned int topLeft;
	unsigned int topRight;
	unsigned int bottomLeft;
	unsigned int bottomRight;

	for (int y = 0; y < (MESH_HEIGHT - 1); y++) {

		for (int x = 0; x < (MESH_WIDTH - 1); x++) {

			topLeft = (y * MESH_WIDTH) + x;
			topRight = topLeft + 1;
			bottomLeft = ((y + 1) * MESH_WIDTH) + x;
			bottomRight = bottomLeft + 1;


			partical_Elements[index + 0] = topLeft;
			partical_Elements[index + 1] = bottomLeft;
			partical_Elements[index + 2] = topRight;

			partical_Elements[index + 3] = topRight;
			partical_Elements[index + 4] = bottomLeft;
			partical_Elements[index + 5] = bottomRight;

			index = index + 6;
		}
	}

	gGrid_NumOfElements = index;


	//ShowAllConstrains(pConstrains);

}


void ClothAddForce(vec3 force) {

	vec3 acc;
	//force=mass*acc
	acc = force / gParticalMass;

	for (int y = 0; y < MESH_HEIGHT; y++) {
		for (int x = 0; x < MESH_WIDTH; x++) {

			partical_Acc[(y * MESH_WIDTH * 3) + (x * 3) + 0] += acc[0];
			partical_Acc[(y * MESH_WIDTH * 3) + (x * 3) + 1] += acc[1];
			partical_Acc[(y * MESH_WIDTH * 3) + (x * 3) + 2] += acc[2];
			//printf("FORCE: Normal For : %d\n", (y * MESH_WIDTH * 3) + (x * 3));
		}
	}

}


void ClothWindForce(vec3 direction) {

	void AddWindForceToTriangle(int, int, int, vec3);


	for (int y = 0; y < MESH_HEIGHT - 1; y++) {

		for (int x = 0; x < MESH_WIDTH - 1; x++) {

			int topLeft = (y * MESH_WIDTH * 3) + (x * 3);
			int topRight = (y * MESH_WIDTH * 3) + ((x + 1) * 3);
			int bottomLeft = ((y + 1) * MESH_WIDTH * 3) + (x * 3);
			int bottomRight = ((y + 1) * MESH_WIDTH * 3) + ((x + 1) * 3);

			AddWindForceToTriangle(topLeft, bottomLeft, topRight, direction);
			AddWindForceToTriangle(topRight, bottomRight, bottomLeft, direction);

		}
	}

}


void AddWindForceToTriangle(int p1, int p2, int p3, vec3 dir) {

	vec3 CalculateTriangleNormal(int, int, int);
	void ParticalAddForce(vec3, int);

	vec3 normal = CalculateTriangleNormal(p1, p2, p3);
	vec3 d = normalize(normal);
	vec3 force = normal * dot(d, dir);

	ParticalAddForce(force, p1);
	ParticalAddForce(force, p2);
	ParticalAddForce(force, p3);

}


void ParticalAddForce(vec3 force, int iPartical) {

	vec3 acc = force / gParticalMass;

	partical_Acc[iPartical + 0] += acc[0];
	partical_Acc[iPartical + 1] += acc[1];
	partical_Acc[iPartical + 2] += acc[2];

	//printf("ACC: Normal For : %d\n", iPartical);

}


vec3 CalculateTriangleNormal(int p1, int p2, int p3) {

	vec3 pos1 = vec3(partical_CurrPos[p1 + 0], partical_CurrPos[p1 + 1], partical_CurrPos[p1 + 2]);
	vec3 pos2 = vec3(partical_CurrPos[p2 + 0], partical_CurrPos[p2 + 1], partical_CurrPos[p2 + 2]);
	vec3 pos3 = vec3(partical_CurrPos[p3 + 0], partical_CurrPos[p3 + 1], partical_CurrPos[p3 + 2]);


	vec3 v1 = pos2 - pos1;
	vec3 v2 = pos3 - pos1;

	vec3 v3 = cross(v1, v2);

	
	partical_Normal[p1 + 0] = v3[0];
	partical_Normal[p1 + 1] = v3[1];
	partical_Normal[p1 + 2] = v3[2];

	partical_Normal[p2 + 0] = v3[0];
	partical_Normal[p2 + 1] = v3[1];
	partical_Normal[p2 + 2] = v3[2];

	partical_Normal[p3 + 0] = v3[0];
	partical_Normal[p3 + 1] = v3[1];
	partical_Normal[p3 + 2] = v3[2];

	return(v3);
}


void ParticalMovement(void) {


	for (int y = 0; y < MESH_HEIGHT; y++) {

		for (int x = 0; x < MESH_WIDTH; x++) {

			if (partical_Move[(y * MESH_WIDTH + x)]) {

				int index = (y * MESH_WIDTH * 3) + (x * 3);


				vec3 currPos = vec3(partical_CurrPos[index + 0],
					partical_CurrPos[index + 1],
					partical_CurrPos[index + 2]);

				vec3 oldPos = vec3(partical_OldPos[index + 0],
					partical_OldPos[index + 1],
					partical_OldPos[index + 2]);

				vec3 acc = vec3(partical_Acc[index + 0],
					partical_Acc[index + 1],
					partical_Acc[index + 2]);



				currPos = currPos + (currPos - oldPos) * (1.0f - DAMPING) + acc * dtSquare;

				partical_OldPos[index + 0] = partical_CurrPos[index + 0];
				partical_OldPos[index + 1] = partical_CurrPos[index + 1];
				partical_OldPos[index + 2] = partical_CurrPos[index + 2];

				partical_CurrPos[index + 0] = currPos[0];
				partical_CurrPos[index + 1] = currPos[1];
				partical_CurrPos[index + 2] = currPos[2];

				partical_Acc[index + 0] = 0.0f;
				partical_Acc[index + 1] = 0.0f;
				partical_Acc[index + 2] = 0.0f;
			}
		}
	}
}



void CheckForConstrains(void) {

	for (int i = 0; i < CONSTRAINT_ITERATIONS; i++) {

		for (int k = 0; k < gTotalConstrains; k++) {

			int index0 = pConstrains[k].index0;
			vec3 p1 = vec3(partical_CurrPos[index0 + 0], partical_CurrPos[index0 + 1], partical_CurrPos[index0 + 2]);

			int index1 = pConstrains[k].index1;
			vec3 p2 = vec3(partical_CurrPos[index1 + 0], partical_CurrPos[index1 + 1], partical_CurrPos[index1 + 2]);

			float rest = pConstrains[k].rest_distance;

			vec3 dis = p2 - p1;

			float len = length(dis);
			vec3 correctionVector = dis * (1 - rest / len);  //

			vec3 halfVec = correctionVector * 0.5f;  //

			int move0 = pConstrains[k].move0;
			int move1 = pConstrains[k].move1;

			if (partical_Move[move0])
				p1 = p1 + halfVec;

			if (partical_Move[move1])
				p2 = p2 - halfVec;



			partical_CurrPos[index0 + 0] = p1[0];
			partical_CurrPos[index0 + 1] = p1[1];
			partical_CurrPos[index0 + 2] = p1[2];

			partical_CurrPos[index1 + 0] = p2[0];
			partical_CurrPos[index1 + 1] = p2[1];
			partical_CurrPos[index1 + 2] = p2[2];

		}


	}

}

void CalculateUpdatedNormals(void)
{
	for (int y = 0; y < MESH_HEIGHT - 1; y++) {

		for (int x = 0; x < MESH_WIDTH - 1; x++) {

			int topLeft = (y * MESH_WIDTH * 3) + (x * 3);
			int topRight = (y * MESH_WIDTH * 3) + ((x + 1) * 3);
			int bottomLeft = ((y + 1) * MESH_WIDTH * 3) + (x * 3);
			int bottomRight = ((y + 1) * MESH_WIDTH * 3) + ((x + 1) * 3);

			CalculateTriangleNormal(topLeft, bottomLeft, topRight);
			CalculateTriangleNormal(topRight, bottomRight, bottomLeft);

		}
	}
}


void SetFixedPoints(void) {

	for (int i = 0; i < MESH_HEIGHT; i = i + 1) {

		partical_Move[i * MESH_WIDTH + 0] = 0;
	}

}


void InitVertexBuffer(void)
{
	/********** Position, Normal and Elements **********/
	LoadGrid(4.0f, 4.0f, -2.0f, 2.0f);
	memset((void*)&partical_Move, 1, sizeof(partical_Move));
	SetFixedPoints();


	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	
	// allocate memory for vertex points and colors, and load data
	glBufferData(GL_ARRAY_BUFFER, sizeof(partical_CurrPos) + sizeof(partical_Normal) + sizeof(partical_Texcoord), NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(partical_CurrPos), partical_CurrPos);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(partical_CurrPos), sizeof(partical_Normal), partical_Normal);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(partical_CurrPos) + sizeof(partical_Normal), sizeof(partical_Texcoord), partical_Texcoord);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

}



void Init_Flag(void)
{
	gProgramFlag = LinkProgramViaCode(&vertexShaderFlag, &pixelShaderFlag);
	InitVertexBuffer();

	textureName = LoadTexture("../Assets/flag.png", textureUnit);

	// get screen size, divide into two viewports with equal aspect ratios
	glfwGetWindowSize(window, &screenWidth, &screenHeight);
	float aspectRatio = (float)screenWidth / (float)screenHeight;

	// compute projection and modelview matrices
	persp = Perspective(30, aspectRatio, 0.001f, 500);

}

void Uninitialize_Flag(void)
{

}

void Display_Flag(void)
{

	void Display_FlagPost(void);

	// init shader program, set vertex feed for points and colors
	glUseProgram(gProgramFlag);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);

	VertexAttribPointer(gProgramFlag, "position", 3, 0, (void*)0);
	VertexAttribPointer(gProgramFlag, "normal", 3, 0, (void*)sizeof(partical_CurrPos));
	VertexAttribPointer(gProgramFlag, "uv", 2, 0, (void*)(sizeof(partical_CurrPos) + sizeof(partical_Normal)));

	mat4 modelview = Translate(-0.80f, 1.80f, -15.0f) * Scale(0.6f, 0.6f, 0.5f);

	//For Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureName);

	// send final view to shader
	SetUniform(gProgramFlag, "samplerTexture", textureUnit);
	SetUniform(gProgramFlag, "modelviewMatrix", modelview);
	SetUniform(gProgramFlag, "perspMatrix", persp);


	// draw shaded cube on left
	glViewport(0, 0, screenWidth, screenHeight);


	ClothAddForce(vec3(0.0f, -0.050f, 0.0f) * dtSquare);
	ClothWindForce(vec3(1.5f, 0.0f, 0.30f) * dtSquare);
	CheckForConstrains();
	ParticalMovement();



	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(partical_CurrPos), partical_CurrPos);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(partical_CurrPos), sizeof(partical_Normal), partical_Normal);

	glDrawElements(GL_TRIANGLES, sizeof(partical_Elements) / sizeof(GLuint), GL_UNSIGNED_INT, partical_Elements);
	static int i = 0;
	glPointSize(10.0);
	
	//glDrawArrays(GL_POINTS, 0, i);
	//i++;
	//if (i > (MESH_WIDTH * MESH_HEIGHT))
		//i = 0;

	Display_FlagPost();

}


void Display_FlagPost(void)
{

	GLfloat x = -2.0, y = -3.40f, z = -15.0f;

	mat4 scale = Scale(1.0, 0.2f, 1.0f);
	mat4 modelview = Translate(x, y - 0.1f, z) * scale;

	Cylinder(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 2.0f, 0.0f), 1.0f, 1.0f, modelview, persp, vec4(0.6f, 0.60f, 0.60f, 1.0f));


	scale = Scale(0.650, 0.2f, 0.650f);
	modelview = Translate(x, y + 0.2f, z) * scale;
	Cylinder(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 2.0f, 0.0f), 1.0f, 1.0f, modelview, persp, vec4(0.6f, 0.60f, 0.60f, 1.0f));

	scale = Scale(0.10, 3.0f, 0.10f);
	modelview = Translate(x, y + 0.60f, z) * scale;
	Cylinder(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 2.0f, 0.0f), 1.0f, 1.0f, modelview, persp, vec4(0.6f, 0.60f, 0.60f, 1.0f));

}



// App Display
void Display(void) {

	// clear screen, enable z-buffer
	glClearColor(.5290f, .807f, .921f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	Display_Flag();

	glFlush();
}


int main(int ac, char** av) {

	void Init_Flag(void);
	void Display(void);
	void Uninitialize_Flag(void);

	// init app window and GL context
	glfwInit();
	window = glfwCreateWindow(screenWidth, screenHeight, "INDIAN FLAG", NULL, NULL);
	glfwSetWindowPos(window, 100, 100);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);


	// init shader and GPU data
	Init_Flag();

	// callbacks
	glfwSetWindowSizeCallback(window, Resize);


	// event loop
	glfwSwapInterval(1);
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		Display();
		glfwSwapBuffers(window);
	}

	Uninitialize_Flag();
	glfwDestroyWindow(window);
	glfwTerminate();
}

