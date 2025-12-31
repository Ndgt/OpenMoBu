
// posteffectdisplacement.cpp
/*
Sergei <Neill3d> Solokhin 2018-2024

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE
*/

#include "posteffectdisplacement.h"
#include "postpersistentdata.h"
#include "shaderpropertywriter.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "postprocessing_helper.h"
#include <hashUtils.h>

uint32_t EffectShaderDisplacement::SHADER_NAME_HASH = xxhash32(EffectShaderDisplacement::SHADER_NAME);

EffectShaderDisplacement::EffectShaderDisplacement(FBComponent* ownerIn)
	: PostEffectBufferShader(ownerIn)
{}

const char* EffectShaderDisplacement::GetUseMaskingPropertyName() const noexcept
{
	return PostPersistentData::DISP_USE_MASKING;
}
const char* EffectShaderDisplacement::GetMaskingChannelPropertyName() const noexcept
{
	return PostPersistentData::DISP_MASKING_CHANNEL;
}

void EffectShaderDisplacement::OnPopulateProperties(PropertyScheme* scheme)
{
	// publish input connection of the effect
	//  input connections we can use to - look for locations, to read values from a given input data component, bind values from values into shader uniforms

	scheme->AddProperty(ShaderProperty("color", "inputSampler"))
		.SetType(EPropertyType::TEXTURE)
		.SetFlag(PropertyFlag::ShouldSkip, true)
		.SetDefaultValue(CommonEffect::ColorSamplerSlot);
	mTime = scheme->AddProperty(ShaderProperty("time", "iTime", nullptr))
		.SetFlag(PropertyFlag::ShouldSkip, true) // NOTE: skip of automatic reading value and let it be done manually
		.GetProxy();
	mUseQuakeEffect = scheme->AddProperty(ShaderProperty(PostPersistentData::DISP_USE_QUAKE_EFFECT, "useQuakeEffect", nullptr))
		.SetFlag(PropertyFlag::IsFlag, true)
		.GetProxy();

	mXDistMag = scheme->AddProperty(ShaderProperty(PostPersistentData::DISP_MAGNITUDE_X, "xDistMag", nullptr))
		.SetScale(0.0001f)
		.GetProxy();
	mYDistMag = scheme->AddProperty(ShaderProperty(PostPersistentData::DISP_MAGNITUDE_Y, "yDistMag", nullptr))
		.SetScale(0.0001f)
		.GetProxy();
	mXSineCycles = scheme->AddProperty(ShaderProperty(PostPersistentData::DISP_SIN_CYCLES_X, "xSineCycles", nullptr)).GetProxy();
	mYSineCycles = scheme->AddProperty(ShaderProperty(PostPersistentData::DISP_SIN_CYCLES_Y, "ySineCycles", nullptr)).GetProxy();
}

bool EffectShaderDisplacement::OnCollectUI(PostEffectContextProxy* effectContext, int maskIndex) const
{
	const PostPersistentData* postProcess = effectContext->GetPostProcessData();
	if (!postProcess)
		return false;

	// this is a custom logic of updating uniform values

	const double time = (postProcess->Disp_UsePlayTime) ? effectContext->GetLocalTime() : effectContext->GetSystemTime();
	const double timerMult = postProcess->Disp_Speed;
	const double _timer = 0.01 * timerMult * time;

	ShaderPropertyWriter writer(this, effectContext);
	writer(mTime, static_cast<float>(_timer));
	
	return true;
}