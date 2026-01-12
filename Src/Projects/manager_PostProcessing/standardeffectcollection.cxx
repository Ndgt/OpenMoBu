
/**	\file	postprocessing_effect.cxx

Sergei <Neill3d> Solokhin 2018-2026

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

//--- Class declaration
#include "standardeffectcollection.h"
#include "posteffectshader_ssao.h"
#include "posteffectshader_displacement.h"
#include "posteffectshader_motionblur.h"
#include "posteffectshader_lensflare.h"
#include "posteffectshader_color.h"
#include "posteffectshader_dof.h"
#include "posteffectshader_filmgrain.h"
#include "posteffectshader_fisheye.h"
#include "posteffectshader_vignetting.h"
#include "postprocessing_helper.h"
#include "fxmaskingshader.h"
#include "posteffectshader_bilateral_blur.h"
#include "postpersistentdata.h"

#include "mobu_logging.h"
#include <FileUtils.h>

// shared shaders

#define SHADER_DEPTH_LINEARIZE_VERTEX		"\\GLSL\\simple.vsh"
#define SHADER_DEPTH_LINEARIZE_FRAGMENT		"\\GLSL\\depthLinearize.fsh"

// this is a depth based blur, fo SSAO
#define SHADER_BLUR_VERTEX					"\\GLSL\\simple.vsh"
#define SHADER_BLUR_FRAGMENT				"\\GLSL\\blur.fsh"

// this is a simple gaussian image blur
#define SHADER_IMAGE_BLUR_VERTEX			"\\GLSL\\simple.vsh"
#define SHADER_IMAGE_BLUR_FRAGMENT			"\\GLSL\\imageBlur.glslf"

#define SHADER_MIX_VERTEX					"\\GLSL\\simple.vsh"
#define SHADER_MIX_FRAGMENT					"\\GLSL\\mix.fsh"

#define SHADER_DOWNSCALE_VERTEX				"\\GLSL\\downscale.vsh"
#define SHADER_DOWNSCALE_FRAGMENT			"\\GLSL\\downscale.fsh"

#define SHADER_SCENE_MASKED_VERTEX			"\\GLSL\\scene_masked.glslv"
#define SHADER_SCENE_MASKED_FRAGMENT		"\\GLSL\\scene_masked.glslf"


void StandardEffectCollection::ChangeContext()
{
	FreeShaders();
	mNeedReloadShaders = true;
}

bool StandardEffectCollection::ReloadShaders()
{
	bool lSuccess = true;

	if (mNeedReloadShaders)
	{
		if (!LoadShaders())
			lSuccess = false;

		mNeedReloadShaders = false;
	}

	return lSuccess;
}

bool StandardEffectCollection::IsOk() const
{
	if (!mFishEye.get())
		return false;
	if (!mColor.get())
		return false;
	if (!mVignetting.get())
		return false;
	if (!mFilmGrain.get())
		return false;
	if (!mLensFlare.get())
		return false;
	if (!mSSAO.get())
		return false;
	if (!mDOF.get())
		return false;
	if (!mDisplacement.get())
		return false;
	
	if (!mEffectDepthLinearize.get())
		return false;
	if (!mMotionBlur.get())
		return false;
	if (!mEffectBilateralBlur.get())
		return false;
	if (!mEffectBlur.get())
		return false;
	if (!mEffectMix.get())
		return false;
	if (!mEffectDownscale.get())
		return false;

	if (!mShaderSceneMasked.get() || !mShaderSceneMasked->IsValid())
		return false;

	return true;
}

PostEffectBufferShader* StandardEffectCollection::ShaderFactory(const BuildInEffect effectType, FBComponent* pOwner, const char *shadersLocation, bool immediatelyLoad)
{
	PostEffectBufferShader* newEffect = nullptr;

	switch (effectType)
	{
	case BuildInEffect::FISHEYE:
		newEffect = new EffectShaderFishEye(pOwner);
		break;
	case BuildInEffect::COLOR:
		newEffect = new EffectShaderColor(pOwner);
		break;
	case BuildInEffect::VIGNETTE:
		newEffect = new EffectShaderVignetting(pOwner);
		break;
	case BuildInEffect::FILMGRAIN:
		newEffect = new EffectShaderFilmGrain(pOwner);
		break;
	case BuildInEffect::LENSFLARE:
		newEffect = new EffectShaderLensFlare(pOwner);
		break;
	case BuildInEffect::SSAO:
		newEffect = new EffectShaderSSAO(pOwner);
		break;
	case BuildInEffect::DOF:
		newEffect = new EffectShaderDOF(pOwner);
		break;
	case BuildInEffect::DISPLACEMENT:
		newEffect = new EffectShaderDisplacement(pOwner);
		break;
	case BuildInEffect::MOTIONBLUR:
		newEffect = new EffectShaderMotionBlur(pOwner);
		break;
	}

	if (immediatelyLoad && newEffect)
	{
		if (!newEffect->Load(shadersLocation))
		{
			LOGE("Post Effect %s failed to Load from %s\n", newEffect->GetName(), shadersLocation);

			delete newEffect;
			newEffect = nullptr;
		}
	}
	
	return newEffect;
}

bool StandardEffectCollection::CheckShadersPath(const char* path)
{
	const char* test_shaders[] = {
		SHADER_DEPTH_LINEARIZE_VERTEX,
		SHADER_DEPTH_LINEARIZE_FRAGMENT,

		SHADER_BLUR_VERTEX,
		SHADER_BLUR_FRAGMENT,
		SHADER_IMAGE_BLUR_FRAGMENT,

		SHADER_MIX_VERTEX,
		SHADER_MIX_FRAGMENT,

		SHADER_DOWNSCALE_VERTEX,
		SHADER_DOWNSCALE_FRAGMENT,

		SHADER_SCENE_MASKED_VERTEX,
		SHADER_SCENE_MASKED_FRAGMENT
	};
	LOGV("[CheckShadersPath] testing path %s\n", path);
	for (const char* shader_path : test_shaders)
	{
		FBString full_path(path, shader_path);

		if (!IsFileExists(full_path))
		{
			LOGV("[CheckShadersPath] %s is not found\n", shader_path);
			return false;
		}
	}

	return true;
}

bool StandardEffectCollection::LoadShaders()
{
	FreeShaders();
	
	constexpr int PATH_LENGTH = 260;
	char shadersPath[PATH_LENGTH];
	if (!FindEffectLocation(CheckShadersPath, shadersPath, PATH_LENGTH))
	{
		LOGE("[PostProcessing] Failed to find shaders location!\n");
		return false;
	}
	
	LOGE("[PostProcessing] Shaders Location - %s\n", shadersPath);

	constexpr FBComponent* pOwner = nullptr;
	mFishEye.reset(ShaderFactory(BuildInEffect::FISHEYE, pOwner, shadersPath));
	mColor.reset(ShaderFactory(BuildInEffect::COLOR, pOwner, shadersPath));
	mVignetting.reset(ShaderFactory(BuildInEffect::VIGNETTE, pOwner, shadersPath));
	mFilmGrain.reset(ShaderFactory(BuildInEffect::FILMGRAIN, pOwner, shadersPath));
	mLensFlare.reset(ShaderFactory(BuildInEffect::LENSFLARE, pOwner, shadersPath));
	mSSAO.reset(ShaderFactory(BuildInEffect::SSAO, pOwner, shadersPath));
	mDOF.reset(ShaderFactory(BuildInEffect::DOF, pOwner, shadersPath));
	mDisplacement.reset(ShaderFactory(BuildInEffect::DISPLACEMENT, pOwner, shadersPath));
	mMotionBlur.reset(ShaderFactory(BuildInEffect::MOTIONBLUR, pOwner, shadersPath));

	// load shared shaders (blur, mix)

	bool lSuccess = true;

	try
	{
		//
		// DEPTH LINEARIZE

		mEffectDepthLinearize.reset(new PostEffectShaderLinearDepth());
		if (!mEffectDepthLinearize->Load(shadersPath))
		{
			throw std::exception("failed to load and prepare depth linearize effect");
		}

		//
		// BLUR (for SSAO)

		mEffectBlur.reset(new EffectShaderBlurLinearDepth(pOwner));
		if (!mEffectBlur->Load(shadersPath))
		{
			throw std::exception("failed to load and prepare SSAO blur effect");
		}

		//
		// IMAGE BLUR, simple bilateral blur

		mEffectBilateralBlur.reset(new PostEffectShaderBilateralBlur());
		if (!mEffectBilateralBlur->Load(shadersPath))
		{
			throw std::exception("failed to load and prepare image blur effect");
		}

		//
		// MIX

		mEffectMix.reset(new EffectShaderMix());
		if (!mEffectMix->Load(shadersPath))
		{
			throw std::exception("failed to load and prepare mix effect");
		}

		//
		// DOWNSCALE

		mEffectDownscale.reset(new PostEffectShaderDownscale());
		if (!mEffectDownscale->Load(shadersPath))
		{
			throw std::exception("failed to load and prepare downscale effect");
		}

		//
		// SCENE MASKED
		std::unique_ptr<GLSLShaderProgram> pNewShader;
		pNewShader.reset(new GLSLShaderProgram);

		FBString vertex_path = FBString(shadersPath, SHADER_SCENE_MASKED_VERTEX);
		FBString fragment_path = FBString(shadersPath, SHADER_SCENE_MASKED_FRAGMENT);

		if (!pNewShader->LoadShaders(vertex_path, fragment_path))
		{
			throw std::exception("failed to load and prepare downscale shader");
		}

		mShaderSceneMasked.reset(pNewShader.release());

	}
	catch (const std::exception &e)
	{
		LOGE("Post Effect Chain ERROR: %s\n", e.what());
		lSuccess = false;
	}

	return lSuccess;
}


void StandardEffectCollection::FreeShaders()
{
	mFishEye.reset(nullptr);
	mColor.reset(nullptr);
	mVignetting.reset(nullptr);
	mFilmGrain.reset(nullptr);
	mLensFlare.reset(nullptr);
	mSSAO.reset(nullptr);
	mDOF.reset(nullptr);
	mDisplacement.reset(nullptr);
	mMotionBlur.reset(nullptr);

	mEffectDepthLinearize.reset(nullptr);
	mEffectBilateralBlur.reset(nullptr);
	mEffectBlur.reset(nullptr);
	mEffectMix.reset(nullptr);
	mEffectDownscale.reset(nullptr);
}