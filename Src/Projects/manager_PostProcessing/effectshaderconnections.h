#pragma once

/**	\file	effectshaderconnections.h

Sergei <Neill3d> Solokhin 2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

#include <GL/glew.h>
#include "shaderpropertyvalue.h"
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

// forward
class EffectShaderUserObject;

enum class ShaderSystemUniform : uint8_t
{
	INPUT_COLOR_SAMPLER_2D, //!< this is an input image that we read from
	iCHANNEL0, //!< this is an input image, compatible with shadertoy
	INPUT_DEPTH_SAMPLER_2D, //!< this is a scene depth texture sampler in case shader will need it for processing
	LINEAR_DEPTH_SAMPLER_2D, //!< a depth texture converted into linear space (used in SSAO)
	INPUT_MASK_SAMPLER_2D, //!< binded mask for a shader processing
	WORLD_NORMAL_SAMPLER_2D,

	USE_MASKING, //!< float uniform [0; 1] to define if the mask have to be used
	UPPER_CLIP, //!< this is an upper clip image level. defined in a texture coord space to skip processing
	LOWER_CLIP, //!< this is a lower clip image level. defined in a texture coord space to skip processing

	RESOLUTION, //!< vec2 that contains processing absolute resolution, like 1920x1080
	iRESOLUTION, //!< vec2 absolute resolution, compatible with shadertoy
	INV_RESOLUTION, //!< inverse resolution
	TEXEL_SIZE, //!< vec2 of a texel size, computed as 1/resolution

	iTIME, //!< compatible with shadertoy, float, shader playback time (in seconds)
	iDATE, //!< compatible with shadertoy, vec4, (year, month, day, time in seconds)

	CAMERA_POSITION, //!< world space camera position
	MODELVIEW,	//!< current camera modelview matrix
	PROJ,		//!< current camera projection matrix
	MODELVIEWPROJ,	//!< current camera modelview-projection matrix

	INV_MODELVIEWPROJ, // inverse of modelview-projection matrix
	PREV_MODELVIEWPROJ, // modelview-projection matrix from a previous frame

	ZNEAR, //!< camera near plane
	ZFAR,	//!< camera far plane

	COUNT
};

class IEffectShaderConnections
{
public:

	// represents a single shader property, its type, name, value, etc.
	struct MANAGER_POSTPROCESSING_API ShaderProperty
	{
		constexpr static int MAX_NAME_LENGTH{ 64 };
		constexpr static int HASH_SEED{ 123 };

		ShaderProperty() = default;
		ShaderProperty(const ShaderProperty& other);

		// constructor to associate property with fbProperty, recognize the type
		ShaderProperty(const char* nameIn, const char* uniformNameIn, FBProperty* fbPropertyIn = nullptr);
		ShaderProperty(const char* nameIn, const char* uniformNameIn, EPropertyType typeIn, FBProperty* fbPropertyIn = nullptr);

		void SetGeneratedByUniform(bool isGenerated) { isGeneratedByUniform = isGenerated; }
		bool IsGeneratedByUniform() const { return isGeneratedByUniform; }

		uint32_t ComputeNameHash() const;

		void SetName(std::string_view nameIN);
		inline const char* GetName() const { return name; }
		inline uint32_t GetNameHash() const { return mDefaultValue.GetNameHash(); }
		void SetUniformName(std::string_view uniformNameIN) { strncpy_s(uniformName, uniformNameIN.data(), uniformNameIN.size()); }
		inline const char* GetUniformName() const { return uniformName; }
		inline char* GetUniformNameAccess() { return uniformName; }

		void SetLocation(GLint locationIN) {
			mDefaultValue.SetLocation(locationIN);
		}
		inline GLint GetLocation() const { return mDefaultValue.GetLocation(); }

		ShaderProperty& SetType(EPropertyType newType);
		inline EPropertyType GetType() const { return mDefaultValue.GetType();  }
		ShaderProperty& SetFlag(PropertyFlag testFlag, bool setValue);
		bool HasFlag(PropertyFlag testFlag) const;

		// toggle a check if glsl location is found
		ShaderProperty& SetRequired(bool isRequired);

		ShaderProperty& SetScale(float scaleIn);
		float GetScale() const;
		
		inline void SetFBProperty(FBProperty* fbPropertyIN) { fbProperty = fbPropertyIN; }
		inline FBProperty* GetFBProperty() const { return fbProperty; }

		inline void SetFBComponent(FBComponent* fbComponentIN) { fbComponent = fbComponentIN; }
		inline FBComponent* GetFBComponent() const { return fbComponent; }

		ShaderProperty& SetDefaultValue(int valueIn);
		ShaderProperty& SetDefaultValue(bool valueIn);
		ShaderProperty& SetDefaultValue(float valueIn);
		ShaderProperty& SetDefaultValue(double valueIn);

		ShaderProperty& SetDefaultValue(float x, float y);
		ShaderProperty& SetDefaultValue(float x, float y, float z);
		ShaderProperty& SetDefaultValue(float x, float y, float z, float w);

		const float* GetDefaultFloatData() const;
		ShaderPropertyValue& GetDefaultValue() { return mDefaultValue; }
		const ShaderPropertyValue& GetDefaultValue() const { return mDefaultValue; }
		
		void SwapValueBuffers();

		static void ReadFBPropertyValue(
			ShaderPropertyValue& value, 
			const ShaderProperty& shaderProperty, 
			const PostEffectContextProxy* effectContext,
			int maskIndex);
		
		// when shader property comes from FBPropertyListObject
		//  we read first object in the list and can have either texture of shader user object type from it
		static void ReadTextureConnections(ShaderPropertyValue& value, FBProperty* fbProperty);

	private:
		
		ShaderPropertyValue mDefaultValue;
		
		char name[MAX_NAME_LENGTH]{ 0 };
		char uniformName[MAX_NAME_LENGTH]{ 0 };
		
		bool isGeneratedByUniform{ false };

		bool padding[3]{ false };

		std::bitset<PROPERTY_BITSET_SIZE> flags;

		FBProperty* fbProperty{ nullptr };
		FBComponent* fbComponent{ nullptr }; // the owner of the property

	};

	virtual ~IEffectShaderConnections() = default;

	virtual ShaderProperty& AddProperty(const ShaderProperty& property) = 0;
	virtual ShaderProperty& AddProperty(ShaderProperty&& property) = 0;

	virtual int GetNumberOfProperties() const = 0;
	virtual ShaderProperty& GetProperty(int index) = 0;
	
	virtual ShaderProperty* FindProperty(const std::string_view name) = 0;
	
	// look for a UI interface, and read properties and its values
	// we should write values into effectContext's shaderPropertyStorage
	virtual bool CollectUIValues(FBComponent* componentIn, PostEffectContextProxy* effectContext, int maskIndex) = 0;

	// use uniformName to track down some type casts
	static FBPropertyType ShaderPropertyToFBPropertyType(const ShaderProperty& prop);

	static EPropertyType FBPropertyToShaderPropertyType(const FBPropertyType& fbType);

	static EPropertyType UniformTypeToShaderPropertyType(GLenum type);
};


