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
#include <nlohmann/json.hpp>
#include <magic_enum.hpp>

#include <fstream>

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
	SetNameHash(other.GetNameHash());
	SetUniformNameHash(other.GetUniformNameHash());

	SetFBComponent(other.GetFBComponent());
	SetFBProperty(other.GetFBProperty());

	flags = other.flags;
	mDefaultValue = other.mDefaultValue;
	indexInArray = other.indexInArray;
	isGeneratedByUniform = other.isGeneratedByUniform;
}

IEffectShaderConnections::ShaderProperty::ShaderProperty(std::string_view nameIn, std::string_view uniformNameIn, FBProperty* fbPropertyIn)
{
	SetName(nameIn);
	SetUniformName(uniformNameIn);

	if (fbPropertyIn)
	{
		const auto propertyType = FBPropertyToShaderPropertyType(fbPropertyIn->GetPropertyType());
		SetType(propertyType);
		SetFBProperty(fbPropertyIn);
	}
}

IEffectShaderConnections::ShaderProperty::ShaderProperty(std::string_view nameIn, std::string_view uniformNameIn, EPropertyType typeIn, FBProperty* fbPropertyIn)
{
	SetType(typeIn);

	SetName(nameIn);
	SetUniformName(uniformNameIn);

	if (fbPropertyIn)
	{
		SetFBProperty(fbPropertyIn);
	}
}

uint32_t IEffectShaderConnections::ShaderProperty::ComputeNameHash(std::string_view s) const
{
	return xxhash32(s.data(), s.size(), HASH_SEED);
}

void IEffectShaderConnections::ShaderProperty::SetName(std::string_view nameIN) 
{ 
	nameHash = ComputeNameHash(nameIN);
	mDefaultValue.SetNameHash(nameHash);
}

void IEffectShaderConnections::ShaderProperty::SetNameHash(uint32_t hashIn)
{
	nameHash = hashIn;
	mDefaultValue.SetNameHash(nameHash);
}

const char* IEffectShaderConnections::ShaderProperty::GetName() const noexcept
{ 
	return ResolveHash32(nameHash); 
}

uint32_t IEffectShaderConnections::ShaderProperty::GetNameHash() const
{
	return nameHash;
}

void IEffectShaderConnections::ShaderProperty::SetUniformName(std::string_view uniformNameIN)
{
	uniformNameHash = ComputeNameHash(uniformNameIN.data());
}

void IEffectShaderConnections::ShaderProperty::SetUniformNameHash(uint32_t hashIn)
{
	uniformNameHash = hashIn;
}

const char* IEffectShaderConnections::ShaderProperty::GetUniformName() const noexcept
{ 
	return ResolveHash32(uniformNameHash);
}

uint32_t IEffectShaderConnections::ShaderProperty::GetUniformNameHash() const
{
	return uniformNameHash;
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

void IEffectShaderConnections::ShaderProperty::ReadFBPropertyValue(
	FBProperty* fbProperty,
	ShaderPropertyValue& value, 
	const ShaderProperty& shaderProperty, 
	const PostEffectContextProxy* effectContext,
	int maskIndex)
{
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
		VERIFY(value.GetType() == EPropertyType::FLOAT); // , "%s | %s\n", fbProperty->GetName(), ResolveHash32(value.GetNameHash()));
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

//////////////////////////////////////////////////////////////////////////
// PropertyScheme

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::PropertyScheme::AddProperty(const ShaderProperty& property)
{
	const uint32_t nameHash = property.GetNameHash();
	VERIFY(nameHash != 0);
	VERIFY(!FindPropertyByHash(nameHash));

	auto& newProp = properties.emplace_back(property);
	newProp.SetIndexInArray(static_cast<int>(properties.size()) - 1);
	return newProp;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::PropertyScheme::AddProperty(ShaderProperty&& property)
{
	const uint32_t nameHash = property.GetNameHash();
	VERIFY(nameHash != 0);
	VERIFY(!FindPropertyByHash(nameHash));

	auto& newProp = properties.emplace_back(std::move(property));
	newProp.SetIndexInArray(static_cast<int>(properties.size()) - 1);
	return newProp;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::PropertyScheme::AddProperty(std::string_view nameIn, std::string_view uniformNameIn, FBProperty* fbPropertyIn)
{
	ShaderProperty property(nameIn, uniformNameIn, fbPropertyIn);
	const uint32_t nameHash = property.GetNameHash();
	VERIFY(nameHash != 0);
	VERIFY(!FindPropertyByHash(nameHash));

	auto& newProp = properties.emplace_back(std::move(property));
	newProp.SetIndexInArray(static_cast<int>(properties.size()) - 1);
	return newProp;
}

IEffectShaderConnections::ShaderProperty& IEffectShaderConnections::PropertyScheme::AddProperty(std::string_view nameIn, std::string_view uniformNameIn, EPropertyType typeIn, FBProperty* fbPropertyIn)
{
	ShaderProperty property(nameIn, uniformNameIn, typeIn, fbPropertyIn);
	const uint32_t nameHash = property.GetNameHash();
	VERIFY(nameHash != 0);
	VERIFY(!FindPropertyByHash(nameHash));

	auto& newProp = properties.emplace_back(std::move(property));
	newProp.SetIndexInArray(static_cast<int>(properties.size()) - 1);
	return newProp;
}

IEffectShaderConnections::ShaderProperty* IEffectShaderConnections::PropertyScheme::FindPropertyByHash(uint32_t nameHash)
{
	auto iter = std::find_if(begin(properties), end(properties), [nameHash](ShaderProperty& prop) {
		return prop.GetNameHash() == nameHash;
		});

	return (iter != end(properties)) ? std::addressof(*iter) : nullptr;
}

const IEffectShaderConnections::ShaderProperty* IEffectShaderConnections::PropertyScheme::FindPropertyByHash(uint32_t nameHash) const
{
	auto iter = std::find_if(begin(properties), end(properties), [nameHash](const ShaderProperty& prop) {
		return prop.GetNameHash() == nameHash;
		});

	return (iter != end(properties)) ? std::addressof(*iter) : nullptr;
}

IEffectShaderConnections::ShaderProperty* IEffectShaderConnections::PropertyScheme::FindProperty(const std::string_view name)
{
	const uint32_t nameHash = xxhash32(name.data(), name.size(), ShaderProperty::HASH_SEED);
	return FindPropertyByHash(nameHash);
}

const IEffectShaderConnections::ShaderProperty* IEffectShaderConnections::PropertyScheme::FindProperty(const std::string_view name) const
{
	const uint32_t nameHash = xxhash32(name.data(), name.size(), ShaderProperty::HASH_SEED);
	return FindPropertyByHash(nameHash);
}

IEffectShaderConnections::ShaderProperty* IEffectShaderConnections::PropertyScheme::FindPropertyByUniform(const std::string_view name)
{
	const uint32_t nameHash = xxhash32(name.data(), name.size(), ShaderProperty::HASH_SEED);

	auto iter = std::find_if(begin(properties), end(properties), [nameHash](ShaderProperty& prop) {
		return prop.GetUniformNameHash() == nameHash;
		});

	return (iter != end(properties)) ? std::addressof(*iter) : nullptr;
}

const IEffectShaderConnections::ShaderProperty* IEffectShaderConnections::PropertyScheme::FindPropertyByUniform(const std::string_view name) const
{
	const uint32_t nameHash = xxhash32(name.data(), name.size(), ShaderProperty::HASH_SEED);

	auto iter = std::find_if(begin(properties), end(properties), [nameHash](const ShaderProperty& prop) {
		return prop.GetUniformNameHash() == nameHash;
		});

	return (iter != end(properties)) ? std::addressof(*iter) : nullptr;
}

IEffectShaderConnections::ShaderProperty* IEffectShaderConnections::PropertyScheme::GetProperty(const IEffectShaderConnections::ShaderPropertyProxy proxy)
{
	if (proxy.index >= 0
		&& proxy.index < static_cast<int>(properties.size())
		&& properties[proxy.index].GetNameHash() == proxy.nameHash)
	{
		return &properties[proxy.index];
	}

	return FindPropertyByHash(proxy.nameHash);
}

const IEffectShaderConnections::ShaderProperty* IEffectShaderConnections::PropertyScheme::GetProperty(const IEffectShaderConnections::ShaderPropertyProxy proxy) const
{
	if (proxy.index >= 0
		&& proxy.index < static_cast<int>(properties.size())
		&& properties[proxy.index].GetNameHash() == proxy.nameHash)
	{
		return &properties[proxy.index];
	}

	return FindPropertyByHash(proxy.nameHash);
}

void IEffectShaderConnections::PropertyScheme::AssociateFBProperties(FBComponent* component)
{
	for (auto& prop : properties)
	{
		FBProperty* fbProp = component->PropertyList.Find(prop.GetName());
		VERIFY_MSG(fbProp || !prop.IsGeneratedByUniform(), "%s\n", prop.GetName());
		if (fbProp)
		{
			prop.SetFBProperty(fbProp);
			prop.SetFBComponent(component);
		}
	}
}

bool IEffectShaderConnections::PropertyScheme::ExportToJSON(const char* fileName) const
{
	// for convenience
	using json = nlohmann::json;
	
	json root = json::object();
	root["properties"] = json::array();

	for (const auto& prop : properties)
	{
		json item = json::object();

		// Basic identifiers
		item["name"] = prop.GetName();
		item["nameHash"] = prop.GetNameHash();
		item["uniformName"] = prop.GetUniformName();
		item["uniformHash"] = prop.GetUniformNameHash();

		// flags
		json flagsItem = json::array();
		for (auto e : magic_enum::enum_values<PropertyFlag>()) {
			if (prop.HasFlag(e))
			{
				flagsItem.push_back(magic_enum::enum_name(e));
			}
		}
		item["flags"] = std::move(flagsItem);

		// Default float data (4 floats)
		json valueItem = json::object();

		const float* df = prop.GetDefaultFloatData();
		json defArr = json::array();
		for (int i = 0; i < 4; ++i)
		{
			float v = (df != nullptr) ? df[i] : 0.0f;
			defArr.push_back(v);
		}
		valueItem["type"] = magic_enum::enum_name(prop.GetDefaultValue().GetType()).data();
		valueItem["isLocationRequired"] = prop.GetDefaultValue().IsRequired();
		valueItem["location"] = prop.GetDefaultValue().GetLocation();
		valueItem["nameHash"] = prop.GetDefaultValue().GetNameHash();
		valueItem["defaultFloat"] = std::move(defArr);

		item["defaultValue"] = std::move(valueItem);

		// Scale
		item["scale"] = prop.GetScale();

		item["index"] = prop.GetIndexInArray();

		item["isGeneratedByUniform"] = prop.IsGeneratedByUniform();

		item["fbComponent"] = prop.GetFBComponent() ? prop.GetFBComponent()->GetFullName() : "Empty";
		item["fbProperty"] = prop.GetFBProperty() ? prop.GetFBProperty()->GetName() : "Empty";
		item["fbPropertyType"] = prop.GetFBProperty() ? prop.GetFBProperty()->GetPropertyTypeName() : "Empty";
		
		root["properties"].push_back(std::move(item));
	}

	// Serialize and write to file
	std::string out;
	try
	{
		out = root.dump(4);
	}
	catch (...)
	{
		return false;
	}

	std::ofstream ofs(fileName, std::ios::out | std::ios::trunc);
	if (!ofs.is_open())
		return false;

	ofs << out;
	ofs.close();
	return true;
}
