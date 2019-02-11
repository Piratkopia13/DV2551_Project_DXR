#include "Material.h"

Material& Material::addDefine(const std::string& defineText, ShaderType type)
{
	// is a MAP of (ShaderType, Set<strings>)
	shaderDefines[type].insert(defineText);
	return *this;
}
