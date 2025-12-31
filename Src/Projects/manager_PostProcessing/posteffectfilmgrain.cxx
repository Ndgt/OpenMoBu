
/**	\file	posteffectfilmgrain.cxx

Sergei <Neill3d> Solokhin 2018-2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

//--- Class declaration
#include "posteffectfilmgrain.h"
#include "postpersistentdata.h"
#include "shaderpropertywriter.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "postprocessing_helper.h"
#include <hashUtils.h>

uint32_t EffectShaderFilmGrain::SHADER_NAME_HASH = xxhash32(EffectShaderFilmGrain::SHADER_NAME);

EffectShaderFilmGrain::EffectShaderFilmGrain(FBComponent* ownerIn)
	: PostEffectBufferShader(ownerIn)
{}

const char* EffectShaderFilmGrain::GetUseMaskingPropertyName() const noexcept
{
	return PostPersistentData::GRAIN_USE_MASKING;
}
const char* EffectShaderFilmGrain::GetMaskingChannelPropertyName() const noexcept
{
	return PostPersistentData::GRAIN_MASKING_CHANNEL;
}

void EffectShaderFilmGrain::OnPopulateProperties(PropertyScheme* scheme)
{
	scheme->AddProperty(ShaderProperty("color", "sampler0"))
		.SetType(EPropertyType::TEXTURE)
		.SetFlag(PropertyFlag::ShouldSkip, true)
		.SetDefaultValue(CommonEffect::ColorSamplerSlot);

	mTimer = scheme->AddProperty(ShaderProperty("time", "timer", EPropertyType::FLOAT))
		.SetFlag(PropertyFlag::ShouldSkip, true) // NOTE: skip of automatic reading value and let it be done manually
		.GetProxy();
	mGrainAmount = scheme->AddProperty(ShaderProperty(PostPersistentData::GRAIN_AMOUNT, "grainamount", nullptr))
		.SetScale(0.01f)
		.SetFlag(PropertyFlag::ShouldSkip, true)
		.GetProxy();
	mColored = scheme->AddProperty(ShaderProperty(PostPersistentData::GRAIN_COLORED, "colored", nullptr))
		.SetFlag(PropertyFlag::ShouldSkip, true)
		.GetProxy();
	mColorAmount = scheme->AddProperty(ShaderProperty(PostPersistentData::GRAIN_COLOR_AMOUNT, "coloramount", nullptr))
		.SetScale(0.01f)
		.SetFlag(PropertyFlag::ShouldSkip, true)
		.GetProxy();
	mGrainSize = scheme->AddProperty(ShaderProperty(PostPersistentData::GRAIN_SIZE, "grainsize", nullptr))
		.SetScale(0.01f)
		.SetFlag(PropertyFlag::ShouldSkip, true)
		.GetProxy();
	mLumAmount = scheme->AddProperty(ShaderProperty(PostPersistentData::GRAIN_LUMAMOUNT, "lumamount", nullptr))
		.SetScale(0.01f)
		.SetFlag(PropertyFlag::ShouldSkip, true)
		.GetProxy();
}

bool EffectShaderFilmGrain::OnCollectUI(PostEffectContextProxy* effectContext, int maskIndex) const
{
	const PostPersistentData* pData = effectContext->GetPostProcessData();
	if (!pData)
		return false;

	const double time = (pData->FG_UsePlayTime) ? effectContext->GetLocalTime() : effectContext->GetSystemTime();

	const double timerMult = pData->FG_TimeSpeed;
	const double _timer = 0.01 * timerMult * time;

	const double _grainamount = pData->FG_GrainAmount;
	const double _colored = (pData->FG_Colored) ? 1.0 : 0.0;
	const double _coloramount = pData->FG_ColorAmount;
	const double _grainsize = pData->FG_GrainSize;
	const double _lumamount = pData->FG_LumAmount;

	ShaderPropertyWriter writer(this, effectContext);
	writer(mTimer, static_cast<float>(_timer))
		(mGrainAmount, static_cast<float>(_grainamount))
		(mColored, static_cast<float>(_colored))
		(mColorAmount, static_cast<float>(_coloramount))
		(mGrainSize, static_cast<float>(_grainsize))
		(mLumAmount, static_cast<float>(_lumamount));
	
	return true;
}