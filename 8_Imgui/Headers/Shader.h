#pragma once

#include <GL/glew.h>
#include <string>
#include "Math.h"
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

class Shader
{
public:
	Shader();
	~Shader();

	bool loadShaderFile(const string& vert_path, const string& frag_path, const string& geom_path = "");
	
	void setInt(const string& name, int value) const;
	void setFloat(const string& name, float value) const;
	void setVec3(const string& name, vec3 vector) const;
	void setVec3(const string& name, float x, float y, float z) const;
	void setMat4(const string& name, mat4& matrix) const;

	void setPVM(glm::mat4& p, glm::mat4& v, glm::mat4& m) const;

	void load();
	void unload();

private:
	bool CompileShader(const string& fileName, GLenum shaderType, GLuint& outShader);
	bool IsCompiled(GLuint& shader);

	GLuint m_shader_ID;
};