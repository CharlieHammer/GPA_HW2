#include "../Externals/Include/Common.h"

enum MenuOptions {
	MENU_NOFILTER,
	MENU_ABSTRACT,
	MENU_WATERCOLOR,
	MENU_MAGNIFIER,
	MENU_BLOOM,
	MENU_PIXEL,
	MENU_SINE_WAVE,
	MENU_NORMAL,
	MENU_TIMER_START,
	MENU_TIMER_STOP,
	MENU_EXIT
};

//filter parameter
int filterType = 0;

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;
float offset = 0.0;

using namespace glm;
using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
// current window size
int screenWidth = WINDOW_WIDTH, screenHeight = WINDOW_HEIGHT;

typedef struct {
	int width;
	int height;
	unsigned char* data;
} Texture_data;

typedef struct
{
	GLuint diffuse_Tex;
} Material;

typedef struct
{
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialID;
} Shape;

struct model
{
	vec3 position = vec3(0, 0, 0);
	vec3 scale = vec3(1, 1, 1);
	vec3 rotation = vec3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

//camera parameter
struct camera
{
	vec3 position;
	vec3 center;
	vec3 up_vector;
};
camera main_camera;
float camera_speed = 0.1f;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
};
project_setting proj;

int cur_idx = 0; // represent which model should be rendered now
string model_path = "../sponza/sponza.obj";
string noiseTexture_path = "../Assets/noise_texture.png";

GLuint noiseTexture;

GLuint program;
GLuint program2;
GLuint bar_program;

GLuint iLocP;// program
GLuint iLocMV;// program
GLuint texLoc;// program
GLuint isNormalLoc;
GLuint texLoc2;// program 2
GLuint barPosLoc;// program 2
GLuint filterTypeLoc;// program 2
GLuint offsetLoc;// program 2
GLuint iMouseLoc;// program 2
GLuint barColorLoc;// bar_program


//Mouse Parameter

//record the position of the last mouse click
int lastMouseX = 400;
int lastMouseY = 300;
vec2 mouseXY = vec2(0.0, 0.0);

//rotate Parameter
float Yaw = 0.0;
float Pitch = 0.0;

//whether use normal color
bool isNormal = false;
bool isMagnifier = false;

//MVP parameter
mat4 project_matrix;
mat4 view_matrix;
mat4 model_matrix = mat4(1.0);
mat4 mv_matrix;

vec3 model_position = vec3(0, 0, 0);
vector<Material> allMaterials;
vector<Shape> allShapes;

//frame buffer parameter
GLuint frame_vao;
GLuint frame_vbo;
GLuint fbo;
GLuint depthrbo;
GLuint fboDataTexture;

//bar parameter
GLuint bar_vao;
GLuint bar_vbo;
float bar_position = 0.0f;
const float BAR_WIDTH = 0.005f;
bool isBarMove = false;

char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char** srcp = new char* [1];
	srcp[0] = src;
	return srcp;
}

void freeShaderSource(char** srcp)
{
	delete[] srcp[0];
	delete[] srcp;
}

GLuint setShaders(string vs_path, string fs_path)
{
	GLuint v, f, p;
	char** vs = NULL;
	char** fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = loadShaderSource(vs_path.c_str());
	fs = loadShaderSource(fs_path.c_str());

	glShaderSource(v, 1, (const GLchar**)&vs[0], NULL);
	glShaderSource(f, 1, (const GLchar**)&fs[0], NULL);

	freeShaderSource(vs);
	freeShaderSource(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p, f);
	glAttachShader(p, v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	if (success)
		glUseProgram(p);
	else
	{
		system("pause");
		exit(123);
	}

	return p;
}

void initParameter()
{
	proj.nearClip = 0.001;
	proj.farClip = 1000.0;
	proj.fovy = 80;
	proj.aspect = (float)(WINDOW_WIDTH) / (float)WINDOW_HEIGHT; // adjust width for side by side view

	main_camera.position = vec3(0.0f, 0.0f, 2.0f);
	main_camera.center = vec3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = vec3(0.0f, 1.0f, 0.0f);
}

void setUniformVariables()
{
	iLocP = glGetUniformLocation(program, "um4p");
	iLocMV = glGetUniformLocation(program, "um4mv");
	texLoc = glGetUniformLocation(program, "tex");
	isNormalLoc = glGetUniformLocation(program, "isNormal");
	texLoc2 = glGetUniformLocation(program2, "tex");
	barPosLoc = glGetUniformLocation(program2, "barPos");
	filterTypeLoc = glGetUniformLocation(program2, "filterType");
	offsetLoc = glGetUniformLocation(program2, "offset");
	iMouseLoc = glGetUniformLocation(program2, "iMouse");
	barColorLoc = glGetUniformLocation(bar_program, "barColor");
}

//load model function

static string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

Texture_data LoadTextureImage(string path)
{
	Texture_data texture;
	const char* Path = path.c_str();
	int n;
	stbi_set_flip_vertically_on_load(true); // verticalmirror image data
	stbi_uc* data = stbi_load(Path, &texture.width, &texture.height,
		&n, 4);
	int data_size = texture.width * texture.height * 4;
	if (data != NULL)
	{
		texture.data = new unsigned char[data_size];
		memcpy(texture.data, data, data_size * sizeof(unsigned char));
		stbi_image_free(data);
	}
	else {
		cout << "failed to load " << path << endl;
	}
	return texture;

}

void LoadTexturedModels(string model_path)
{
	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(model_path.c_str(),aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
	
	printf("Load Models Success ! Shapes size %d Material size %d\n", scene->mNumMeshes, scene->mNumMaterials);

	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* material = scene->mMaterials[i];
		Material Material;
		aiString texturePath;
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS)
		{
			// load width, height and data from texturePath.C_Str()
			Texture_data texture_data = LoadTextureImage(base_dir + texturePath.C_Str());
			glGenTextures(1, &Material.diffuse_Tex);
			glBindTexture(GL_TEXTURE_2D, Material.diffuse_Tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_data.width, texture_data.height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, texture_data.data);
			glGenerateMipmap(GL_TEXTURE_2D);
			cout << "Material " << i << " texture load successfully" << endl;
		}
		else{
			// load some default image as default_diffuse_tex
			cout << "Material " << i << " has no diffuse texture" << endl;
		}
		allMaterials.push_back(Material);
	}
	//todo
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[i];
		Shape shape;
		vector<GLfloat> positions;
		vector<GLfloat> normals;
		vector<GLfloat> texcoords;
		vector<unsigned int> indices;
		//set the material index
		glGenVertexArrays(1, &shape.vao);
		glBindVertexArray(shape.vao);
		// create 3 vbos to hold data
		glGenBuffers(1, &shape.vbo_position);
		glGenBuffers(1, &shape.vbo_normal);
		glGenBuffers(1, &shape.vbo_texcoord);
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
		{
			positions.push_back(mesh->mVertices[v][0]);
			positions.push_back(mesh->mVertices[v][1]);
			positions.push_back(mesh->mVertices[v][2]);
			normals.push_back(mesh->mNormals[v][0]);
			normals.push_back(mesh->mNormals[v][1]);
			normals.push_back(mesh->mNormals[v][2]);
			texcoords.push_back(mesh->mTextureCoords[0][v][0]);
			texcoords.push_back(mesh->mTextureCoords[0][v][1]);
		}
		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_position);
		glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), &positions[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_texcoord);
		glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat), &texcoords[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), &normals[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		// create 1 ibo to hold data
		glGenBuffers(1, &shape.ibo);
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f){
			indices.push_back(mesh->mFaces[f].mIndices[0]);
			indices.push_back(mesh->mFaces[f].mIndices[1]);
			indices.push_back(mesh->mFaces[f].mIndices[2]);
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		// glVertexAttribPointer / glEnableVertexArray calls¡K
		shape.materialID = mesh->mMaterialIndex;
		shape.drawCount = mesh->mNumFaces * 3;
		// save shape¡K
		allShapes.push_back(shape);
	}

	//aiReleaseImport(scene);
}

void LoadNoiseTexture(string noiseTex_path) {
	Texture_data texture_data = LoadTextureImage(noiseTex_path.c_str());
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_data.width, texture_data.height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, texture_data.data);
	glGenerateMipmap(GL_TEXTURE_2D);

	cout << "load noise texture successfully" << endl;
}

void setFBOVertex() {
	glGenVertexArrays(1, &frame_vao);
	glBindVertexArray(frame_vao);
	static const GLfloat window_vertex[] =
	{
		1.0f, -1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};
	glGenBuffers(1, &frame_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, frame_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(window_vertex), window_vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (const GLvoid*)(sizeof(GL_FLOAT) * 2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenFramebuffers(1, &fbo);

	cout << "set FBO's vertex sucessfully" << endl;
}

void setCompareBar() {

	glGenVertexArrays(1, &bar_vao);

	glUseProgram(bar_program);
	glUniform3f(barColorLoc, 0.5f, 0.5f, 0.5f);

	glBindVertexArray(bar_vao);
	float left = -BAR_WIDTH + bar_position;
	float right = BAR_WIDTH + bar_position;
	const GLfloat bar_vertex[] =
	{
		left, -1.0f, 0.0f,
		right, -1.0f, 0.0f,
		right, 1.0f, 0.0f,
		left, 1.0f, 0.0f
	};
	glGenBuffers(1, &bar_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, bar_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bar_vertex), bar_vertex, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	//set the uniform
	glUseProgram(program2);
	glUniform1f(barPosLoc, (bar_position / 2 + 0.5));

	cout << "set comparision bar sucessfully" << endl;
}

void changeBarLoc(){
	glBindVertexArray(bar_vao);
	float left = -BAR_WIDTH + bar_position;
	float right = BAR_WIDTH + bar_position;
	const GLfloat bar_vertex[] =
	{
		left, -1.0f, 0.0f, 
		right, -1.0f, 0.0f,
		right, 1.0f, 0.0f, 
		left, 1.0f, 0.0f
	};
	glBindBuffer(GL_ARRAY_BUFFER, bar_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bar_vertex), bar_vertex);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	//set the uniform
	glUseProgram(program2);
	glUniform1f(barPosLoc, (bar_position / 2 + 0.5));
}

void setupRC()
{
	// setup shaders
	program = setShaders("../Assets/vertex.vs.glsl", "../Assets/fragment.fs.glsl");
	program2 = setShaders("../Assets/vertex2.vs.glsl", "../Assets/fragment2.fs.glsl");
	bar_program = setShaders("../Assets/bar_vertex.vs.glsl", "../Assets/bar_fragment.fs.glsl");
	initParameter();
	setUniformVariables();

	//load the model
	LoadTexturedModels(model_path);

	//load the noise texture
	LoadNoiseTexture(noiseTexture_path);

	//set the fbo
	setFBOVertex();

	//set the bar
	setCompareBar();

	//set some uniform default
	glUseProgram(program);
	glUniform1i(isNormalLoc, isNormal);

	glUseProgram(program2);
	glUniform1i(filterTypeLoc, filterType);
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}

void My_Init()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

void My_Display()
{
	/*glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);*/
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//draw the frame buffer
	glUseProgram(program);
	
	project_matrix = perspective(deg2rad(proj.fovy), proj.aspect, proj.nearClip, proj.farClip);

	view_matrix = lookAt(main_camera.position, main_camera.center + main_camera.position, main_camera.up_vector);
	
	model_matrix = translate(mat4(1.0), model_position);

	mv_matrix = view_matrix * model_matrix;
	//set uniform
	glUniformMatrix4fv(iLocMV, 1, GL_FALSE, value_ptr(mv_matrix));
	glUniformMatrix4fv(iLocP, 1, GL_FALSE, value_ptr(project_matrix));
	glActiveTexture(GL_TEXTURE0);
	for (int i = 0; i < allShapes.size(); ++i)
	{
		glBindVertexArray(allShapes[i].vao);
		int materialID = allShapes[i].materialID;
		glBindTexture(GL_TEXTURE_2D, allMaterials[materialID].diffuse_Tex);
		glDrawElements(GL_TRIANGLES, allShapes[i].drawCount, GL_UNSIGNED_INT, 0);
	}

	//from frame buffer to window
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboDataTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glBindVertexArray(frame_vao);
	glUseProgram(program2);
	glUniform1f(offsetLoc, offset);
	glUniform2f(iMouseLoc, mouseXY.x, mouseXY.y);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	//if in magnifier mode, don't draw the bar
	if (!isMagnifier){
		//draw the comparison bar
		glUseProgram(bar_program);
		glBindVertexArray(bar_vao);
		glDrawArrays(GL_QUADS, 0, 4);
	}
    glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	proj.aspect = (float)(width) / (float)height;
	screenWidth = width;
	screenHeight = height;

	glViewport(0, 0, screenWidth, screenHeight);

	glDeleteRenderbuffers(1, &depthrbo);
	glDeleteTextures(1, &fboDataTexture);

	glGenRenderbuffers(1, &depthrbo);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, screenWidth, screenHeight);

	glGenTextures(1, &fboDataTexture);
	glBindTexture(GL_TEXTURE_2D, fboDataTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboDataTexture, 0);
	//check whether the framebuffer set successfully
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		cout << "framebuffer set did not complete!" << endl;
	}
	
}

void My_Timer(int val)
{
	int currentTime = glutGet(GLUT_ELAPSED_TIME);

	offset = currentTime;
	glutPostRedisplay();
	glutTimerFunc(timer_speed, My_Timer, val);
}

void My_MouseMove(int x, int y) {
	//if in magnifier mode, can't change bar's location and our trackball
	if (isMagnifier){
		mouseXY = vec2(float(x) / screenWidth, float(screenHeight - y) / screenHeight);
		return;
	}
	if (isBarMove){
		float sw = screenWidth / 2;
		float normal_x = float(x - sw) / sw;
		if (normal_x > 1.0){
			normal_x = 1.0;
		}
		if (normal_x < -1.0){
			normal_x = -1.0;
		}
		bar_position = normal_x;
		changeBarLoc();
	}
	else {
		int deltaX = x - lastMouseX;
		int deltaY = lastMouseY - y;

		Yaw -= deltaX * 0.05f;
		Pitch -= deltaY * 0.05;

		if (Pitch > 89.0f) Pitch = 89.0f;
		if (Pitch < -89.0f) Pitch = -89.0f;


		vec3 front;
		front.x = cos(radians(Pitch)) * cos(radians(Yaw));
		front.y = sin(radians(Pitch));
		front.z = cos(radians(Pitch)) * sin(radians(Yaw));
		normalize(front);

		main_camera.center = front;

		//change the lastMouse
		lastMouseX = x;
		lastMouseY = y;

		//cout << main_camera.center.x << " " << main_camera.center.y << " " << main_camera.center.z << endl;
		//cout << Yaw << " " << Pitch << endl;
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		float sw = screenWidth / 2;
		float normal_x = float(x - sw) / sw;
		if (normal_x >= bar_position - BAR_WIDTH * 2 && normal_x <= bar_position + BAR_WIDTH * 2 && !isMagnifier) {
			isBarMove = true;
			cout << "click bar!" << endl;
		}
		else {
			lastMouseX = x;
			lastMouseY = y;
		}
		printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
	}
	else if (state == GLUT_UP)
	{
		if (isBarMove){
			isBarMove = false;
		}
		printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
	
}

void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	float speed = 0.1f;
	switch (key)
	{
	case 'w':
	case 'W':
		main_camera.position.xz += camera_speed * main_camera.center.xz;
		break;
	case 'a':
	case 'A':
		main_camera.position.xz -= normalize(cross(main_camera.center, main_camera.up_vector)).xz;
		break;
	case 's':
	case 'S':
		main_camera.position.xz -= camera_speed * main_camera.center.xz;
		break;
	case 'd':
	case 'D':
		main_camera.position.xz += normalize(cross(main_camera.center, main_camera.up_vector)).xz;
		break;
	case 'z':
	case 'Z':
		main_camera.position -= camera_speed * main_camera.up_vector;
		break;
	case 'x':
	case 'X':
		main_camera.position += camera_speed * main_camera.up_vector;
		break;
	default:
		break;
	}
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Wheel(int wheel, int direction, int x, int y) {
	if (direction > 0) {
		main_camera.position.z -= 1;
	}
	else if (direction < 0) {
		main_camera.position.z += 1;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_NOFILTER:
	case MENU_ABSTRACT:
	case MENU_WATERCOLOR:
	case MENU_BLOOM:
	case MENU_PIXEL:
	case MENU_SINE_WAVE:
		isMagnifier = false;
		glUseProgram(program2);
		glUniform1i(filterTypeLoc, id);
		break;
	case MENU_MAGNIFIER:
		isMagnifier = true;
		mouseXY = vec2(0.5, 0.5);
		glUseProgram(program2);
		glUniform1i(filterTypeLoc, id);
		break;
	case MENU_NORMAL:
		isNormal = isNormal ^ 1;
		glUseProgram(program);
		glUniform1i(isNormalLoc, isNormal);
		break;
	case MENU_TIMER_START:
		if (!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("AS2_Framework"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();
	setupRC();
	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Normal", MENU_NORMAL);
	glutAddMenuEntry("No filter", MENU_NOFILTER);
	glutAddMenuEntry("Abstract", MENU_ABSTRACT);
	glutAddMenuEntry("Watercolor", MENU_WATERCOLOR);
	glutAddMenuEntry("Magnifier", MENU_MAGNIFIER);
	glutAddMenuEntry("Bloom", MENU_BLOOM);
	glutAddMenuEntry("Pixel", MENU_PIXEL);
	glutAddMenuEntry("Sine Wave", MENU_SINE_WAVE);
	glutAddMenuEntry("exit", MENU_EXIT);


	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMouseWheelFunc(My_Wheel);
	glutMotionFunc(My_MouseMove);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// Enter main event loop.
	glutMainLoop();

	return 0;
}
