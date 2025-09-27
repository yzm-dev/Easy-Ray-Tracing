#pragma once

#include <GL/glew.h>
#include <iostream>
#include <unordered_map>
#include "glm/fwd.hpp"

class Shader
{
private:
	unsigned int m_RendererID;
	std::string m_VertexFilePath;
	std::string m_FragmentFilePath;
	// Saving the uniform location
	std::unordered_map<std::string, int> m_UniformLocationCache;
public:
	Shader(const std::string& m_VertexFilePath, const std::string& m_FragmentFilePath);
	~Shader();

	void BindShader() const;
	void UnBindShader() const;

	std::string ReadShaderSourceFromFile(const std::string& filePath);
	void SetUniform1i(const std::string& name, int value);
	void SetUniform1f(const std::string& name, float value);
    void SetUniform2f(const std::string& name, float v0, float v1);
    void SetUniform3f(const std::string& name, float value1, float value2, float value3);
    void SetUniform3fv(const std::string& name, const glm::vec3& value);
	void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void SetUniformMat4fv(const std::string& name, const glm::mat4& matrix);

	// Binds a named uniform block to a binding point (useful on GL 3.3 where layout(binding=) may not be available)
    void BindUniformBlock(const std::string& blockName, GLuint bindingPoint) const;
private:
	int GetUniformLocation(const std::string& name);

	unsigned int CompileShader(GLenum shaderType, const std::string& shaderSource);
	unsigned int CreateShader(const std::string& vertexShaderSource, const std::string& fragmentShaderSource);

};
