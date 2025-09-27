#include "Shader.h"
#include "glm/gtc/type_ptr.hpp"

#include <fstream>
#include <sstream>

Shader::Shader(const std::string& m_VertexFilePath, const std::string& m_FragmentFilePath) : m_VertexFilePath(m_VertexFilePath), m_FragmentFilePath(m_FragmentFilePath), m_RendererID(0)
{
    std::string vertexShaderSource = ReadShaderSourceFromFile(m_VertexFilePath);
    std::string fragmentShaderSource = ReadShaderSourceFromFile(m_FragmentFilePath);
    if (vertexShaderSource.empty() || fragmentShaderSource.empty()) {
        std::cerr << "Failed to read shader source from files." << std::endl;
        return;
    }
    m_RendererID = CreateShader(vertexShaderSource, fragmentShaderSource);
    if (m_RendererID == -1) {
        std::cerr << "Failed to create shader program." << std::endl;
    }
}

Shader::~Shader()
{
    if (m_RendererID != 0)
        glDeleteProgram(m_RendererID);
}

void Shader::BindShader() const
{
    if (m_RendererID != 0)
        glUseProgram(m_RendererID);
}

void Shader::UnBindShader() const
{
    glUseProgram(0);
}

void Shader::SetUniform1i(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetUniform1f(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetUniform2f(const std::string& name, float v0, float v1)
{
    glUniform2f(GetUniformLocation(name), v0, v1);
}

void Shader::SetUniform3f(const std::string& name, float value1, float value2, float value3)
{
    glUniform3f(GetUniformLocation(name), value1, value2, value3);
}

void Shader::SetUniform3fv(const std::string& name, const glm::vec3& value)
{
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
}

void Shader::SetUniformMat4fv(const std::string& name, const glm::mat4& matrix)
{
    //glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]);
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::BindUniformBlock(const std::string& blockName, GLuint bindingPoint) const
{
    if (m_RendererID == 0) return;
    GLuint index = glGetUniformBlockIndex(m_RendererID, blockName.c_str());
    if (index == GL_INVALID_INDEX) {
        // Optional: warn but don't spam
        // std::cerr << "Uniform block not found: " << blockName << std::endl;
        return;
    }
    glUniformBlockBinding(m_RendererID, index, bindingPoint);
}

int Shader::GetUniformLocation(const std::string& name)
{
    if (m_RendererID == 0) return -1;

    auto it = m_UniformLocationCache.find(name);
    if (it != m_UniformLocationCache.end())
        return it->second;

    int location = glGetUniformLocation(m_RendererID, name.c_str());
    if (location == -1) {
        //std::cout << "GetUniformLocation Warning: uniform '" << name << "' does not exist!" << std::endl;
        return -1;
    }
    m_UniformLocationCache[name] = location;
    return location;
}

std::string Shader::ReadShaderSourceFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Not opening this shader file: " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int Shader::CompileShader(GLenum shaderType, const std::string& shaderSource)
{
    unsigned int shader = glCreateShader(shaderType);
    const char* src = shaderSource.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compile error:\n" << infoLog << std::endl;
        return -1;
    }
    return shader;
}

unsigned int Shader::CreateShader(const std::string& vertexShaderSource, const std::string& fragmentShaderSource)
{
    unsigned int shaderProgram = glCreateProgram();
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glValidateProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}