
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

FBPropertyType IEffectShaderConnections::ShaderPropertyToFBPropertyType(const IEffectShaderConnections::ShaderProperty& prop)
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

EPropertyType IEffectShaderConnections::UniformTypeToShaderPropertyType(GLenum type)
{
	switch (type)
	{
	case GL_FLOAT:
		return EPropertyType::FLOAT;
	case GL_INT:
		return EPropertyType::INT;
	case GL_BOOL:
		return EPropertyType::FLOAT;
	case GL_FLOAT_VEC2:
		return EPropertyType::VEC2;
	case GL_FLOAT_VEC3:
		return EPropertyType::VEC3;
	case GL_FLOAT_VEC4:
		return EPropertyType::VEC4;
	case GL_FLOAT_MAT4:
		return EPropertyType::MAT4;
	case GL_SAMPLER_2D:
		return EPropertyType::TEXTURE;
	default:
		LOGE("unsupported uniform type %d", type);
		return EPropertyType::FLOAT;
	}
}

/////////////////////////////////////////////////////////////////////////
// ShaderProperty

IEffectShaderConnections::ShaderProperty::ShaderProperty(const IEffectShaderConnections::ShaderProperty& other)
{
	strcpy_s(name, sizeof(char) * MAX_NAME_LENGTH, other.name);
	strcpy_s(uniformName, sizeof(char) * MAX_NAME_LENGTH, other.uniformName);
	
	flags = other.flags;
	fbProperty = other.fbProperty;
	mDefaultValue = other.mDefaultValue;
}

IEffectShaderConnections::ShaderProperty::ShaderProperty(const char* nameIn, const char* uniformNameIn, FBProperty* fbPropertyIn)
{
	if (nameIn)
		strcpy_s(name, sizeof(char) * MAX_NAME_LENGTH, nameIn);
	if (uniformNameIn)
		strcpy_s(uniformName, sizeof(char) * MAX_NAME_LENGTH, uniformNameIn);
	if (fbPropertyIn)
	{
		const auto propertyType = FBPropertyToShaderPropertyType(fbPropertyIn->GetPropertyType());
		SetType(propertyType);
		fbProperty = fbPropertyIn;
	}

	const uint32_t nameHash = ComputeNameHash();
	mDefaultValue.SetNameHash(nameHash);
}

IEffectShaderConnections::ShaderProperty::ShaderProperty(const char* nameIn, const char* uniformNameIn, EPropertyType typeIn, FBProperty* fbPropertyIn)
{
	if (nameIn)
		strcpy_s(name, sizeof(char) * 64, nameIn);
	if (uniformNameIn)
		strcpy_s(uniformName, sizeof(char) * 64, uniformNameIn);

	SetType(typeIn);

	if (fbPropertyIn)
		fbProperty = fbPropertyIn;

	const uint32_t nameHash = ComputeNameHash();
	mDefaultValue.SetNameHash(nameHash);
}

uint32_t IEffectShaderConnections::ShaderProperty::ComputeNameHash() const
{
	return xxhash32(name, HASH_SEED);
}

void IEffectShaderConnections::ShaderProperty::SetName(std::string_view nameIN) 
{ 
	strncpy_s(name, nameIN.data(), nameIN.size()); 
	const uint32_t nameHash = ComputeNameHash();
	mDefaultValue.SetNameHash(nameHash);
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetType(EPropertyType newType) 
{
	mDefaultValue.SetType(newType);
	return *this;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetRequired(bool isRequired)
{
	mDefaultValue.SetRequired(isRequired);
	return *this;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetDefaultValue(int valueIn)
{
	assert((type == IEffectShaderConnections::EPropertyType::INT)
		|| (type == IEffectShaderConnections::EPropertyType::FLOAT)
		|| (type == IEffectShaderConnections::EPropertyType::TEXTURE));

	mDefaultValue.SetValue(valueIn);
	return *this;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetDefaultValue(bool valueIn)
{
	assert(type == IEffectShaderConnections::EPropertyType::BOOL);
	mDefaultValue.SetValue(valueIn);
	return *this;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetDefaultValue(float valueIn)
{
	assert(type == IEffectShaderConnections::EPropertyType::FLOAT);
	mDefaultValue.SetValue(valueIn);
	return *this;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetDefaultValue(double valueIn)
{
	assert(type == IEffectShaderConnections::EPropertyType::FLOAT);
	mDefaultValue.SetValue(valueIn);
	return *this;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetDefaultValue(float x, float y)
{
	assert(type == IEffectShaderConnections::EPropertyType::VEC2);
	mDefaultValue.SetValue(x, y);
	return *this;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetDefaultValue(float x, float y, float z)
{
	assert(type == IEffectShaderConnections::EPropertyType::VEC3);
	mDefaultValue.SetValue(x, y, z);
	return *this;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetDefaultValue(float x, float y, float z, float w)
{
	assert(type == IEffectShaderConnections::EPropertyType::VEC4);
	mDefaultValue.SetValue(x, y, z, w);
	return *this;
}
const float* IEffectShaderConnections::ShaderProperty::GetDefaultFloatData() const {
	return mDefaultValue.GetFloatData();
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetFlag(PropertyFlag testFlag, bool setValue) {
	flags.set(static_cast<size_t>(testFlag), setValue);
	if (testFlag == PropertyFlag::INVERT_VALUE)
		mDefaultValue.SetInvertValue(setValue);
	return *this;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::ShaderProperty::SetScale(float scaleIn)
{
	mDefaultValue.SetScale(scaleIn);
	return *this;
}

float IEffectShaderConnections::ShaderProperty::GetScale() const
{
	return mDefaultValue.GetScale();
}

bool IEffectShaderConnections::ShaderProperty::HasFlag(PropertyFlag testFlag) const {
	return flags.test(static_cast<size_t>(testFlag));
}

void IEffectShaderConnections::ShaderProperty::SwapValueBuffers()
{
	//const int writeIndex = 1 - mReadIndex.load(std::memory_order_relaxed);
	
	// Atomic swap
	//mReadIndex.store(writeIndex, std::memory_order_release);
}

void IEffectShaderConnections::ShaderProperty::ReadFBPropertyValue(
	ShaderPropertyValue& value, 
	const ShaderProperty& shaderProperty, 
	const PostEffectContextProxy* effectContext,
	int maskIndex)
{
	FBProperty* fbProperty = shaderProperty.fbProperty;
	if (fbProperty == nullptr)
		return;

	double v[4]{ 0.0 };
	const FBPropertyType fbType = fbProperty->GetPropertyType();

	switch (fbType)
	{
	case FBPropertyType::kFBPT_int:
	{
		VERIFY(value.GetType() == EPropertyType::INT);
		int ivalue = 0;
		fbProperty->GetData(&ivalue, sizeof(int), effectContext->GetEvaluateInfo());
		value.SetValue(ivalue);
	} break;

	case FBPropertyType::kFBPT_bool:
	{
		VERIFY((value.GetType() == EPropertyType::BOOL)
			|| (value.GetType() == EPropertyType::FLOAT));

		bool bvalue = false;
		fbProperty->GetData(&bvalue, sizeof(bool), effectContext->GetEvaluateInfo());
		value.SetValue(bvalue);
	} break;

	case FBPropertyType::kFBPT_double:
	{
		VERIFY(value.GetType() == EPropertyType::FLOAT);
		fbProperty->GetData(v, sizeof(double), effectContext->GetEvaluateInfo());
		value.SetValue(v[0]);
	} break;

	case FBPropertyType::kFBPT_float:
	{
		VERIFY(value.GetType() == EPropertyType::FLOAT);
		float fvalue = 0.0f;
		fbProperty->GetData(&fvalue, sizeof(float), effectContext->GetEvaluateInfo());
		value.SetValue(fvalue);
	} break;

	case FBPropertyType::kFBPT_Vector2D:
	{
		VERIFY(value.GetType() == EPropertyType::VEC2);
		fbProperty->GetData(v, sizeof(double) * 2, effectContext->GetEvaluateInfo());
		value.SetValue(static_cast<float>(v[0]), static_cast<float>(v[1]));
	} break;

	case FBPropertyType::kFBPT_Vector3D:
	case FBPropertyType::kFBPT_ColorRGB:
	{
		if (!shaderProperty.HasFlag(PropertyFlag::ConvertWorldToScreenSpace))
		{
			VERIFY(value.GetType() == EPropertyType::VEC3);
			fbProperty->GetData(v, sizeof(double) * 3, effectContext->GetEvaluateInfo());
			value.SetValue(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
		}
		else
		{
			VERIFY(effectContext != nullptr);
			// convert world to screen shape, output VEC2
			VERIFY(value.GetType() == EPropertyType::VEC2);

			// world space to screen space
			fbProperty->GetData(v, sizeof(double) * 3, effectContext->GetEvaluateInfo());

			const FBMatrix mvp(effectContext->GetModelViewProjMatrix());

			FBVector4d v4;
			FBVectorMatrixMult(v4, mvp, FBVector4d(v[0], v[1], v[2], 1.0));

			v4[0] = effectContext->GetViewWidth() * 0.5 * (v4[0] + 1.0);
			v4[1] = effectContext->GetViewHeight() * 0.5 * (v4[1] + 1.0);

			value.SetValue(static_cast<float>(v4[0]) / static_cast<float>(effectContext->GetViewWidth()),
				static_cast<float>(v4[1]) / static_cast<float>(effectContext->GetViewHeight()));
		}

	} break;

	case FBPropertyType::kFBPT_Vector4D:
	case FBPropertyType::kFBPT_ColorRGBA:
	{
		VERIFY(value.GetType() == EPropertyType::VEC4);
		fbProperty->GetData(v, sizeof(double) * 4, effectContext->GetEvaluateInfo());
		value.SetValue(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]), static_cast<float>(v[3]));
	} break;

	case FBPropertyType::kFBPT_object:
	{
		// processed in PostEffectBufferShader::CollectUIValues
	} break;

	default:
		LOGE("unsupported fb property type %d", static_cast<int>(fbType));
		break;
	}
}


void IEffectShaderConnections::ShaderProperty::ReadTextureConnections(ShaderPropertyValue& value, FBProperty* fbProperty)
{
	bool isFound = false;

	if (FBIS(fbProperty, FBPropertyListObject))
	{
		if (FBPropertyListObject* listObjProp = FBCast<FBPropertyListObject>(fbProperty))
		{
			FBComponent* firstObject = (listObjProp->GetCount() > 0) ? listObjProp->GetAt(0) : nullptr;

			if (FBIS(firstObject, FBTexture))
			{
				FBTexture* textureObj = FBCast<FBTexture>(firstObject);

				value.SetType(EPropertyType::TEXTURE);
				value.texture = textureObj;
				isFound = true;
			}
			else if (FBIS(firstObject, EffectShaderUserObject))
			{
				EffectShaderUserObject* shaderObject = FBCast<EffectShaderUserObject>(firstObject);

				value.SetType(EPropertyType::SHADER_USER_OBJECT);
				value.shaderUserObject = shaderObject;
				isFound = true;
			}
		}
	}

	if (!isFound)
	{
		// not assigned object, which could be just a procedural applied current source buffer's texture
		value.texture = nullptr;
		value.shaderUserObject = nullptr;
		value.SetType(EPropertyType::TEXTURE);
	}
}