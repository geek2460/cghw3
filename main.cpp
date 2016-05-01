#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "tiny_obj_loader.h"
#include <cmath>

#define GLM_FORCE_RADIANS
#define PI 3.145926
#define OBJ_SIZE 6
static unsigned int setup_shader(const char *vertex_shader, const char *fragment_shader);
static std::string readfile(const char *filename);
static int add_obj(unsigned int program, const char *filename,const char *texbmp);
static void render();

struct object_struct{
	unsigned int program;
	unsigned int vao;
	unsigned int vbo[4];
	unsigned int texture;
	glm::mat4 model;
	object_struct(): model(glm::mat4(1.0f)){}
} ;

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// Camera
glm::vec3 cameraPos   = glm::vec3(0.0f, 15.0f,  40.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -15.0f, -40.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
GLfloat yaw    = -90.0f;	// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (due to how Eular angles work) so we initially rotate a bit to the left.
GLfloat pitch  =  0.0f;
GLfloat lastX  =  WIDTH  / 2.0;
GLfloat lastY  =  HEIGHT / 2.0;
GLfloat fov =  45.0f;
bool keys[1024];
// Deltatime
GLfloat delta = 0.0f;	// Time between current frame and last frame in 1 sec for calculate FPS
GLfloat last = 0.0f,start = 0.0f;  	// Time of last frame in 1 sec for calculate FPS

GLfloat deltaframe = 0.0f;   // Time between current frame and last frame no delay to calculate animate movement
GLfloat lastframe = 0.0f;       // Time of last frame no delay  to calculate animate movement



std::vector<object_struct> objects;//vertex array object,vertex buffer object and texture(color) for objs
unsigned int program[OBJ_SIZE];
std::vector<int> indicesCount;//Number of indice of objs

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

//key in event to set corresponse label
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

//move camera
void do_movement()
{
	// Camera controls
	GLfloat cameraSpeed = 5.0f*deltaframe;
	if (keys[GLFW_KEY_W])
		cameraPos.y -= cameraSpeed * cameraFront.y/5;
	if (keys[GLFW_KEY_S])
		cameraPos.y += cameraSpeed * cameraFront.y/5;
	if (keys[GLFW_KEY_A])
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (keys[GLFW_KEY_D])
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (keys[GLFW_KEY_C]){
	}
	

}

//zoom in/out camera
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (fov >= 1.0f && fov <= 45.0f)
		fov -= yoffset*deltaframe;
	if (fov <= 1.0f)
		fov = 1.0f;
	if (fov >= 45.0f)
		fov = 45.0f;
}


static unsigned int setup_shader(const char *vertex_shader, const char *fragment_shader)
{
	GLuint vs=glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, (const GLchar**)&vertex_shader, nullptr);

	glCompileShader(vs);

	int status, maxLength;
	char *infoLog=nullptr;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
	if(status==GL_FALSE)
	{
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];

		glGetShaderInfoLog(vs, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Vertex Shader Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}

	GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, (const GLchar**)&fragment_shader, nullptr);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	if(status==GL_FALSE)
	{
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];

		glGetShaderInfoLog(fs, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Fragment Shader Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}

	unsigned int program=glCreateProgram();
	// Attach our shaders to our program
	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if(status==GL_FALSE)
	{
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);


		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];
		glGetProgramInfoLog(program, maxLength, NULL, infoLog);

		glGetProgramInfoLog(program, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Link Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}
	return program;
}

static std::string readfile(const char *filename)
{
	std::ifstream ifs(filename);
	if(!ifs)
		exit(EXIT_FAILURE);
	return std::string( (std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>()));
}

// mini bmp loader written by HSU YOU-LUN
static unsigned char *load_bmp(const char *bmp, unsigned int *width, unsigned int *height, unsigned short int *bits)
{
	unsigned char *result=nullptr;
	FILE *fp = fopen(bmp, "rb");
	if(!fp)
		return nullptr;
	char type[2];
	unsigned int size, offset;
	// check for magic signature	
	fread(type, sizeof(type), 1, fp);
	if(type[0]==0x42 || type[1]==0x4d){
		fread(&size, sizeof(size), 1, fp);
		// ignore 2 two-byte reversed fields
		fseek(fp, 4, SEEK_CUR);
		fread(&offset, sizeof(offset), 1, fp);
		// ignore size of bmpinfoheader field
		fseek(fp, 4, SEEK_CUR);
		fread(width, sizeof(*width), 1, fp);
		fread(height, sizeof(*height), 1, fp);
		// ignore planes field
		fseek(fp, 2, SEEK_CUR);
		fread(bits, sizeof(*bits), 1, fp);
		unsigned char *pos = result = new unsigned char[size-offset];
		fseek(fp, offset, SEEK_SET);
		while(size-ftell(fp)>0)
			pos+=fread(pos, 1, size-ftell(fp), fp);
	}
	fclose(fp);
	return result;
}

static int add_obj(unsigned int program, const char *filename,const char *texbmp)
{
	object_struct new_node;

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, filename);

	if (!err.empty()||shapes.size()==0)
	{
		std::cerr<<err<<std::endl;
		exit(1);
	}

	glGenVertexArrays(1, &new_node.vao);
	glGenBuffers(4, new_node.vbo);
	glGenTextures(1, &new_node.texture);

	glBindVertexArray(new_node.vao);

	// Upload postion array
	glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.positions.size(),
			shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	if(shapes[0].mesh.texcoords.size()>0)
	{

		// Upload texCoord array
		glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.texcoords.size(),
				shapes[0].mesh.texcoords.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindTexture( GL_TEXTURE_2D, new_node.texture);
		unsigned int width, height;
		unsigned short int bits;
		unsigned char *bgr=load_bmp(texbmp, &width, &height, &bits);
		GLenum format = (bits == 24? GL_BGR: GL_BGRA);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, bgr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glGenerateMipmap(GL_TEXTURE_2D);
		delete [] bgr;
	}

	if(shapes[0].mesh.normals.size()>0)
	{
		// Upload normal array
		glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.normals.size(),
				shapes[0].mesh.normals.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	// Setup index buffer for glDrawElements
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_node.vbo[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*shapes[0].mesh.indices.size(),
			shapes[0].mesh.indices.data(), GL_STATIC_DRAW);

	indicesCount.push_back(shapes[0].mesh.indices.size());

	glBindVertexArray(0);

	new_node.program = program;

	objects.push_back(new_node);
	return objects.size()-1;
}

static void releaseObjects()
{
	for(int i=0;i<objects.size();i++){
		glDeleteVertexArrays(1, &objects[i].vao);
		glDeleteTextures(1, &objects[i].texture);
		glDeleteBuffers(4, objects[i].vbo);
		glDeleteProgram(program[i]);
	}
}

static void setUniformMat4(unsigned int program, const std::string &name, const glm::mat4 &mat)
{
	// This line can be ignore. But, if you have multiple shader program
	// You must check if currect binding is the one you want
	glUseProgram(program);
	GLint loc=glGetUniformLocation(program, name.c_str());
	if(loc==-1) return;

	// mat4 of glm is column major, same as opengl
	// we don't need to transpose it. so..GL_FALSE
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

static void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for(int i=0;i<objects.size();i++){
		glUseProgram(objects[i].program);
		glBindVertexArray(objects[i].vao);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture);
		//you should send some data to shader here
		glDrawElements(GL_TRIANGLES, indicesCount[i], GL_UNSIGNED_INT, nullptr);
	}
	glBindVertexArray(0);
}

int main(int argc, char *argv[])
{
	//input 1.flat 2.Gouraud 3.Phong 4.Blinn-Phong
	int n = 0;
	
	std::cout<<"input 1).flat 2).Gouraud 3).Phong 4).Blinn-Phong to choose a shader : ";
	
	while(std::cin>>n)
	{
		if(n>4 || n<1)
			std::cout<<"input 1).flat 2).Gouraud 3).Phong 4).Blinn-Phong to choose a shader : ";
		else
			break;
		
	}
	
	
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	// OpenGL 3.3, Mac OS X is reported to have some problem. However I don't have Mac to test
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// For Mac OS X
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(800, 600, "Simple Example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);

	// This line MUST put below glfwMakeContextCurrent
	glewExperimental = GL_TRUE;
	glewInit();

	// Enable vsync
	glfwSwapInterval(1);

	// Setup input callback
	glfwSetKeyCallback(window, key_callback);
	//glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	
	// load shader program
	switch(n)
	{
		case 1://flat shading
			for(int i=0; i < OBJ_SIZE ; i++)
				program[i] = setup_shader(readfile("vs_flat.txt").c_str(), readfile("fs_flat.txt").c_str());
			break;
		case 2://2.Gouraud 
			for(int i=0; i < OBJ_SIZE ; i++)
				program[i] = setup_shader(readfile("vs_Gouraud.txt").c_str(), readfile("fs_Gouraud.txt").c_str());
			break;
		case 3://3.Phong 
			for(int i=0; i < OBJ_SIZE ; i++)
				program[i] = setup_shader(readfile("vs_Phong.txt").c_str(), readfile("fs_Phong.txt").c_str());
			break;
		case 4://4.Blinn-Phong
			for(int i=0; i < OBJ_SIZE ; i++)
				program[i] = setup_shader(readfile("vs_BlinnPhong.txt").c_str(), readfile("fs_BlinnPhong.txt").c_str());
			break;
		default:
			for(int i=0; i < OBJ_SIZE ; i++)
				program[i] = setup_shader(readfile("vs_flat.txt").c_str(), readfile("fs_flat.txt").c_str());
			
			
	}
	int sun = add_obj(program[0], "obj_texture/sun.obj","obj_texture/sun.bmp");
	int earth = add_obj(program[1], "obj_texture/earth.obj","obj_texture/earth.bmp");
	int moon = add_obj(program[2], "obj_texture/moon.obj","obj_texture/moon.bmp");
	int mercury = add_obj(program[3], "obj_texture/mercury.obj","obj_texture/mercury.bmp");
	int venus = add_obj(program[4], "obj_texture/venus.obj","obj_texture/venus.bmp");
	int mars = add_obj(program[5], "obj_texture/mars.obj","obj_texture/mars.bmp");

	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	// Enable blend mode for billboard
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glm::mat4 rev;

	last = start = glfwGetTime();
	int fps=0;

	while (!glfwWindowShouldClose(window))
	{//program will keep draw here until you close the window

		
		GLfloat currentFrame = glfwGetTime();
		deltaframe = currentFrame - lastframe;
		lastframe = currentFrame;

		render();
		glfwSwapBuffers(window);
		glfwPollEvents();

		//show the FPS
		delta = glfwGetTime() - start;
		fps++;
		if(glfwGetTime() - last > 1.0)
		{
			std::cout<<(double)fps/(glfwGetTime()-last)<<std::endl;
			fps = 0;
			last = glfwGetTime();
		}
		do_movement();

		//set camera/view transform
		glm::mat4 view;
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		// Projection 
		glm::mat4 projection;
		projection = glm::perspective(fov, (GLfloat)WIDTH/(GLfloat)HEIGHT, 0.1f, 100.0f);  
		//lamb positions
		glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
		for(int i=0;i<objects.size();i++)
		{
			setUniformMat4(program[i], "view", view);
			setUniformMat4(program[i], "projection",projection);
			
			//set ambient uniform
			GLint lightColorLoc  = glGetUniformLocation(program[i], "lightColor");
			glUniform3f(lightColorLoc,  1.0f,1.0f,1.0f);
			GLint lightPosLoc = glGetUniformLocation(program[i], "lightPos");
			glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);  
			GLint viewPosLoc = glGetUniformLocation(program[i], "viewPos");
			glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);  
		}
		//directly set sun's ambient value lighter
		glUseProgram(program[0]);
		GLint loc=glGetUniformLocation(program[0], "lightColor");
		glUniform3f(loc,  10.0f,10.0f,10.0f);
		
		
		//counter-clock wise direction
		GLfloat angle = -(float)glfwGetTime()*PI/180.0f*50;

		//sun movement matrix
		objects[sun].model =  glm::rotate(objects[sun].model,-angle/5,glm::vec3(0.0f,1.0f,0.0f));

		//earth movement matrix
		objects[earth].model =  glm::translate(objects[earth].model,glm::vec3(20.0f*cos(angle),0.0f,10.0f*sin(angle)));
		objects[earth].model =  glm::rotate(objects[earth].model,10.0f,glm::vec3(0.0f,0.0f,1.0f));
		objects[earth].model =  glm::rotate(objects[earth].model,angle*10,glm::vec3(0.0f,1.0f,0.0f));

		//moon movement matrix
		objects[moon].model =  glm::translate(objects[moon].model,glm::vec3(2.0f*cos(angle*10),sin(angle*10)/2,2.0f*sin(angle*10)));
		objects[moon].model =  glm::translate(objects[moon].model,glm::vec3(20.0f*cos(angle),0.0f,10.0f*sin(angle)));
		objects[moon].model =  glm::rotate(objects[moon].model,-angle*10,glm::vec3(0.0f,1.0f,0.0f));

		//mercury movement
		objects[mercury].model =  glm::translate(objects[mercury].model,glm::vec3(10.0f*cos(angle*2+90),3.0f*sin(angle*2+90),5.0f*sin(angle*2+90)));
		objects[mercury].model = glm::rotate(objects[mercury].model,angle*5,glm::vec3(0.0f,1.0f,0.0f));
		
		//venus movement
		objects[venus].model =  glm::translate(objects[venus].model,glm::vec3(-3+15.0f*cos(angle*2.0f-45),2.0f*sin(angle*2.0f-45),10.0f*sin(angle*2.0f-45)));
		objects[venus].model = glm::rotate(objects[venus].model,angle*10,glm::vec3(0.0f,1.0f,0.0f));

		//mars movement
		objects[mars].model =  glm::translate(objects[mars].model,glm::vec3(-5+25.0f*cos(angle+45),2.0f*sin(angle+45),20.0f*sin(angle+45)));
		objects[mars].model = glm::rotate(objects[mars].model,angle*10,glm::vec3(0.0f,1.0f,0.0f));

		//sent model matrix to shader program
		for(int i=0;i<objects.size();i++)
		{
			setUniformMat4(objects[i].program, "model",  objects[i].model);
			objects[i].model = glm::mat4();
		}



	}

	releaseObjects();
	glfwDestroyWindow(window);
	glfwTerminate();
	return EXIT_SUCCESS;
}
