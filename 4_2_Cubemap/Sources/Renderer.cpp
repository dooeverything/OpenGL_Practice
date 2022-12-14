#include "Renderer.h"

#include <fstream>
#include <sstream>

Renderer::Renderer() : m_is_running(1), m_type(0), m_width(0), m_height(0), m_Matrices_UBO(0)
{
	m_window = nullptr;
	m_context = 0;
}

bool Renderer::InitSetup()
{
	SDL_Init(SDL_INIT_VIDEO);

	// Set core OpenGL profile
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Set version of SDL 
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	// Set a buffer with 8 bits for each channel
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	// Set double buffering
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Hardware Acceleration
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	m_window = SDL_CreateWindow("Cubemap, Reflection, Refraction", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		static_cast<int>(m_width), static_cast<int>(m_height), SDL_WINDOW_OPENGL);

	if (!m_window)
	{
		printf("Failed to load window: %s \n", SDL_GetError());
		return false;
	}

	m_context = SDL_GL_CreateContext(m_window);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		printf("Failed to initialize GLEW. \n");
		return false;
	}

	glGetError();

	// Enable depth buffer
	glEnable(GL_DEPTH_TEST);

	printf("%s \n", glGetString(GL_VERSION));

	return true;
}

bool Renderer::initialize(const float screen_width, const float screen_height)
{
	m_width = screen_width;
	m_height = screen_height;

	if (!InitSetup())
	{
		printf("Failed to init setup \n");
		return false;
	}

	// Load models
	m_skybox = LoadSkybox("Vertices/Cube/Skybox_Vertices_List.txt", "Vertices/Cube/Cube_Indices_List.txt");
	m_primitives[Shape::CUBE] = LoadPrimitive(Shape::CUBE);
	m_link = LoadCharacter("Models/Link/Link_Final.obj", false);

	if (!m_link || !m_skybox || !m_primitives[Shape::CUBE])
	{
		cout << "Failed to load some models" << endl;
		return false;
	}

	// Load shaders
	m_shaders["link"] = LoadShader("Shaders/Model.vert", "Shaders/Model.frag");
	
	m_shaders["cubemap"] = LoadShader("Shaders/1_1_Cubemap.vert", "Shaders/1_1_Cubemap.frag");
	m_shaders["cubemap"]->SetActive();
	m_shaders["cubemap"]->SetInt("skybox", 0);
	
	m_shaders["reflection"] = LoadShader("Shaders/1_2_Reflection.vert", "Shaders/1_2_Reflection.frag");
	m_shaders["reflection"]->SetActive();
	m_shaders["reflection"]->SetInt("skybox", 0);

	m_shaders["refraction"] = LoadShader("Shaders/1_3_Refraction.vert", "Shaders/1_3_Refraction.frag");
	m_shaders["refraction"]->SetActive();
	m_shaders["refraction"]->SetInt("skybox", 0);

	for (auto& it : m_shaders)
	{
		if (!it.second)
		{
			cout << "Failed to load " << it.first << endl;
			return false;
		}
	}

	vector<string> faces
	{
		"textures/skybox/right.jpg",
		"textures/skybox/left.jpg",
		"textures/skybox/top.jpg",
		"textures/skybox/bottom.jpg",
		"textures/skybox/front.jpg",
		"textures/skybox/back.jpg",
	};

	// Load textures
	m_cubemap_texture = LoadSkyboxTex(faces);
	
	if (!m_cubemap_texture)
	{
		printf("Failed to load some textures \n");
		return false;
	}

	m_camera = unique_ptr<Camera>(new Camera());


	SetupUBO();

	return true;
}

void Renderer::DrawModels()
{
	mat4 p = perspective(radians(m_camera->GetFov()), m_width / m_height, 0.1f, 100.0f);
	mat4 v = m_camera->MyLookAt();
	mat4 m = mat4(1.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, m_Matrices_UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), value_ptr(p));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), value_ptr(v));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Draw a reflection box
	m_shaders["reflection"]->SetActive();
	m = translate(m, vec3(3.0, 1.0, 1.0));
	m_shaders["reflection"]->SetMat4("model", m);
	m_shaders["reflection"]->SetVec3("cameraPos", m_camera->GetPos());

	m_primitives[Shape::CUBE]->SetActive();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_cubemap_texture->GetTextureID());
	m_primitives[Shape::CUBE]->Draw();
	glBindVertexArray(0);

	// Draw a refraction box
	m_shaders["refraction"]->SetActive();
	m = mat4(1.0f);
	m = translate(m, vec3(-3.0, 1.0, 1.0));
	m_shaders["refraction"]->SetMat4("model", m);
	m_shaders["refraction"]->SetVec3("cameraPos", m_camera->GetPos());
	
	m_primitives[Shape::CUBE]->SetActive();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_cubemap_texture->GetTextureID());
	m_primitives[Shape::CUBE]->Draw();
	glBindVertexArray(0);

	m_shaders["link"]->SetActive();
	m = mat4(1.0f);
	m = translate(m, vec3(0.0, 2.0, 0.0));
	m_shaders["link"]->SetMat4("projection", p);
	m_shaders["link"]->SetMat4("view", v);
	m_shaders["link"]->SetMat4("model", m);	
	m_link->Draw(*m_shaders["link"]);
	glBindVertexArray(0);

	// Draw a skybox
	glDepthFunc(GL_LEQUAL); // To pass the depth test, it must be less than or equal to the depth buffer
	m_shaders["cubemap"]->SetActive();
	m_shaders["cubemap"]->SetMat4("projection", p);
	v = mat4(mat3(m_camera->MyLookAt()));
	m_shaders["cubemap"]->SetMat4("view", v);

	m_skybox->SetActive();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap_texture->GetTextureID());
	m_skybox->Draw();
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}

void Renderer::Draw()
{
	// Get current frame for camera delta time 
	float current_frame = (float)SDL_GetTicks() * 0.001f;
	float last_frame = m_camera->GetLastFrame();
	m_camera->SetDeltaTime(current_frame - last_frame);
	m_camera->SetLastFrame(current_frame);

	m_camera->ProcessInput();

	SDL_Event m_event;

	while (SDL_PollEvent(&m_event))
	{
		if (m_event.type == SDL_QUIT)
		{
			m_is_running = 0;
		}

		if (m_event.type == SDL_MOUSEBUTTONUP)
		{
			m_camera->ProcessMouseUp(m_event, m_window, 
				static_cast<int>(m_width/2), static_cast<int>(m_height/2));
		}

		if (m_event.type == SDL_MOUSEMOTION)
		{
			m_camera->ProcessMouseDown(m_event);		
		}
	}
	 
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Need to clear color, depth, and stencil buffer for every frame
	// color, depth, and stencil are updated for every frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	glEnable(GL_DEPTH_TEST);

	SetupLight();

	DrawModels();

	SDL_GL_SwapWindow(m_window);
}

unique_ptr<Model> Renderer::LoadCharacter(const string& path, bool flip)
{
	//m_link = new Model();
	cout << "Load Model" << endl;
	unique_ptr<Model> model(new Model());
	stbi_set_flip_vertically_on_load(flip);

	if (!model->LoadModel(path))
	{
		printf("Renderer failed to load model %s \n", path.c_str());
		return nullptr;
	}

	return model;
}

unique_ptr<Mesh> Renderer::LoadPrimitive(const Shape& shape)
{
	cout << "Load Primitive!" << endl;

	string v_path = "";
	string i_path = "";
	
	switch (shape)
	{
	case(Shape::SQUARE):
		v_path = "Vertices/Box/Box_Vertices_List.txt";
		i_path = "Vertices/Box/Box_Indices_List.txt";
		break;

	case(Shape::CUBE):
		v_path = "Vertices/Cube/Cube_Vertices_List.txt";
		i_path = "Vertices/Cube/Cube_Indices_List.txt";
		break;
		
	default:
		cout << "Invalide Shape!" << endl;
		return nullptr;
	}

	vector<Vertex> vertices = LoadVertices(v_path);
	vector<unsigned int> indices = LoadIndices(i_path);

	if (vertices.empty())
	{
		printf("Renderer Failed to load vertices \n");
		return nullptr;
	}

	if (indices.empty())
	{
		printf("Renderer Failed to load indices \n");
		return nullptr;
	}

	unique_ptr<Mesh> primitive(new Mesh(vertices, indices));
	primitive->SetupMesh();

	return primitive;
}

unique_ptr<Mesh> Renderer::LoadSkybox(const string& v_path, const string& i_path)
{
	cout << "Load skybox" << endl;

	vector<Vertex> vertices = LoadVertices(v_path);
	vector<unsigned int> indices = LoadIndices(i_path);

	if (vertices.empty())
	{
		printf("Renderer Failed to load skybox's vertices \n");
		return nullptr;
	}
	
	if (indices.empty())
	{
		printf("Renderer Failed to load skybox's indices \n");
		return nullptr;
	}
	
	unique_ptr<Mesh> skybox(new Mesh(LoadVertices(v_path), LoadIndices(i_path)));
	skybox->SetupMesh();

	return skybox;
}

unique_ptr<Shader> Renderer::LoadShader(const string& vert, const string& frag)
{
	cout << "Load Shader!" << endl;

	unique_ptr<Shader> temp(new Shader());
	if (!temp->LoadShaderFile(vert, frag))
	{
		printf("Renderer Failed to load shader \n");
		return nullptr;
	}

	return temp;
}

unique_ptr<Texture> Renderer::LoadSkyboxTex(const vector<string>& faces)
{
	cout << "Load skybox texture" << endl;;

	unique_ptr<Texture> temp(new Texture());
	if (!temp->LoadCubemapTexture(faces))
	{
		printf("Rendere failed to load skybox texture \n");
		return nullptr;
	}
	return temp;
}

vector<Vertex> Renderer::LoadVertices(const string& vert_path)
{
	vector<Vertex> vertices;

	cout << "Load Vertices!" << endl;
	
	ifstream vertex_file(vert_path);
	
	if (!vertex_file.is_open())
	{
		cout << "Renderer Failed to open the vertex file " << vert_path << endl;
		return {};
	}

	string line;
	int index_tex = 0;
	int index_nor = 0;
	while (getline(vertex_file, line))
	{
		stringstream ss(line);
		if (line[0] == '#')
		{
			//cout << "# ignore" << endl;
			continue;
		}
		else if (line[0] == 'v')
		{
			cout << "Vertex load" << endl;
			m_type = 0;
			continue;
		}
		else if (line[0] == 'n')
		{
			cout << line << endl;
			m_type = 1;
			continue;
		}
		else if (line[0] == 't')
		{
			cout << "Texture coordinate load" << endl;
			m_type = 2;
			continue;
		}

		if (m_type == 0)
		{
			Vertex vertex;
			string vertex_x;
			string vertex_y;
			string vertex_z;

			getline(ss, vertex_x, ',');
			getline(ss, vertex_y, ',');
			getline(ss, vertex_z, ',');


			//vertex.position = position;
			vertex.position.x = stof(vertex_x);
			vertex.position.y = stof(vertex_y);
			vertex.position.z = stof(vertex_z);
			vertices.push_back(vertex);
		}
		else if (m_type == 1)
		{
			string normal_x;
			string normal_y;
			string normal_z;

			getline(ss, normal_x, ',');
			getline(ss, normal_y, ',');
			getline(ss, normal_z);

			Vertex& vertex = vertices[index_nor];

			vertex.normal.x = stof(normal_x);
			vertex.normal.y = stof(normal_y);
			vertex.normal.z = stof(normal_z);
			
			++index_nor;
		}
		else if (m_type == 2)
		{
			string texture_x;
			string texture_y;

			getline(ss, texture_x, ',');
			getline(ss, texture_y, ',');

			//cout << texture_x << " " << texture_y << endl;

			Vertex& vertex = vertices[index_tex];

			vertex.texCoords.x = stof(texture_x);
			vertex.texCoords.y = stof(texture_y);
			++index_tex;
		}
	}
	return vertices;
}

vector<unsigned int> Renderer::LoadIndices(const string& index_path)
{
	cout << "Load indices!" << endl;

	vector<unsigned int> indices;

	ifstream index_file(index_path);

	if (!index_file.is_open())
	{
		cout << "Renderer Failed to open the index file " << index_path << endl;
	}

	string line;

	while (getline(index_file, line))
	{
		stringstream ss(line);
		if (line[0] == '#')
		{
			continue;
		}

		string index;

		while (getline(ss, index, ','))
		{
			const char* index_char = index.c_str();
			unsigned int index_uint = strtoul(index_char, nullptr, 0);
			indices.push_back(index_uint);
		}

	}

	return indices;
}

void Renderer::SetupUBO()
{
	const GLuint& reflect_ID = m_shaders["reflection"]->GetShaderProgram();
	const GLuint& refract_ID = m_shaders["refraction"]->GetShaderProgram();

	// Get uniform block index of the named uniform block "Matrices" from specific shader program
	unsigned int uniformReflect = glGetUniformBlockIndex(reflect_ID, "Matrices");
	unsigned int uniformRefract = glGetUniformBlockIndex(refract_ID, "Matrices");

	// link the uniform block to the uniform binding point, which is 0
	//            Uniform block						binding points			uniform buffer object
	//  reflect_shader::uniform Matrices ---------------> 0 ------------------> m_Matrices_UBO
	//													 /|\
	//	refract_shader::uniform Matrices -----------------
	glUniformBlockBinding(reflect_ID, uniformReflect, 0);
	glUniformBlockBinding(refract_ID, uniformRefract, 0);

	// Create a uniform buffer object to store data 
	glGenBuffers(1, &m_Matrices_UBO);
	glBindBuffer(GL_UNIFORM_BUFFER, m_Matrices_UBO);
	// Set the initial size of uniform buffer object
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Set up the range of buffer that links to the binding point
	// from the first index of 'GL_UNIFORM_BUFFER m_Matrices_UBO', 
	// get data with size of 2*mat4 with offset 0 of m_Matrices_UBO(Uniform Buffer Object)
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_Matrices_UBO, 0, 2 * sizeof(mat4));
}

void Renderer::SetupLight()
{
	// Lights colors
	vec3 amb = { 0.5f, 0.5f, 0.5f };
	vec3 diff = { 1.0f, 1.0f, 1.0f };
	vec3 spec = { 0.5f, 0.5f, 0.5f };
	vec3 dir = { -0.2f, -1.0f, -0.3f };

	vec3 cam_pos = m_camera->GetPos();
	vec3 cam_forward = m_camera->GetForward();

	m_shaders["link"]->SetActive();
	m_shaders["link"]->SetVec3("viewPos", cam_pos);

	m_shaders["link"]->SetVec3("dirLight.direction", dir);
	m_shaders["link"]->SetVec3("dirLight.ambient", amb);
	m_shaders["link"]->SetVec3("dirLight.diffuse", diff);
	m_shaders["link"]->SetVec3("dirLight.specular", spec);
}

void Renderer::UnLoad()
{
	for (auto& it : m_shaders)
	{
		it.second->UnLoad();
	}

	// Terminate and clear all allocated SDL resources
	SDL_GL_DeleteContext(m_context);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}


