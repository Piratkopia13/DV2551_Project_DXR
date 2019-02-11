#pragma once

#include <string>
#include <set>
#include <map>

/* 
 * extend this class with a concrete implementation,
 * public interface should not change...
 * 
 * A material represents the programmable part of the pipeline:
 * Vertex, Geometry (wont be used), Fragment and Compute Shaders (wont be used)
 * Only need to support Vertex and Fragment for now.
 * Any extra functionality should be added to a concrete subclass
 */
struct Color {
	float r, g, b, a;
};

class Material
{
public:

	enum class ShaderType { VS = 0, PS = 1, GS = 2, CS = 3 };

	Material() : isValid(false) {};
	virtual ~Material() {};

	// all defines should be included in the shader before COMPILATION.
	Material& addDefine(const std::string& defineText, ShaderType type);

	// set shader name, DOES NOT COMPILE
	virtual void setShader(const std::string& shaderFileName, ShaderType type) = 0;

	// removes any resource linked to shader type
	virtual void removeShader(ShaderType type) = 0;

	virtual void setDiffuse(Color c) = 0;

	/*
	 * Compile and link all shaders
	 * Returns 0  if compilation/linking succeeded.
	 * Returns -1 if compilation/linking fails.
	 * Error is returned in errString
	 * A Vertex and a Fragment shader MUST be defined.
	 * If compileMaterial is called again, it should RE-COMPILE the shader
	 * In principle, it should only be necessary to re-compile if the defines set 
	 * has changed.
	*/
	virtual int compileMaterial(std::string& errString) = 0;

	// this constant buffer will be bound every time we bind the material
	virtual void addConstantBuffer(std::string name, unsigned int location) = 0;

	// location identifies the constant buffer in a unique way
	virtual void updateConstantBuffer(const void* data, size_t size, unsigned int location) = 0;

	// activate the material for use.
	virtual int enable() = 0;

	// disable material
	virtual void disable() = 0;
	
	bool isValid;
	Color color {};

	std::map<ShaderType, std::string> shaderFileNames;
	std::map<ShaderType, std::set<std::string>> shaderDefines;
};

