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


PostEffectContextMoBu::PostEffectContextMoBu(FBCamera* cameraIn,
	FBComponent* userObjectIn, 
	PostPersistentData* postProcessDataIn, 
	FBEvaluateInfo* pEvaluateInfoIn, 
	StandardEffectCollection* effectCollectionIn,
	const Parameters& parametersIn)
		: postProcessData(postProcessDataIn)
		, effectCollection(effectCollectionIn)
		, effectChain(postProcessDataIn)
{
	UpdateContextParameters(cameraIn, parametersIn);
}

StandardEffectCollection* PostEffectContextMoBu::GetEffectCollection() const noexcept
{
	return effectCollection;
}

const PostEffectContextMoBu::Cache& PostEffectContextMoBu::GetReadCache() const
{
	const Cache& c = mCache[mReadIndex.load(std::memory_order_acquire)];
	return c;
}
PostEffectContextMoBu::Cache& PostEffectContextMoBu::GetWriteCache()
{
	const uint32_t writeIndex = 1 - mReadIndex.load(std::memory_order_acquire);
	return mCache[writeIndex];
}
void PostEffectContextMoBu::SwapCacheIndices()
{
	const uint32_t writeIndex = 1 - mReadIndex.load(std::memory_order_acquire);
	mReadIndex.store(writeIndex, std::memory_order_release);
}

int PostEffectContextMoBu::GetViewWidth() const noexcept 
{ 
	return GetReadCache().parameters.w;
}
int PostEffectContextMoBu::GetViewHeight() const noexcept 
{ 
	return GetReadCache().parameters.h;
}

int PostEffectContextMoBu::GetLocalFrame() const noexcept 
{ 
	return GetReadCache().parameters.localFrame;
}
double PostEffectContextMoBu::GetSystemTime() const noexcept 
{ 
	return GetReadCache().parameters.sysTime;
}
double PostEffectContextMoBu::GetLocalTime() const noexcept 
{ 
	return GetReadCache().parameters.localTime;
}

double PostEffectContextMoBu::GetLocalTimeDT() const noexcept 
{ 
	return GetReadCache().parameters.localTimeDT;
}
double PostEffectContextMoBu::GetSystemTimeDT() const noexcept 
{ 
	return GetReadCache().parameters.sysTimeDT;
}

double* PostEffectContextMoBu::GetCameraPosition() const noexcept 
{ 
	return GetReadCache().cameraPosition; 
}
const float* PostEffectContextMoBu::GetCameraPositionF() const noexcept 
{ 
	return GetReadCache().cameraPositionF; 
}

float PostEffectContextMoBu::GetCameraNearDistance() const noexcept 
{ 
	return GetReadCache().zNear; 
}
float PostEffectContextMoBu::GetCameraFarDistance() const noexcept 
{ 
	return GetReadCache().zFar; 
}

bool PostEffectContextMoBu::IsCameraOrthogonal() const noexcept 
{ 
	return GetReadCache().isCameraOrtho;
}

double* PostEffectContextMoBu::GetModelViewMatrix() const noexcept 
{ 
	return GetReadCache().modelView;
}
const float* PostEffectContextMoBu::GetModelViewMatrixF() const noexcept 
{ 
	return GetReadCache().modelViewF;
}
double* PostEffectContextMoBu::GetProjectionMatrix() const noexcept 
{ 
	return GetReadCache().projection;
}
const float* PostEffectContextMoBu::GetProjectionMatrixF() const noexcept 
{ 
	return GetReadCache().projectionF;
}
double* PostEffectContextMoBu::GetModelViewProjMatrix() const noexcept 
{ 
	return GetReadCache().modelViewProj;
}
const float* PostEffectContextMoBu::GetModelViewProjMatrixF() const noexcept 
{ 
	return GetReadCache().modelViewProjF;
}
// returns the modelview-projection matrix of the previous frame
const float* PostEffectContextMoBu::GetPrevModelViewProjMatrixF() const noexcept 
{ 
	return GetReadCache().prevModelViewProjF;
}
// returns the inverse of the modelview-projection matrix
const float* PostEffectContextMoBu::GetInvModelViewProjMatrixF() const noexcept 
{ 
	return GetReadCache().invModelViewProjF;
}
	
// 4 floats in format - year + 1900, month + 1, day, seconds since midnight
const float* PostEffectContextMoBu::GetIDate() const noexcept 
{ 
	return GetReadCache().iDate;
}

FBCamera* PostEffectContextMoBu::GetCamera() const 
{ 
	return GetReadCache().camera;
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

void PostEffectContextMoBu::UpdateContextParameters(FBCamera* cameraIn, const Parameters& parametersIn)
{
	Cache& writeCache = GetWriteCache();
	writeCache.camera = cameraIn;
	writeCache.parameters = parametersIn;
	PrepareCache(writeCache, cameraIn);
}

void PostEffectContextMoBu::Evaluate(
	FBEvaluateInfo* pEvaluateInfoIn, 
	FBCamera* cameraIn, 
	const Parameters& parametersIn)
{
	UpdateContextParameters(cameraIn, parametersIn);
	effectChain.Evaluate(this, pEvaluateInfoIn, cameraIn);
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
}

bool PostEffectContextMoBu::IsReadyToRender() const
{
	return effectChain.IsReadyToRender();
}

bool PostEffectContextMoBu::Render(FBEvaluateInfo* pEvaluateInfoIn, PostEffectBuffers* buffers)
{
	if (!effectChain.IsReadyToRender())
	{
		return false;
	}
	const double time = GetReadCache().parameters.localTime;
	return effectChain.Render(buffers, time, this, pEvaluateInfoIn);
}


void PostEffectContextMoBu::PrepareCache(Cache& cacheOut, FBCamera* camera)
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