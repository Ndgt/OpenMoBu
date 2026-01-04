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

	struct ShaderPropertyProxy
	{
		int32_t index{ -1 }; // index in the property array (in case array is not changed or sorted)
		uint32_t nameHash{ 0 }; // hash key of a name to double check that we reference to a correct property
	};

	// represents a single shader property, its type, name, value, etc.
	struct MANAGER_POSTPROCESSING_API ShaderProperty
	{
		constexpr static int MAX_NAME_LENGTH{ 64 };
		constexpr static int HASH_SEED{ 123 };

		ShaderProperty() = default;
		ShaderProperty(const ShaderProperty& other);

		// constructor to associate property with fbProperty, recognize the type
		ShaderProperty(std::string_view nameIn, std::string_view uniformNameIn, FBProperty* fbPropertyIn = nullptr);
		ShaderProperty(std::string_view nameIn, std::string_view uniformNameIn, EPropertyType typeIn, FBProperty* fbPropertyIn = nullptr);

		void SetGeneratedByUniform(bool isGenerated) { isGeneratedByUniform = isGenerated; }
		bool IsGeneratedByUniform() const { return isGeneratedByUniform; }

		// get proxy of this property for a quick access from a property scheme
		ShaderPropertyProxy GetProxy() const
		{
			return { indexInArray, GetNameHash() };
		}

		void SetName(std::string_view nameIN);
		void SetNameHash(uint32_t nameHashIn);
		inline const char* GetName() const noexcept;
		inline uint32_t GetNameHash() const;
		void SetUniformName(std::string_view uniformNameIN);
		void SetUniformNameHash(uint32_t hashIn);
		inline const char* GetUniformName() const noexcept;
		inline uint32_t GetUniformNameHash() const;
		//inline char* GetUniformNameAccess() { return uniformName; }

		void SetLocation(GLint locationIN) {
			mDefaultValue.SetLocation(locationIN);
		}
		inline GLint GetLocation() const { return mDefaultValue.GetLocation(); }

		ShaderProperty& SetType(EPropertyType newType);
		inline EPropertyType GetType() const { return mDefaultValue.GetType();  }
		ShaderProperty& SetFlag(PropertyFlag testFlag, bool setValue=true);
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
		
		static void ReadFBPropertyValue(
			FBProperty* fbProperty,
			ShaderPropertyValue& value, 
			const ShaderProperty& shaderProperty, 
			const PostEffectContextProxy* effectContext,
			int maskIndex);
		
		// when shader property comes from FBPropertyListObject
		//  we read first object in the list and can have either texture of shader user object type from it
		static void ReadTextureConnections(ShaderPropertyValue& value, FBProperty* fbProperty);

		void SetIndexInArray(int32_t indexInArrayIn) { indexInArray = indexInArrayIn; }
		int32_t GetIndexInArray() const { return indexInArray; }

	private:
		
		ShaderPropertyValue mDefaultValue;
		
		uint32_t nameHash{ 0 };
		uint32_t uniformNameHash{ 0 };

		int32_t indexInArray{ -1 }; // for a quick access in the property array

		//char name[MAX_NAME_LENGTH]{ 0 };
		//char uniformName[MAX_NAME_LENGTH]{ 0 };
		
		std::bitset<PROPERTY_BITSET_SIZE> flags;

		bool isGeneratedByUniform{ false };


		// TODO: move that into a evaluator cache (per effect shader)
		FBProperty* fbProperty{ nullptr };
		FBComponent* fbComponent{ nullptr }; // the owner of the property

		// calculate a hash and add it into a hash server
		uint32_t ComputeNameHash(std::string_view s) const;
	};

	// result of glsl uniforms reflection
	struct PropertyScheme
	{
		PropertyScheme()
		{
			ResetSystemUniformLocations();
		}

		ShaderProperty& AddProperty(const ShaderProperty& property);
		ShaderProperty& AddProperty(ShaderProperty&& property);
		ShaderProperty& AddProperty(std::string_view nameIn, std::string_view uniformNameIn, FBProperty* fbPropertyIn = nullptr);
		ShaderProperty& AddProperty(std::string_view nameIn, std::string_view uniformNameIn, EPropertyType typeIn, FBProperty* fbPropertyIn = nullptr);

		bool IsEmpty() const { return properties.empty(); }

		ShaderProperty* FindPropertyByHash(uint32_t nameHash);
		const ShaderProperty* FindPropertyByHash(uint32_t nameHash) const;

		ShaderProperty* FindProperty(const std::string_view name);
		const ShaderProperty* FindProperty(const std::string_view name) const;
		ShaderProperty* FindPropertyByUniform(const std::string_view name);
		const ShaderProperty* FindPropertyByUniform(const std::string_view name) const;

		ShaderProperty* GetProperty(const ShaderPropertyProxy proxy);
		const ShaderProperty* GetProperty(const ShaderPropertyProxy proxy) const;

		size_t GetNumberOfProperties() const { return properties.size(); }

		const std::vector<ShaderProperty>& GetProperties() const { return properties; }

		void	ResetSystemUniformLocations();

		void SetSystemUniformLoc(ShaderSystemUniform u, GLint location) noexcept {
			systemUniformLocations[static_cast<uint32_t>(u)] = location;
		}
		inline GLint GetSystemUniformLoc(ShaderSystemUniform u) const noexcept {
			return systemUniformLocations[static_cast<uint32_t>(u)];
		}

		// @see PostEffectBufferShader::Render
		void AssociateFBProperties(FBComponent* component);

		bool ExportToJSON(const char* fileName) const;

	private:
		std::vector<ShaderProperty> properties;
		std::array<GLint, static_cast<int>(ShaderSystemUniform::COUNT)> systemUniformLocations;
	};

	virtual ~IEffectShaderConnections() = default;

	virtual int GetNumberOfProperties() const = 0;
	virtual const ShaderProperty& GetProperty(int index) const = 0;
	
	virtual const ShaderProperty* FindProperty(const std::string_view name) const = 0;
	
	// use uniformName to track down some type casts
	static FBPropertyType ShaderPropertyToFBPropertyType(const ShaderProperty& prop);

	static EPropertyType FBPropertyToShaderPropertyType(const FBPropertyType& fbType);

	static EPropertyType UniformTypeToShaderPropertyType(GLenum type);
};


