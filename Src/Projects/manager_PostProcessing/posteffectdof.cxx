
/**	\file	posteffectdof.cxx

Sergei <Neill3d> Solokhin 2018-2024

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

//--- Class declaration
#include "posteffectdof.h"
#include "postpersistentdata.h"
#include "shaderpropertywriter.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "postprocessing_helper.h"
#include <hashUtils.h>

uint32_t EffectShaderDOF::SHADER_NAME_HASH = xxhash32(EffectShaderDOF::SHADER_NAME);

EffectShaderDOF::EffectShaderDOF(FBComponent* ownerIn)
	: PostEffectBufferShader(ownerIn)
{}

const char* EffectShaderDOF::GetUseMaskingPropertyName() const noexcept
{
	return PostPersistentData::DOF_USE_MASKING;
}
const char* EffectShaderDOF::GetMaskingChannelPropertyName() const noexcept
{
	return PostPersistentData::DOF_MASKING_CHANNEL;
}

void EffectShaderDOF::OnPopulateProperties(ShaderPropertyScheme* scheme)
{
	// Sample slots

	scheme->AddProperty("color", "colorSampler")
		.SetType(EPropertyType::TEXTURE)
		.SetFlag(PropertyFlag::SKIP)
		.SetDefaultValue(CommonEffect::ColorSamplerSlot);

	// Core depth of field parameters
	mFocalDistance = scheme->AddProperty(PostPersistentData::DOF_FOCAL_DISTANCE, "focalDistance", EPropertyType::FLOAT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mFocalRange = scheme->AddProperty(PostPersistentData::DOF_FOCAL_RANGE, "focalRange", EPropertyType::FLOAT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mFStop = scheme->AddProperty(PostPersistentData::DOF_FSTOP, "fstop", EPropertyType::FLOAT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mCoC = scheme->AddProperty(PostPersistentData::DOF_COC, "CoC", EPropertyType::FLOAT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	// Rendering parameters

	mSamples = scheme->AddProperty(PostPersistentData::DOF_SAMPLES, "samples", EPropertyType::INT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mRings = scheme->AddProperty(PostPersistentData::DOF_RINGS, "rings", EPropertyType::INT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	// Focus control
	mAutoFocus = scheme->AddProperty(PostPersistentData::DOF_AUTO_FOCUS, "autoFocus", EPropertyType::BOOL)
		.SetRequired(false)
		.GetProxy();

	mFocus = scheme->AddProperty(PostPersistentData::DOF_USE_FOCUS_POINT, "focus", EPropertyType::BOOL)
		.SetRequired(false)
		.GetProxy();

	mFocusPoint = scheme->AddProperty(PostPersistentData::DOF_FOCUS_POINT, "focusPoint", EPropertyType::VEC4)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mManualDOF = scheme->AddProperty("manualdof", "manualdof", EPropertyType::BOOL)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();
	// Near and far DOF blur parameters
	mNDOFStart = scheme->AddProperty("ndofstart", "ndofstart", EPropertyType::FLOAT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();
	mNDOFDist = scheme->AddProperty("ndofdist", "ndofdist", EPropertyType::FLOAT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();
	mFDOFStart = scheme->AddProperty("fdofstart", "fdofstart", EPropertyType::FLOAT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();
	mFDOFDist = scheme->AddProperty("fdofdist", "fdofdist", EPropertyType::FLOAT)
		.GetProxy();

	// Visual enhancement parameters
	mBlurForeground = scheme->AddProperty(PostPersistentData::DOF_BLUR_FOREGROUND, "blurForeground", EPropertyType::BOOL)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mThreshold = scheme->AddProperty(PostPersistentData::DOF_THRESHOLD, "threshold", EPropertyType::FLOAT)
		.SetScale(0.01f)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mGain = scheme->AddProperty(PostPersistentData::DOF_GAIN, "gain", EPropertyType::FLOAT)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mBias = scheme->AddProperty(PostPersistentData::DOF_BIAS, "bias", EPropertyType::FLOAT)
		.SetScale(0.01f)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mFringe = scheme->AddProperty(PostPersistentData::DOF_FRINGE, "fringe", EPropertyType::FLOAT)
		.SetScale(0.01f)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mNoise = scheme->AddProperty(PostPersistentData::DOF_NOISE, "noise", EPropertyType::BOOL)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	// Experimental bokeh shape parameters
	mPentagon = scheme->AddProperty(PostPersistentData::DOF_PENTAGON, "pentagon", EPropertyType::BOOL)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	mFeather = scheme->AddProperty(PostPersistentData::DOF_PENTAGON_FEATHER, "feather", EPropertyType::FLOAT)
		.SetScale(0.01f)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();

	// Debug utilities
	mDebugBlurValue = scheme->AddProperty(PostPersistentData::DOF_DEBUG_BLUR_VALUE, "debugBlurValue", EPropertyType::BOOL)
		.SetFlag(PropertyFlag::SKIP)
		.GetProxy();
}

bool EffectShaderDOF::OnCollectUI(PostEffectContextProxy* effectContext, int maskIndex) const
{
	PostPersistentData* pData = effectContext->GetPostProcessData();
	if (!pData)
		return false;

	FBCamera* camera = effectContext->GetCamera();

	double _focalDistance = pData->FocalDistance;
	double _focalRange = pData->FocalRange;
	double _fstop = pData->FStop;
	int _samples = pData->Samples;
	int _rings = pData->Rings;

	float _useFocusPoint = (pData->UseFocusPoint) ? 1.0f : 0.0f;
	FBVector2d _focusPoint = pData->FocusPoint;

	const bool _blurForeground = pData->BlurForeground;

	double _CoC = pData->CoC;
	double _threshold = pData->Threshold;
	
	double _gain = pData->Gain;
	double _bias = pData->Bias;
	double _fringe = pData->Fringe;
	double _feather = pData->PentagonFeather;

	const bool _debugBlurValue = pData->DebugBlurValue;

	if (pData->UseCameraDOFProperties)
	{
		_focalDistance = camera->FocusSpecificDistance;
		_focalRange = camera->FocusAngle;

		FBModel *pInterest = nullptr;
		FBCameraFocusDistanceSource cameraFocusDistanceSource;
		camera->FocusDistanceSource.GetData(&cameraFocusDistanceSource, sizeof(FBCameraFocusDistanceSource), effectContext->GetEvaluateInfo());
		if (kFBFocusDistanceCameraInterest == cameraFocusDistanceSource)
			pInterest = camera->Interest;
		else if (kFBFocusDistanceModel == cameraFocusDistanceSource)
			pInterest = camera->FocusModel;
		
		if (nullptr != pInterest)
		{
			FBMatrix modelView, modelViewI;

			((FBModel*)camera)->GetMatrix(modelView);
			FBMatrixInverse(modelViewI, modelView);

			FBVector3d lPos;
			pInterest->GetVector(lPos);

			FBTVector p(lPos[0], lPos[1], lPos[2], 1.0);
			FBVectorMatrixMult(p, modelViewI, p);
			double dist = p[0];

			// Dont write to property
			// FocalDistance = dist;
			_focalDistance = dist;
		}
	}
	else
	if (pData->AutoFocus && pData->FocusObject.GetCount() > 0)
	{
		FBMatrix modelView, modelViewI;

		((FBModel*)camera)->GetMatrix(modelView);
		FBMatrixInverse(modelViewI, modelView);

		FBVector3d lPos;
		FBModel *pModel = (FBModel*)pData->FocusObject.GetAt(0);
		pModel->GetVector(lPos);

		FBTVector p(lPos[0], lPos[1], lPos[2]);
		FBVectorMatrixMult(p, modelViewI, p);
		double dist = p[0];

		// Dont write to property
		// FocalDistance = dist;
		_focalDistance = dist;
	}

	ShaderPropertyWriter writer(this, effectContext);

	writer(mFocalDistance, static_cast<float>(_focalDistance))
		(mFocalRange, static_cast<float>(_focalRange))
		(mFStop, static_cast<float>(_fstop))
		(mManualDOF, false)
		(mNDOFStart, 1.0f)
		(mNDOFDist, 2.0f)
		(mFDOFStart, 1.0f)
		(mFDOFDist, 3.0f)
		(mSamples, _samples)
		(mRings, _rings)
		(mCoC, static_cast<float>(_CoC))
		(mBlurForeground, _blurForeground)
		(mThreshold, static_cast<float>(_threshold))
		(mGain, static_cast<float>(_gain))
		(mBias, static_cast<float>(_bias))
		(mFringe, static_cast<float>(_fringe))
		(mFeather, static_cast<float>(_feather))
		(mDebugBlurValue, _debugBlurValue)
		(mNoise, pData->Noise)
		(mPentagon, pData->Pentagon)
		(mFocusPoint, 0.01f * (float)_focusPoint[0], 0.01f * (float)_focusPoint[1], 0.0f, _useFocusPoint);
	
	return true;
}