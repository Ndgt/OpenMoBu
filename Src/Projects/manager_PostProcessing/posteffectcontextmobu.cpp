/**	\file	posteffectcontextmobu.cpp

Sergei <Neill3d> Solokhin 2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

#include "posteffectcontextmobu.h"

#include <limits>
#include <ctime>

#include "posteffectchain.h"
#include "shaderpropertystorage.h"
#include "standardeffectcollection.h"
#include "posteffect_userobject.h"


PostEffectContextMoBu::PostEffectContextMoBu(FBCamera* cameraIn,
	FBComponent* userObjectIn, 
	PostPersistentData* postProcessDataIn, 
	FBEvaluateInfo* pEvaluateInfoIn, 
	StandardEffectCollection* effectCollectionIn,
	const PostEffectContextProxy::Parameters& parametersIn)
		: postProcessData(postProcessDataIn)
		, standardEffects(effectCollectionIn)
		, effectChain(postProcessDataIn)
{
	UpdateContextParameters(cameraIn, parametersIn);
}

StandardEffectCollection* PostEffectContextMoBu::GetEffectCollection() const noexcept
{
	return standardEffects;
}

const PostEffectContextProxy::Cache& PostEffectContextMoBu::GetReadCache() const
{
	const PostEffectContextProxy::Cache& c = mCache[mReadIndex.load(std::memory_order_acquire)];
	return c;
}
PostEffectContextProxy::Cache& PostEffectContextMoBu::GetWriteCache()
{
	const uint32_t writeIndex = 1 - mReadIndex.load(std::memory_order_acquire);
	return mCache[writeIndex];
}
void PostEffectContextMoBu::SwapCacheIndices()
{
	const uint32_t writeIndex = 1 - mReadIndex.load(std::memory_order_acquire);
	mReadIndex.store(writeIndex, std::memory_order_release);
}

PostPersistentData* PostEffectContextMoBu::GetPostProcessData() const
{ 
	return postProcessData; 
}

const PostEffectChain* PostEffectContextMoBu::GetFXChain() const 
{ 
	return &effectChain; 
}
PostEffectChain* PostEffectContextMoBu::GetFXChain() { 
	return &effectChain; 
}
const ShaderPropertyStorage* PostEffectContextMoBu::GetShaderPropertyStorage() const 
{ 
	return &shaderPropertyStorage; 
}
ShaderPropertyStorage* PostEffectContextMoBu::GetShaderPropertyStorage() 
{ 
	return &shaderPropertyStorage; 
}

void PostEffectContextMoBu::UpdateContextParameters(FBCamera* cameraIn, const PostEffectContextProxy::Parameters& parametersIn)
{
	PostEffectContextProxy::Cache& writeCache = GetWriteCache();
	writeCache.camera = cameraIn;
	writeCache.parameters = parametersIn;
	PrepareCache(writeCache, cameraIn);
}

void PostEffectContextMoBu::Evaluate(
	FBEvaluateInfo* pEvaluateInfoIn, 
	FBCamera* cameraIn, 
	const PostEffectContextProxy::Parameters& parametersIn)
{
	UpdateContextParameters(cameraIn, parametersIn);

	PostEffectContextProxy proxy(
		cameraIn,
		pEvaluateInfoIn,
		standardEffects,
		postProcessData,
		&effectChain,
		&shaderPropertyStorage.GetWriteEffectMap(),
		GetWriteCache());

	effectChain.Evaluate(&proxy);
}

void PostEffectContextMoBu::Synchronize()
{
	SwapCacheIndices();
	effectChain.Synchronize();
	shaderPropertyStorage.CommitWrite(0);
}

void PostEffectContextMoBu::ChangeContext()
{
	effectChain.ChangeContext();
	shaderPropertyStorage.Clear();
}

bool PostEffectContextMoBu::IsReadyToRender() const
{
	return effectChain.IsReadyToRender();
}

bool PostEffectContextMoBu::IsAnyReloadShadersRequested() const
{
	if (!standardEffects || !postProcessData)
	{
		return false;
	}

	return (standardEffects->IsNeedToReloadShaders()
		|| postProcessData->IsNeedToReloadShaders(false)
		|| postProcessData->IsExternalReloadRequested());
}

bool PostEffectContextMoBu::ReloadShaders()
{
	if (!standardEffects || !postProcessData)
	{
		return false;
	}

	// standard effects
	constexpr const bool propagateToUserEffects = false;
	if (postProcessData->IsNeedToReloadShaders(propagateToUserEffects)
		|| standardEffects->IsNeedToReloadShaders())
	{
		standardEffects->ChangeContext();
		if (!standardEffects->ReloadShaders())
		{
			return false;
		}
	}

	// user effects

	if (postProcessData->IsExternalReloadRequested())
	{
		for (int i = 0; i < postProcessData->UserEffects.GetCount(); ++i)
		{
			FBComponent* component = postProcessData->UserEffects.GetAt(i);
			PostEffectUserObject* userEffect = FBCast<PostEffectUserObject>(component);
			if (userEffect)
			{
				if (userEffect->IsNeedToReloadShaders())
				{
					if (!userEffect->DoReloadShaders())
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool PostEffectContextMoBu::Render(FBEvaluateInfo* pEvaluateInfoIn, PostEffectBuffers* buffers)
{
	if (!effectChain.IsReadyToRender())
	{
		return false;
	}
	const double time = GetReadCache().parameters.localTime;

	PostEffectContextProxy proxy(
		GetReadCache().camera,
		pEvaluateInfoIn,
		standardEffects,
		postProcessData,
		&effectChain,
		&shaderPropertyStorage.GetReadEffectMap(),
		GetReadCache());

	return effectChain.Render(buffers, time, &proxy);
}


void PostEffectContextMoBu::PrepareCache(PostEffectContextProxy::Cache& cacheOut, FBCamera* camera)
{
	if (!camera)
		return;
	
	cacheOut.zNear = static_cast<float>(camera->NearPlaneDistance);
	cacheOut.zFar = static_cast<float>(camera->FarPlaneDistance);
		
	cacheOut.isCameraOrtho = (camera->Type == FBCameraType::kFBCameraTypeOrthogonal);

	camera->GetVector(cacheOut.cameraPosition, kModelTranslation, true);
	for (int i = 0; i < 3; ++i)
		cacheOut.cameraPositionF[i] = static_cast<float>(cacheOut.cameraPosition[i]);

	camera->GetCameraMatrix(cacheOut.modelView, FBCameraMatrixType::kFBModelView);
	camera->GetCameraMatrix(cacheOut.projection, FBCameraMatrixType::kFBProjection);
	camera->GetCameraMatrix(cacheOut.modelViewProj, FBCameraMatrixType::kFBModelViewProj);
	FBMatrixInverse(cacheOut.invModelViewProj, cacheOut.modelViewProj);
	cacheOut.prevModelViewProj = cacheOut.parameters.prevModelViewProjMatrix;

	for (int i = 0; i < 16; ++i)
	{
		cacheOut.modelViewF[i] = static_cast<float>(cacheOut.modelView[i]);
		cacheOut.projectionF[i] = static_cast<float>(cacheOut.projection[i]);
		cacheOut.modelViewProjF[i] = static_cast<float>(cacheOut.modelViewProj[i]);
		cacheOut.invModelViewProjF[i] = static_cast<float>(cacheOut.invModelViewProj[i]);
		cacheOut.prevModelViewProjF[i] = static_cast<float>(cacheOut.prevModelViewProj[i]);
	}
		
	std::time_t now = std::time(nullptr);
	std::tm localTime;
	localtime_s(&localTime, &now);  // now should be of type std::time_t

	const float secondsSinceMidnight = static_cast<float>(localTime.tm_hour * 3600 + localTime.tm_min * 60 + localTime.tm_sec);
	cacheOut.iDate[0] = static_cast<float>(localTime.tm_year + 1900);
	cacheOut.iDate[1] = static_cast<float>(localTime.tm_mon + 1);
	cacheOut.iDate[2] = static_cast<float>(localTime.tm_mday);
	cacheOut.iDate[3] = secondsSinceMidnight;
}