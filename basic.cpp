// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "shader.hpp"
#include "texture.hpp"
#include "controls.hpp"
#include "objloader.hpp"
#include "vboindexer.hpp"

#define WORLD_SIZE_Y 100
#define WORLD_SIZE_X 100

int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Tutorial 09 - Rendering several models", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
    
	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

	// Dark blue background
	glClearColor(1, 1, 1, 1);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "StandardShading.vertexshader", "StandardShading.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the texture
	GLuint Texture = loadDDS("uvmap.DDS");
	
	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ("box.obj", vertices, uvs, normals);
	if (!res)
	{
		return -1;
	}

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	// Load it into a VBO

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0] , GL_STATIC_DRAW);

	// Get a handle for our "LightPosition" uniform
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	GLuint LightPowerID = glGetUniformLocation(programID, "LightPower");

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;

	int world[WORLD_SIZE_Y][WORLD_SIZE_X];
	int temp_world[WORLD_SIZE_Y][WORLD_SIZE_X];
	int i, j;
	for (i = 0; i < WORLD_SIZE_Y; i++) {
		for (j = 0; j < WORLD_SIZE_X; j++) {
			world[i][j] = (rand() % 2 == 0) ? 1 : 0;
			temp_world[i][j] = 0;
		}
	}

	int should_update = 0;

	do{

		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if ( currentTime - lastTime >= 0.0625 ){ // If last prinf() was more than 1sec ago
			// printf and reset
			/*printf("%f ms/frame\n", 1000.0/double(nbFrames));
			nbFrames = 0;*/
			lastTime += 0.0625;
			should_update = 1;
		}


		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		
		
		////// Start of the rendering of the first object //////
		
		// Use our shader
		glUseProgram(programID);
	
		glm::vec3 lightPos = glm::vec3(4,4,4);
		glUniform3f(LightID, getPosition().x, getPosition().y, getPosition().z);
		glUniform1f(LightPowerID, getBrightness());
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]); // This one doesn't change between objects, so this can be done once for all objects that use "programID"

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to user Texture Unit 0
		glUniform1i(TextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);


		if (should_update) {

		// Loop variables
		int row, column;

		// Binary int to check if any cells are alive
		int isOneAlive = 0;

		for (row = 0; row < WORLD_SIZE_Y; row++) {
			for (column = 0; column < WORLD_SIZE_X; column++) {

				// Get the cell (we know if it equals 1 then it's alive)
				int isCellAlive = world[row][column];

				if (isCellAlive) {
					isOneAlive = 1;
				}


				/**/
				// Get bounding box of coordinates
				int topY = (row - 1 + WORLD_SIZE_Y) % WORLD_SIZE_Y;
				int bottomY = (row + 1) % WORLD_SIZE_Y;
				int leftX = (column - 1 + WORLD_SIZE_X) % WORLD_SIZE_X;
				int rightX = (column + 1) % WORLD_SIZE_X;

				// Set up variable to keep track of how many cells are alive
				int totalAliveNeighbors = 0;

				// Get the 3x3 bounding box around the current cell
				totalAliveNeighbors += world[topY][leftX];
				totalAliveNeighbors += world[topY][column];
				totalAliveNeighbors += world[topY][rightX];

				totalAliveNeighbors += world[row][leftX];
				totalAliveNeighbors += world[row][rightX];

				totalAliveNeighbors += world[bottomY][leftX];
				totalAliveNeighbors += world[bottomY][column];
				totalAliveNeighbors += world[bottomY][rightX];
				/**/

				// If the cell is alive and has 0 or 1 live neighbors, it dies.
				if (isCellAlive && (totalAliveNeighbors == 0 || totalAliveNeighbors == 1)) {
					temp_world[row][column] = 0;
					continue;
				}

				// If the cell is alive and has 2 or 3 live neighbors, it lives on.
				if (isCellAlive && (totalAliveNeighbors == 2 || totalAliveNeighbors == 3)) {
					temp_world[row][column] = 1;
					continue;
				}

				// If the cell is alive and has 4 live neighbors, it dies.
				if (isCellAlive && (totalAliveNeighbors >= 4)) {
					temp_world[row][column] = 0;
					continue;
				}

				// If the cell is dead and has exactly 3 live neighbors, it becomes alive.
				if (!isCellAlive && (totalAliveNeighbors == 3)) {
					temp_world[row][column] = 1;
					continue;
				}

			}
		}


		for (i = 0; i < WORLD_SIZE_Y; i++) {
			for (j = 0; j < WORLD_SIZE_X; j++) {
				world[i][j] = temp_world[i][j];
				
			}
		}

		should_update = 0;

	}

	for (i = 0; i < WORLD_SIZE_Y; i++) {
		for (j = 0; j < WORLD_SIZE_X; j++) {
			if (world[i][j])
			{

				// render a cube in this location
				glm::mat4 ModelMatrix2 = glm::mat4(1.0);
				ModelMatrix2 = glm::translate(ModelMatrix2, glm::vec3(j * 2, i * 2, 0.0f));
				glm::mat4 MVP2 = ProjectionMatrix * ViewMatrix * ModelMatrix2;

				glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP2[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix2[0][0]);

				glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, (void*)0);

			}
		}
	}

			

	// If no live cells are found, return -1
	/*if (!isOneAlive) {
		return -1;
	}*/




		/*int i;
		for (i = 0; i < 10; i++)
		{
			glm::mat4 ModelMatrix2 = glm::mat4(1.0);
			ModelMatrix2 = glm::translate(ModelMatrix2, glm::vec3(0.0f, i * 4, 0.0f));
			glm::mat4 MVP2 = ProjectionMatrix * ViewMatrix * ModelMatrix2;

			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP2[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix2[0][0]);

			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, (void*)0);
		}*/

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

