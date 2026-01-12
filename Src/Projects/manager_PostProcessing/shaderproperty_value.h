#pragma once

/**	\file	shaderproperty_value.h

Sergei <Neill3d> Solokhin 2025-2026

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

//--- SDK include
#include <fbsdk/fbsdk.h>

#include <GL/glew.h>

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

// forward
class EffectShaderUserObject;

constexpr size_t PROPERTY_BITSET_SIZE = 8;

enum class EPropertyType : uint8_t
{
	NONE, // in case we have input property with no connection or value
	INT,
	BOOL,
	FLOAT,
	VEC2,
	VEC3,
	VEC4,
	MAT4,
	TEXTURE, // pointer to texture
	SHADER_USER_OBJECT // pointer to shader user object
};

enum class PropertyFlag : uint8_t
{
	SYSTEM = 1,	// flag that the property is a system one, like masking, upper/lower clip, etc.
	IsClamped100 = 1,
	IsClamped1 = 2,
	IsFlag = 3, // when bool UI value is converted into float [0; 1] uniform
	IsColor = 4,
	ConvertWorldToScreenSpace = 5, // this is when world space vector3 is converted into screen space vector2 uniform
	SKIP = 6, //!< this is for manual processing of property (like manual reading and setting value)
	INVERT_VALUE = 7 //!< a given property value is going to be written as 1.0 - value to the uniform
};


// a generic value holder for different property types
struct MANAGER_POSTPROCESSING_API ShaderPropertyValue
{
	ShaderPropertyValue() = default;

	ShaderPropertyValue(EPropertyType newType) : value(std::array<float, 1>{ 0.0f }) 
	{
		SetType(newType);
	}
	ShaderPropertyValue(const ShaderPropertyValue& other) = default;

	void SetNameHash(uint32_t nameHashIn) { key = nameHashIn; }
	inline uint32_t GetNameHash() const { return key; }

	// change type and apply a default value according to a given type
	void SetType(EPropertyType newType);
	inline EPropertyType GetType() const { return type; }

	void SetLocation(GLint locationIn) { location = locationIn; }
	inline GLint GetLocation() const { return location; }

	void SetRequired(bool isRequired) { bIsLocationRequired = isRequired; }
	inline bool IsRequired() const { return bIsLocationRequired; }

	inline void SetValue(int valueIn) {
		value = std::array<float, 1>{ static_cast<float>(valueIn) };
	}
	inline void SetValue(bool valueIn) {
		value = std::array<float, 1>{ valueIn ? 1.0f : 0.0f };
	}
	inline void SetValue(float valueIn) {
		value = std::array<float, 1>{ valueIn };
	}
	inline void SetValue(double valueIn) {
		value = std::array<float, 1>{ static_cast<float>(valueIn) };
	}

	inline void SetValue(float x, float y) {
		value = std::array<float, 2>{ x, y };
	}
	inline void SetValue(float x, float y, float z) {
		value = std::array<float, 3>{ x, y, z };
	}
	inline void SetValue(float x, float y, float z, float w) {
		value = std::array<float, 4>{ x, y, z, w };
	}

	inline const float* GetFloatData() const {
		return std::visit([](auto&& arg) -> const float* {
			return arg.data();
			}, value);
	}

	inline void SetScale(float valueIn) { scale = valueIn; }
	inline float GetScale() const { return scale; }

	inline void SetInvertValue(bool doInvertValueIn) { doInvertValue = doInvertValueIn; }
	inline bool IsInvertValue() const { return doInvertValue; }

	// extracted value from reference object property
	union
	{
		FBTexture* texture{ nullptr };
		EffectShaderUserObject* shaderUserObject;
	};

private:

	// Type-safe dynamic storage for float values
	std::variant<std::array<float, 1>, std::array<float, 2>, std::array<float, 3>, std::array<float, 4>, std::vector<float>> value;

	uint32_t	key = 0; //!< unique key to identify property

	EPropertyType type{ EPropertyType::FLOAT };

	bool bIsLocationRequired{ true }; //!< should we treat missing location as an error or not
	GLshort location{ -1 }; //!< GLSL shader location holder

	float scale{ 1.0f };

	bool doInvertValue{ false };

};