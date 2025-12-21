#pragma once

/**	\file	posteffectcontextmobu.h

Sergei <Neill3d> Solokhin 2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

#include "posteffectcontext.h"

//--- SDK include
#include <fbsdk/fbsdk.h>

#include <limits>
#include <ctime>
#include <stdint.h>

#include "posteffectchain.h"
#include "shaderpropertystorage.h"

// forward declarations
class StandardEffectCollection;

// TODO: mark to delete in case context is not in use any more !
/**
* effect context, thread-safe
* implementation class is used to read from UI and build the data
* interface read methods can be used to read from render thread in a safe manner
*/
class PostEffectContextMoBu : public IPostEffectContext
{
public:

	PostEffectContextMoBu(FBCamera* cameraIn,
		FBComponent* userObjectIn,
		PostPersistentData* postProcessDataIn,
		FBEvaluateInfo* pEvaluateInfoIn,
		StandardEffectCollection* effectCollectionIn,
		const Parameters& parametersIn);

	[[nodiscard]] int GetViewWidth() const noexcept;
	[[nodiscard]] int GetViewHeight() const noexcept;

	[[nodiscard]] int GetLocalFrame() const noexcept;
	[[nodiscard]] double GetSystemTime() const noexcept;
	[[nodiscard]] double GetLocalTime() const noexcept;

	[[nodiscard]] double GetLocalTimeDT() const noexcept;
	[[nodiscard]] double GetSystemTimeDT() const noexcept;

	virtual double* GetCameraPosition() const noexcept override;
	virtual const float* GetCameraPositionF() const noexcept override;

	virtual float GetCameraNearDistance() const noexcept override;
	virtual float GetCameraFarDistance() const noexcept override;

	virtual bool IsCameraOrthogonal() const noexcept override;

	virtual double* GetModelViewMatrix() const noexcept override;
	virtual const float* GetModelViewMatrixF() const noexcept override;
	virtual double* GetProjectionMatrix() const noexcept override;
	virtual const float* GetProjectionMatrixF() const noexcept override;
	virtual double* GetModelViewProjMatrix() const noexcept override;
	virtual const float* GetModelViewProjMatrixF() const noexcept override;
	// returns the modelview-projection matrix of the previous frame
	virtual const float* GetPrevModelViewProjMatrixF() const noexcept override;
	// returns the inverse of the modelview-projection matrix
	virtual const float* GetInvModelViewProjMatrixF() const noexcept override;
	
	// 4 floats in format - year + 1900, month + 1, day, seconds since midnight
	virtual const float* GetIDate() const noexcept override;

	inline FBCamera* GetCamera() const override;
	
	inline StandardEffectCollection* GetEffectCollection() const noexcept;
	inline PostPersistentData* GetPostProcessData() const override;
	inline const PostEffectChain* GetFXChain() const override;
	inline PostEffectChain* GetFXChain();
	inline const ShaderPropertyStorage* GetShaderPropertyStorage() const override;
	inline ShaderPropertyStorage* GetShaderPropertyStorage();

	// evaluate thread to read UI and prepare cache for render
	void Evaluate(
		FBEvaluateInfo* pEvaluateInfoIn,
		FBCamera* cameraIn,
		const Parameters& parametersIn);

	// synchronize between evaluate and render threads
	void Synchronize();

	// notify about graphics context change, clear all hardware resources
	void ChangeContext();


	bool IsReadyToRender() const;

	bool Render(FBEvaluateInfo* pEvaluateInfoIn, 
		PostEffectBuffers* buffers);

private:

	struct alignas(16) Cache
	{
		// playback and viewport parameters
		Parameters parameters;

		// camera matrices and position

		FBMatrix	modelView;
		FBMatrix	projection;
		FBMatrix	modelViewProj;
		FBMatrix	invModelViewProj;
		FBMatrix	prevModelViewProj;
		FBVector3d	cameraPosition;

		mutable FBComponent* userObject{ nullptr }; //!< this is a component where all ui properties are exposed
		FBCamera* camera{ nullptr }; //!< current camera that we are drawing with
		
		float zNear;
		float zFar;

		float modelViewF[16];
		float projectionF[16];
		float modelViewProjF[16];
		float invModelViewProjF[16];
		float prevModelViewProjF[16];
		float cameraPositionF[3];
		float iDate[4];

		bool isCameraOrtho{ false };
	};

	Cache mCache[2];
	std::atomic<uint32_t> mReadIndex{ 0 };
	
	// that is a key to the context and have to be the same in any thread	
	PostPersistentData* postProcessData{ nullptr }; //!< this is a main post process object for common effects properties
	
	StandardEffectCollection* effectCollection{ nullptr }; //!< standard effects collection to use

	PostEffectChain effectChain; //!< build a chain from a postProcessData
	
	// NOTE: this class is already thread-safe, call shaderPropertyStorage.CommitWrite(0);
	ShaderPropertyStorage shaderPropertyStorage; //!< storage for shader properties


	const Cache& GetReadCache() const;
	Cache& GetWriteCache();
	void SwapCacheIndices();

	// update parameters for write cache
	void UpdateContextParameters(FBCamera* cameraIn, const Parameters& parametersIn);

	void PrepareCache(Cache& cacheOut, FBCamera* camera);
};