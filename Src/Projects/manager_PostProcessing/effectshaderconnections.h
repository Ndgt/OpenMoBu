#pragma once

/**	\file	effectshaderconnections.h

Sergei <Neill3d> Solokhin 2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

#include <GL/glew.h>
#include "shaderpropertyvalue.h"
#include "shaderproperty.h"
#include "posteffectcontext.h"

#include <array>
#include <atomic>
#include <bitset>
#include <variant>
#include <vector>

// DLL export macro for Windows
#ifndef MANAGER_POSTPROCESSING_API
# if defined(_WIN32) || defined(_WIN64)
#  if defined(manager_PostProcessing_EXPORTS)
#   define MANAGER_POSTPROCESSING_API __declspec(dllexport)
#  else
#   define MANAGER_POSTPROCESSING_API __declspec(dllimport)
#  endif
# else
#  define MANAGER_POSTPROCESSING_API
# endif
#endif

class IEffectShaderConnections
{
public:

	virtual ~IEffectShaderConnections() = default;

	virtual int GetNumberOfProperties() const = 0;
	virtual const ShaderProperty& GetProperty(int index) const = 0;
	
	virtual const ShaderProperty* FindProperty(const std::string_view name) const = 0;
	
	// use uniformName to track down some type casts
	static FBPropertyType ShaderPropertyToFBPropertyType(const ShaderProperty& prop);

	static EPropertyType FBPropertyToShaderPropertyType(const FBPropertyType& fbType);

	static EPropertyType UniformTypeToShaderPropertyType(GLenum type);
};