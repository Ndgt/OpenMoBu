#pragma once

/**	\file	posteffectcontext.h

Sergei <Neill3d> Solokhin 2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

//--- SDK include
#include <fbsdk/fbsdk.h>
#include "hashUtils.h"
#include <array>
#include <mobu_logging.h>

// forward
class PostPersistentData;
class PostEffectChain;
class ShaderPropertyStorage;
class StandardEffectCollection;

/**
* keep track of each folded shader name and its parents
*/
class PostEffectNameContext
{
public:
	void PushName(const char* childName) {
		VERIFY(currentLevel < MAX_LEVELS - 1);
		currentLevel = currentLevel < MAX_LEVELS - 1 ? currentLevel + 1 : currentLevel;
		strcpy_s(names[currentLevel].data(), names[currentLevel].size(), childName);
	}

	void PopName() {
		VERIFY(currentLevel >= 0);
		currentLevel = currentLevel >= 0 ? currentLevel - 1 : EMPTY_LEVEL;
	}

	const char* GetParentName() const noexcept
	{
		return (currentLevel > 0) ? names[currentLevel - 1].data() : "";
	}
	uint32_t GetParentNameHash() const noexcept
	{
		return xxhash32(GetParentName());
	}

	const char* GetName() const noexcept
	{
		return (currentLevel >= 0) ? names[currentLevel].data() : "";
	}
	uint32_t GetNameHash() const noexcept
	{
		return xxhash32(GetName());
	}
private:

	static const int32_t MAX_LEVELS{ 2 };
	static const int32_t EMPTY_LEVEL{ -1 };
	// keep track of all folded effect names we process
	std::array<std::array<char, 64>, MAX_LEVELS> names;
	int32_t currentLevel{ EMPTY_LEVEL };
};

/**
* effect context, thread-safe
* implementation class is used to read from UI and build the data
* interface read methods can be used to read from render thread in a safe manner
*/
class IPostEffectContext
{
public:
	struct Parameters
	{
		FBMatrix prevModelViewProjMatrix; //!< modelview-projection matrix of the previous frame

		double sysTime{ 0.0 }; //!< system time (in seconds)
		double sysTimeDT{ 0.0 };

		double localTime{ 0.0 }; //!< playback time (in seconds)
		double localTimeDT{ 0.0 };

		int w{ 1 }; //!< viewport width
		int h{ 1 }; //!< viewport height
		int localFrame{ 0 }; //!< playback frame number
		int padding{ 0 }; //!< padding for alignment
	};

public:
	virtual ~IPostEffectContext() = default;

	// interface to query the needed data

	[[nodiscard]] virtual int GetViewWidth() const noexcept = 0;
	[[nodiscard]] virtual int GetViewHeight() const noexcept = 0;

	[[nodiscard]] virtual int GetLocalFrame() const noexcept = 0;
	[[nodiscard]] virtual double GetSystemTime() const noexcept = 0;
	[[nodiscard]] virtual double GetLocalTime() const noexcept = 0;

	[[nodiscard]] virtual double GetLocalTimeDT() const noexcept = 0;
	[[nodiscard]] virtual double GetSystemTimeDT() const noexcept = 0;

	[[nodiscard]] virtual double* GetCameraPosition() const noexcept = 0;
	[[nodiscard]] virtual const float* GetCameraPositionF() const noexcept = 0;

	[[nodiscard]] virtual float GetCameraNearDistance() const noexcept = 0;
	[[nodiscard]] virtual float GetCameraFarDistance() const noexcept = 0;

	[[nodiscard]] virtual bool IsCameraOrthogonal() const noexcept = 0;

	[[nodiscard]] virtual double* GetModelViewMatrix() const noexcept = 0;
	[[nodiscard]] virtual const float* GetModelViewMatrixF() const noexcept = 0;
	[[nodiscard]] virtual double* GetProjectionMatrix() const noexcept = 0;
	[[nodiscard]] virtual const float* GetProjectionMatrixF() const noexcept = 0;
	[[nodiscard]] virtual double* GetModelViewProjMatrix() const noexcept = 0;
	[[nodiscard]] virtual const float* GetModelViewProjMatrixF() const noexcept = 0;
	// returns the modelview-projection matrix of the previous frame
	[[nodiscard]] virtual const float* GetPrevModelViewProjMatrixF() const noexcept = 0;
	// returns the inverse of the modelview-projection matrix
	[[nodiscard]] virtual const float* GetInvModelViewProjMatrixF() const noexcept = 0;

	// 4 floats in format - year + 1900, month + 1, day, seconds since midnight
	[[nodiscard]] virtual const float* GetIDate() const noexcept = 0;

	[[nodiscard]] virtual StandardEffectCollection* GetEffectCollection() const noexcept = 0;
	[[nodiscard]] virtual FBCamera* GetCamera() const = 0;
	[[nodiscard]] virtual PostPersistentData* GetPostProcessData() const = 0;
	[[nodiscard]] virtual const PostEffectChain* GetFXChain() const = 0;
	[[nodiscard]] virtual PostEffectChain* GetFXChain() = 0;
	[[nodiscard]] virtual const ShaderPropertyStorage* GetShaderPropertyStorage() const = 0;
	[[nodiscard]] virtual ShaderPropertyStorage* GetShaderPropertyStorage() = 0;
};