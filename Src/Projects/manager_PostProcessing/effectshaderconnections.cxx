/**	\file	effectshaderconnections.cxx

Sergei <Neill3d> Solokhin 2018-2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

//--- Class declaration
#include "effectshaderconnections.h"
#include "hashUtils.h"
#include "mobu_logging.h"
#include "posteffect_shader_userobject.h"
#include "posteffectcontextmobu.h"


/////////////////////////////////////////////////////////////////////////
// IEffectShaderConnections

FBPropertyType IEffectShaderConnections::ShaderPropertyToFBPropertyType(const ShaderProperty& prop)
{
	switch (prop.GetType())
	{
	case EPropertyType::FLOAT:
		return (prop.HasFlag(PropertyFlag::IsFlag)) ? FBPropertyType::kFBPT_bool : FBPropertyType::kFBPT_double;
	case EPropertyType::INT:
		return FBPropertyType::kFBPT_int;
	case EPropertyType::BOOL:
		return FBPropertyType::kFBPT_bool;
	case EPropertyType::VEC2:
		return (prop.HasFlag(PropertyFlag::ConvertWorldToScreenSpace)) ? FBPropertyType::kFBPT_Vector3D : FBPropertyType::kFBPT_Vector2D;
	case EPropertyType::VEC3:
		return (prop.HasFlag(PropertyFlag::IsColor)) ? FBPropertyType::kFBPT_ColorRGB : FBPropertyType::kFBPT_Vector3D;
	case EPropertyType::VEC4:
		return (prop.HasFlag(PropertyFlag::IsColor)) ? FBPropertyType::kFBPT_ColorRGBA : FBPropertyType::kFBPT_Vector4D;
	case EPropertyType::MAT4:
		return FBPropertyType::kFBPT_Vector4D; // TODO:
	case EPropertyType::TEXTURE:
		return FBPropertyType::kFBPT_object; // reference to a texture object that we could bind to a property
	default:
		return FBPropertyType::kFBPT_double;
	}
}

EPropertyType IEffectShaderConnections::FBPropertyToShaderPropertyType(const FBPropertyType& fbType)
{
	switch (fbType)
	{
	case FBPropertyType::kFBPT_int:
		return EPropertyType::INT;
	case FBPropertyType::kFBPT_float:
		return EPropertyType::FLOAT;
	case FBPropertyType::kFBPT_bool:
		return EPropertyType::FLOAT;
	case FBPropertyType::kFBPT_Vector2D:
		return EPropertyType::VEC2;
	case FBPropertyType::kFBPT_ColorRGB:
	case FBPropertyType::kFBPT_Vector3D:
		return EPropertyType::VEC3;
	case FBPropertyType::kFBPT_ColorRGBA:
	case FBPropertyType::kFBPT_Vector4D:
		return EPropertyType::VEC4;
	}
	return EPropertyType::FLOAT;
}

