/*
 * A framework for GLSL programming in TNM084 for MT4-5
 *
 * This is based on a framework designed to be easy to understand
 * for students in a computer graphics course in the first year
 * of a M Sc curriculum. It uses custom code for some things that
 * are better solved by external libraries like GLEW and GLM, but
 * the emphasis is on simplicity and readability, not generality.
 * For the window management, GLFW 3.x is used for convenience.
 * The framework should work in Windows, MacOS X and Linux.
 * Some Windows-specific stuff for extension loading is still
 * here. GLEW could have been used for that purpose, but for clarity
 * and minimal dependence on other code, we rolled our own extension
 * loading for the things we need. That code is short-circuited on
 * platforms other than Windows. This code is dependent only on
 * the GLFW and OpenGL libraries. OpenGL 3.3 or higher is assumed.
 * Compatibility with OpenGL 3.2 would require some changes, most
 * notably because GLSL version 3.30 is used in the shaders.
 *
 * Author: Stefan Gustavson (stefan.gustavson@liu.se) 2013-2016
 * This code is in the public domain.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// In MacOS X, tell GLFW to include the modern OpenGL headers.
// Windows does not want this, so we make this Mac-only.
#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#endif

// In Linux, tell GLFW to include the modern OpenGL functions.
// Windows does not want this, so we make this Linux-only.
#ifdef __linux__
#define GL_GLEXT_PROTOTYPES
#endif

// GLFW 3.x, to handle the OpenGL window
//#include <GLFW/glfw3.h>
#include "GLFW/glfw3.h"

// In Windows, glext.h is almost never up to date, so use our local copy
#ifdef _WIN32
#include "GL/glext.h"
#endif

// Some utility classes and boilerplate code
#include "tnm084.h"
#include "tgaloader.h"
#include "triangleSoup.h"
#include "pollRotator.h"

#include <stdlib.h>
#include <time.h>

#define SHADERPATH "shader/"

#define TEXTUREFILENAME "textures/earth2048.tga"

// File names for a mesh model, a texture file and the two shaders

#define VERTEXSHADERFILEPLANET SHADERPATH "vertexshader_planet.glsl"
#define FRAGMENTSHADERFILEPLANET SHADERPATH "fragmentshader_planet.glsl"

#define VERTEXSHADERFILEWATER SHADERPATH "vertexshader_water.glsl"
#define FRAGMENTSHADERFILEWATER SHADERPATH "fragmentshader_water.glsl"

//#define VERTEXSHADERFILECLOUD SHADERPATH "vertexshader_cloud.glsl"
//#define FRAGMENTSHADERFILECLOUD SHADERPATH "fragmentshader_cloud.glsl"

/*
 * setupViewport() - set up the OpenGL viewport to handle window resizing
 */
void setupViewport(GLFWwindow* window, GLfloat *P) {

    int width, height;

    // Get current window size. The user may resize it at any time.
    glfwGetWindowSize( window, &width, &height );

    // Hack: Adjust the perspective matrix P for non-square aspect ratios
    P[0] = P[5]*height/width;

    // Set viewport. This is the pixel rectangle we want to draw into.
    glViewport( 0, 0, width, height ); // The entire window
}

/*
 * main(argc, argv) - the standard C entry point for the program
 */
int main(int argc, char* argv[]) {

	Texture texture;

	triangleSoup trianglePlanet;
	GLuint programPlanet;
	GLint location_time_planet, location_MV_planet, location_P_planet, location_tex;

	triangleSoup triangleWater;
	GLuint programPlanet, programWater, programCloud;
	GLint location_time_water, location_MV_water, location_P_water;

	triangleSoup triangleCloud;
	GLuint programPlanet, programWater, programCloud;
	GLint location_time_cloud, location_MV_cloud, location_P_cloud;

	float time;
	double fps = 0.0;

	GLFWmonitor* monitor;
	const GLFWvidmode* vidmode;  // GLFW struct to hold information on the display
	GLFWwindow* window;

	rotatorMouse rotator;

	initRotatorMouse(&rotator);

	// Initialise GLFW, bail out if unsuccessful
	if (!glfwInit()) {
		printf("Failed to initialise GLFW. Exiting.\n");
		return -1;
	}

	monitor = glfwGetPrimaryMonitor();
	vidmode = glfwGetVideoMode(monitor);

	// Make sure we are getting a GL context of precisely version 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Exclude old legacy cruft from the context. We don't want it.
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(vidmode->width / 2, vidmode->height / 2, "Hello GLSL", NULL, NULL);
	if (!window)
	{
		printf("Failed to open GLFW window. Exiting.\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	// glfwSwapInterval() doesn't seem to work in Windows 7-8-10 with NVidia cards.
	// Change the system-wide settings instead to turn vertical sync on or off.
	glfwSwapInterval(0); // Do not wait for screen refresh between frames

	// Load the extensions for GLSL - note that this has to be done
	// *after* the window has been opened, or we won't have a GL context
	// to query for those extensions and connect to instances of them.
	// (It's Microsoft Windows that forces us to do this, not OpenGL.)
	loadExtensions();

	printf("GL vendor:       %s\n", glGetString(GL_VENDOR));
	printf("GL renderer:     %s\n", glGetString(GL_RENDERER));
	printf("GL version:      %s\n", glGetString(GL_VERSION));
	printf("Desktop size:    %d x %d pixels\n", vidmode->width, vidmode->height);

	// Set up some matrices.
	GLfloat MV[16]; // Modelview matrix

	//Temporary matrices for composition of a dynamic MV
	GLfloat R1[16];
	GLfloat R2[16];

	// When sent to GLSL, a 4x4 matrix is specified as a sequence
	// of 4-vectors for the four columns. Therefore, initialization
	// in C code actually looks like the transpose of the matrix.
	GLfloat Tz[16] = {
	  1.0f, 0.0f, 0.0f, 0.0f,  // First column
	  0.0f, 1.0f, 0.0f, 0.0f,  // Second column
	  0.0f, 0.0f, 1.0f, 0.0f,  // Third column
	  0.0f, 0.0f, -5.0f, 1.0f   // Fourth column
	};

	// Perspective projection matrix
	// This is the standard gluPerspective() form of the
	// matrix, with d=4, near=3, far=7 and aspect=1.
	// ("near" and "far" are very close together for this demo)
	GLfloat P[16] = {
		4.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 4.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -2.5f, -1.0f,
		0.0f, 0.0f, -10.5f, 0.0f
	};

	// Create geometry for rendering
	soupInit(&trianglePlanet); // Initialize all fields to zero
	soupCreateSphere(&trianglePlanet, 1.0, 50); // A latitude-longitude sphere mesh
	soupPrintInfo(trianglePlanet);

	soupInit(&triangleWater); // Initialize all fields to zero
	soupCreateSphere(&triangleWater, 1.0, 50); // A latitude-longitude sphere mesh
	soupPrintInfo(triangleWater);

	soupInit(&triangleCloud); // Initialize all fields to zero
	soupCreateSphere(&triangleCloud, 1.3, 50); // A latitude-longitude sphere mesh
	soupPrintInfo(triangleCloud);

	// Enable texturing, in case it's not already the default
	glEnable(GL_TEXTURE_2D);
	// Load a texture from a TGA file
	createTexture(&texture, TEXTUREFILENAME);

	// Create a shader program object from GLSL code in two files
	programPlanet = createShader(VERTEXSHADERFILEPLANET, FRAGMENTSHADERFILEPLANET);
	programWater = createShader(VERTEXSHADERFILEWATER, FRAGMENTSHADERFILEWATER);
	//programCloud = createShader(VERTEXSHADERFILECLOUD, FRAGMENTSHADERFILECLOUD);

	// Get the uniform locations for the things we want to change during runtime
	location_MV_planet = glGetUniformLocation(programPlanet, "MV");
	location_P_planet = glGetUniformLocation(programPlanet, "P");
	location_time_planet = glGetUniformLocation(programPlanet, "time");
	location_tex = glGetUniformLocation(programPlanet, "tex");

	location_MV_water = glGetUniformLocation(programWater, "MV");
	location_P_water = glGetUniformLocation(programWater, "P");
	location_time_water = glGetUniformLocation(programWater, "time");

	location_MV_cloud = glGetUniformLocation(programWater, "MV");
	location_P_cloud = glGetUniformLocation(programWater, "P");
	location_time_cloud = glGetUniformLocation(programWater, "time");

	// Main loop: render frames until the program is terminated
	while (!glfwWindowShouldClose(window))
	{
		// Calculate and update the frames per second (FPS) display
		fps = computeFPS(window);

		// Set the background RGBA color, and clear the buffers for drawing
		glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Set up the viewport
		setupViewport(window, P);

		// Handle mouse input to rotate the view
		pollRotatorMouse(window, &rotator);
		//printf("phi = %6.2f, theta = %6.2f\n", rotator.phi, rotator.theta);
		{
			// Activate our shader program.
			glUseProgram(programPlanet);

			// Tell the shader that we are using texture unit 0
			if (location_tex != -1) {
				glUniform1i(location_tex, 0);
			}

			// Update the uniform time variable
			if (location_time != -1) {
				time = (float)glfwGetTime();
				glUniform1f(location_time, time);
			}

			// Modify MV according to user input
			mat4roty(R1, rotator.phi * M_PI / 180.0);
			mat4rotx(R2, rotator.theta * M_PI / 180.0);
			mat4mult(R2, R1, MV);
			mat4mult(Tz, MV, MV);
			// mat4print(MV);

			// Update the transformation matrix MV, a uniform variable
			if (location_MV_planet != -1) {
				glUniformMatrix4fv(location_MV_planet, 1, GL_FALSE, MV);
			}

			// Update the perspective projection matrix P
			if (location_P_planet != -1) {
				glUniformMatrix4fv(location_P_planet, 1, GL_FALSE, P);
			}

			// Draw the scene
			glEnable(GL_DEPTH_TEST); // Use the Z buffer
			glEnable(GL_CULL_FACE);  // Use back face culling
			glCullFace(GL_BACK);
			//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

			// Render the geometry
			soupRender(trianglePlanet);

			// Play nice and deactivate the shader program
			glUseProgram(0);
		}

		{
			// Activate our shader program.
			glUseProgram(programWater);

			// Update the uniform time variable
			if (location_time_water != -1) {
				time = (float)glfwGetTime();
				glUniform1f(location_time_water, time);
			}

			// Modify MV according to user input
			mat4roty(R1, rotator.phi * M_PI / 180.0);
			mat4rotx(R2, rotator.theta * M_PI / 180.0);
			mat4mult(R2, R1, MV);
			mat4mult(Tz, MV, MV);
			// mat4print(MV);

			// Update the transformation matrix MV, a uniform variable
			if (location_MV_water != -1) {
				glUniformMatrix4fv(location_MV_water, 1, GL_FALSE, MV);
			}

			// Update the perspective projection matrix P
			if (location_P_water != -1) {
				glUniformMatrix4fv(location_P_water, 1, GL_FALSE, P);
			}

			// Draw the scene
			glEnable(GL_DEPTH_TEST); // Use the Z buffer
			glEnable(GL_CULL_FACE);  // Use back face culling
			glCullFace(GL_BACK);
			//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

			// Render the geometry
			soupRender(trianglePlanet);

			// Play nice and deactivate the shader program
			glUseProgram(0);
		}

		/*
				{
			// Activate our shader program.
			glUseProgram(programCloud);

			// Tell the shader that we are using texture unit 0
			if (location_tex_w != -1) {
				glUniform1i(location_tex_w, 0);
			}

			// Update the uniform time variable
			if (location_time_w != -1) {
				time = (float)glfwGetTime();
				glUniform1f(location_time_w, time);
			}

			// Modify MV according to user input
			mat4roty(R1, rotator.phi * M_PI / 180.0);
			mat4rotx(R2, rotator.theta * M_PI / 180.0);
			mat4mult(R2, R1, MV);
			mat4mult(Tz, MV, MV);
			// mat4print(MV);

			// Update the transformation matrix MV, a uniform variable
			if (location_MV_w != -1) {
				glUniformMatrix4fv(location_MV_w, 1, GL_FALSE, MV);
			}

			// Update the perspective projection matrix P
			if (location_P_w != -1) {
				glUniformMatrix4fv(location_P_w, 1, GL_FALSE, P);
			}

			// Draw the scene
			glEnable(GL_DEPTH_TEST); // Use the Z buffer
			glEnable(GL_CULL_FACE);  // Use back face culling
			glCullFace(GL_BACK);
			//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

			// Render the geometry
			soupRender(triangleCloud);

			// Play nice and deactivate the shader program
			glUseProgram(0);
		}
		*/

		// Swap buffers, i.e. display the image and prepare for next frame.
		glfwSwapBuffers(window);

		// Make sure GLFW takes the time to process keyboard and mouse input
		glfwPollEvents();

		// Reload and recompile the shader program if the spacebar is pressed.
		if (glfwGetKey(window, GLFW_KEY_SPACE)) {
			glDeleteProgram(programPlanet);
			programPlanet = createShader(VERTEXSHADERFILEPLANET, FRAGMENTSHADERFILEPLANET);
			glDeleteProgram(programWater);
			programWater = createShader(VERTEXSHADERFILEWATER, FRAGMENTSHADERFILEWATER);
			//glDeleteProgram(programCloud);
			//programCloud = createShader(VERTEXSHADERFILECLOUD, FRAGMENTSHADERFILECLOUD);
		}

		// Exit the program if the ESC key is pressed.
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	// Close the OpenGL window and terminate GLFW.
	glfwDestroyWindow(window);
	glfwTerminate();

	// Exit gracefully
	return 0;
}
