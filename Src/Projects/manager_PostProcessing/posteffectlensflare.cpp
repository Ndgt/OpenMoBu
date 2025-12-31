
// posteffectlensflare.cpp
/*
Sergei <Neill3d> Solokhin 2018-2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE
*/

#include "posteffectlensflare.h"
#include "postpersistentdata.h"
#include "shaderpropertywriter.h"

#include "postprocessing_helper.h"
#include "math3d.h"
#include <hashUtils.h>
#include <mobu_logging.h>

uint32_t EffectShaderLensFlare::SHADER_NAME_HASH = xxhash32(EffectShaderLensFlare::SHADER_NAME);

EffectShaderLensFlare::EffectShaderLensFlare(FBComponent* ownerIn)
	: PostEffectBufferShader(ownerIn)
{}

const char* EffectShaderLensFlare::GetUseMaskingPropertyName() const noexcept
{
	return PostPersistentData::FLARE_USE_MASKING;
}
const char* EffectShaderLensFlare::GetMaskingChannelPropertyName() const noexcept
{
	return PostPersistentData::FLARE_MASKING_CHANNEL;
}

void EffectShaderLensFlare::OnPopulateProperties(PropertyScheme* scheme)
{
	scheme->AddProperty(ShaderProperty("color", "sampler0"))
		.SetType(EPropertyType::TEXTURE)
		.SetDefaultValue(CommonEffect::ColorSamplerSlot)
		.SetFlag(PropertyFlag::ShouldSkip, true);

	mFlareSeed = scheme->AddProperty(ShaderProperty(PostPersistentData::FLARE_SEED, "flareSeed", nullptr))
		.SetRequired(false)
		.GetProxy();

	mAmount = scheme->AddProperty(ShaderProperty(PostPersistentData::FLARE_AMOUNT, "amount", nullptr))
		.SetScale(0.01f)
		.GetProxy();

	mTime = scheme->AddProperty(ShaderProperty("timer", "iTime", nullptr))
		.SetFlag(PropertyFlag::ShouldSkip, true) // NOTE: skip of automatic reading value and let it be done manually
		.GetProxy();
	mLightPos = scheme->AddProperty(ShaderProperty("light_pos", "light_pos", nullptr))
		.SetFlag(PropertyFlag::ShouldSkip, true) // NOTE: skip of automatic reading value and let it be done manually
		.SetType(EPropertyType::VEC4)
		.GetProxy();

	mTint = scheme->AddProperty(ShaderProperty(PostPersistentData::FLARE_TINT, "tint", nullptr))
		.SetType(EPropertyType::VEC4)
		.SetFlag(PropertyFlag::ShouldSkip, true)
		.GetProxy();

	mInner = scheme->AddProperty(ShaderProperty(PostPersistentData::FLARE_INNER, "inner", nullptr))
		.SetScale(0.01f)
		.GetProxy();
	mOuter = scheme->AddProperty(ShaderProperty(PostPersistentData::FLARE_OUTER, "outer", nullptr))
		.SetScale(0.01f)
		.GetProxy();

	mFadeToBorders = scheme->AddProperty(ShaderProperty(PostPersistentData::FLARE_FADE_TO_BORDERS, "fadeToBorders", nullptr))
		.SetFlag(PropertyFlag::IsFlag, true)
		.SetType(EPropertyType::FLOAT)
		.GetProxy();
	mBorderWidth = scheme->AddProperty(ShaderProperty(PostPersistentData::FLARE_BORDER_WIDTH, "borderWidth", nullptr)).GetProxy();
	mFeather = scheme->AddProperty(ShaderProperty(PostPersistentData::FLARE_BORDER_FEATHER, "feather", nullptr))
		.SetScale(0.01f)
		.GetProxy();
}

bool EffectShaderLensFlare::OnCollectUI(PostEffectContextProxy* effectContext, int maskIndex) const
{
	PostPersistentData* data = effectContext->GetPostProcessData();
	if (!data)
		return false;

	const int numberOfPasses = data->FlareLight.GetCount();
	mNumberOfPasses.store(numberOfPasses, std::memory_order_release);

	const double systemTime = (data->FlareUsePlayTime) ? effectContext->GetLocalTime() : effectContext->GetSystemTime();
	double timerMult = data->FlareTimeSpeed;
	double flareTimer = 0.01 * timerMult * systemTime;
	
	ShaderPropertyWriter writer(this, effectContext);
	writer(mTime, static_cast<float>(flareTimer));

	return true;
}

int EffectShaderLensFlare::GetNumberOfPasses() const
{
	return mNumberOfPasses.load(std::memory_order_acquire);
}

void EffectShaderLensFlare::OnRenderBegin(PostEffectRenderContext& renderContextParent, PostEffectContextProxy* effectContext)
{
	PostPersistentData* data = effectContext->GetPostProcessData();
	if (!data)
		return;

	const int lastShaderIndex = GetCurrentShader();
	const int newShaderIndex = data->FlareType.AsInt();
	SetCurrentShader(newShaderIndex);

	if (newShaderIndex < 0 || newShaderIndex >= NUMBER_OF_SHADERS)
	{
		SetCurrentShader(0);
		const EFlareType newFlareType{ EFlareType::flare1 };
		data->FlareType.SetData((void*)&newFlareType);
	}

	if (lastShaderIndex != newShaderIndex)
	{
		//if (ShaderProperty* flareSeedProperty = GetPropertySchemePtr()->GetProperty(mFlareSeed))
		{
			// flare seed property is used in bubble and anamorphic flare shaders
			//flareSeedProperty->SetRequired(newShaderIndex != EFlareType::flare1);
		}
		
		bIsNeedToUpdatePropertyScheme = true;
	}

	subShaders[mCurrentShader].CollectUIValues(mCurrentShader, effectContext, GetNumberOfPasses(), 0);
}

bool EffectShaderLensFlare::OnRenderPassBegin(int passIndex, PostEffectRenderContext& renderContext)
{
	const int currentShader = GetCurrentShader();
	VERIFY(currentShader >= 0 && currentShader < GetNumberOfVariations());
	const SubShader& subShader = subShaders[currentShader];

	const IEffectShaderConnections::PropertyScheme* propertyScheme = GetPropertySchemePtr();

	if (passIndex >= 0 && passIndex < static_cast<int>(subShader.m_LightPositions.size()))
	{
		const FBVector3d pos(subShader.m_LightPositions[passIndex]);
		renderContext.OverrideUniform(propertyScheme, mLightPos, static_cast<float>(pos[0]), static_cast<float>(pos[1]), static_cast<float>(pos[2]), subShader.m_DepthAttenuation);
		
		const FBColor tint(subShader.m_LightColors[passIndex]);
		renderContext.OverrideUniform(propertyScheme, mTint, static_cast<float>(tint[0]), static_cast<float>(tint[1]), static_cast<float>(tint[2]), 1.0f);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// EffectShaderLensFlare::SubShader

bool EffectShaderLensFlare::SubShader::CollectUIValues(int shaderIndex, PostEffectContextProxy* effectContext, int numberOfPasses, int maskIndex)
{
	PostPersistentData* pData = effectContext->GetPostProcessData();

	double flarePos[3] = { 0.01 * pData->FlarePosX, 0.01 * pData->FlarePosY, 1.0 };

	m_DepthAttenuation = (pData->FlareDepthAttenuation) ? 1.0f : 0.0f;

	if (pData->UseFlareLightObject && pData->FlareLight.GetCount() > 0)
	{
		ProcessLightObjects(effectContext, pData, effectContext->GetCamera(), numberOfPasses, effectContext->GetViewWidth(), effectContext->GetViewHeight(), 
			effectContext->GetSystemTimeDT(), effectContext->GetSystemTime(), flarePos);
	}
	else
	{
		m_LightPositions.clear();
		m_LightColors.clear();
		m_LightAlpha.clear();
	}

	return true;
}

void EffectShaderLensFlare::SubShader::ProcessLightObjects(PostEffectContextProxy* effectContext, PostPersistentData* pData, FBCamera* pCamera, 
	int numberOfPasses, int w, int h, double dt, FBTime systemTime, double* flarePos)
{
	m_LightPositions.resize(numberOfPasses);
	m_LightColors.resize(numberOfPasses);
	m_LightAlpha.resize(numberOfPasses, 0.0f);

	FBMatrix mvp;
	pCamera->GetCameraMatrix(mvp, kFBModelViewProj);

	for (int i = 0; i < numberOfPasses; ++i)
	{
		ProcessSingleLight(effectContext, pData, pCamera, mvp, i, w, h, dt, flarePos);
	}

	// relative coords to a screen size
	pData->FlarePosX = 100.0 * flarePos[0];
	pData->FlarePosY = 100.0 * flarePos[1];
}

void EffectShaderLensFlare::SubShader::ProcessSingleLight(PostEffectContextProxy* effectContext, PostPersistentData* pData, FBCamera* pCamera, FBMatrix& mvp, int index, int w, int h, double dt, double* flarePos)
{
	FBLight* pLight = static_cast<FBLight*>(pData->FlareLight.GetAt(index));

	FBVector3d lightPos;
	pLight->GetVector(lightPos);

	FBVector4d v4;
	FBVectorMatrixMult(v4, mvp, FBVector4d(lightPos[0], lightPos[1], lightPos[2], 1.0));

	v4[0] = static_cast<double>(w) * 0.5 * (v4[0] + 1.0);
	v4[1] = static_cast<double>(h) * 0.5 * (v4[1] + 1.0);

	flarePos[0] = v4[0] / static_cast<double>(w);
	flarePos[1] = v4[1] / static_cast<double>(h);
	flarePos[2] = v4[2];

	m_LightPositions[index].Set(flarePos);
	FBColor color(pLight->DiffuseColor);

	bool isFading = false;

	if (pData->LensFlare_UseOcclusion && pData->FlareOcclusionObjects.GetCount() > 0)
	{
		const int offsetX = pCamera->CameraViewportX;
		const int offsetY = pCamera->CameraViewportY;

		const int x = offsetX + static_cast<int>(v4[0]);
		const int y = offsetY + (h - static_cast<int>(v4[1]));

		FBVector3d camPosition;
		pCamera->GetVector(camPosition);
		
		const double distToLight = VectorLength(VectorSubtract(lightPos, camPosition));

		for (int i = 0, count = pData->FlareOcclusionObjects.GetCount(); i < count; ++i)
		{
			if (FBModel* model = FBCast<FBModel>(pData->FlareOcclusionObjects.GetAt(i)))
			{
				FBVector3d hitPosition, hitNormal;
				if (model->RayCast(pCamera, x, y, hitPosition, hitNormal))
				{
					const double distToHit = VectorLength(VectorSubtract(hitPosition, camPosition));

					if (distToHit < distToLight)
					{
						isFading = true;
						break;
					}
				}
			}

		}
	}

	float alpha = m_LightAlpha[index];
	double occSpeed = 100.0;
	pData->FlareOcclusionSpeed.GetData(&occSpeed, sizeof(double), effectContext->GetEvaluateInfo());
	alpha += static_cast<float>(occSpeed) * ((isFading) ? -dt : dt);
	alpha = clamp01(alpha);

	const double f = smoothstep(0.0, 1.0, static_cast<double>(alpha));

	color[0] *= f;
	color[1] *= f;
	color[2] *= f;

	m_LightColors[index].Set(color);
	m_LightAlpha[index] = alpha;
}

