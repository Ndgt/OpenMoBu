
#pragma once

// posteffect_rendercontext.cxx
/*
Sergei <Neill3d> Solokhin 2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE
*/

#include "posteffect_rendercontext.h"
#include "mobu_logging.h"
#include "posteffectbase.h"


void PostEffectRenderContext::OverrideUniform(const IEffectShaderConnections::ShaderProperty& shaderProperty, float valueIn)
{
	ShaderPropertyValue newValue(shaderProperty.GetDefaultValue());
	newValue.SetValue(valueIn);

	overrideUniforms.emplace_back(std::move(newValue));
}

void PostEffectRenderContext::OverrideUniform(const IEffectShaderConnections::ShaderProperty& shaderProperty, float x, float y)
{
	ShaderPropertyValue newValue(shaderProperty.GetDefaultValue());
	newValue.SetValue(x, y);

	overrideUniforms.emplace_back(std::move(newValue));
}

void PostEffectRenderContext::OverrideUniform(const IEffectShaderConnections::ShaderProperty& shaderProperty, float x, float y, float z, float w)
{
	ShaderPropertyValue newValue(shaderProperty.GetDefaultValue());
	newValue.SetValue(x, y, z, w);

	overrideUniforms.emplace_back(std::move(newValue));
}

void PostEffectRenderContext::UploadUniforms(const ShaderPropertyStorage::PropertyValueMap* uniformsMap, bool skipTextureProperties) const
{
	// given uniforms
	if (uniformsMap)
	{
		UploadUniformsInternal(*uniformsMap, skipTextureProperties);
	}
	// override uniforms if defined
	UploadUniformsInternal(overrideUniforms, true);
}


void PostEffectRenderContext::UploadUniformsInternal(const ShaderPropertyStorage::PropertyValueMap& uniformsMap, bool skipTextureProperties) const
{
	for (const ShaderPropertyValue& value : uniformsMap)
	{
		UploadUniformValue(value, skipTextureProperties);
	}
}

void PostEffectRenderContext::UploadUniformValue(const ShaderPropertyValue& value, bool skipTextureProperties)
{
	constexpr int MAX_USER_TEXTURE_SLOTS = 16;

	if (value.GetLocation() < 0)
	{
		if (value.IsRequired())
		{
			LOGE("required property location is not found [%u] %s\n", value.GetNameHash(), ResolveHash32(value.GetNameHash()));
		}
		return;
	}

	const float* floatData = value.GetFloatData();
	if (!floatData) {
		LOGE("Property %u has null data\n", value.GetNameHash());
		return;
	}

	switch (value.GetType())
	{
	case EPropertyType::INT:
		glUniform1i(value.GetLocation(), static_cast<int>(floatData[0]));
		break;

	case EPropertyType::BOOL:
		glUniform1f(value.GetLocation(), floatData[0]);
		break;

	case EPropertyType::FLOAT:
	{
		float scaledValue = value.GetScale() * floatData[0];
		if (value.IsInvertValue())
			scaledValue = 1.0f - scaledValue;

		glUniform1f(value.GetLocation(), scaledValue);
	} break;

	case EPropertyType::VEC2:
		glUniform2fv(value.GetLocation(), 1, floatData);
		break;

	case EPropertyType::VEC3:
		glUniform3fv(value.GetLocation(), 1, floatData);
		break;

	case EPropertyType::VEC4:
		glUniform4fv(value.GetLocation(), 1, floatData);
		break;

	case EPropertyType::TEXTURE:
	case EPropertyType::SHADER_USER_OBJECT:
	{
		// designed to be used with multi-pass rendering, when textures are bound from the first pass
		if (skipTextureProperties)
			break;

		const int textureSlot = static_cast<int>(floatData[0]);
		if (textureSlot >= 0 && textureSlot < MAX_USER_TEXTURE_SLOTS)
		{
			glUniform1i(value.GetLocation(), textureSlot);
		}	
	} break;

	default:
		LOGE("not supported property for auto upload into uniform [%u] %s\n", value.GetNameHash(), ResolveHash32(value.GetNameHash()));
	}
}