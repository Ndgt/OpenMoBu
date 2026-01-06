
#pragma once

/** \file   PostProcessContextData.h

Sergei <Neill3d> Solokhin 2022-2025

GitHub page - https://github.com/Neill3d/OpenMoBu
Licensed under The "New" BSD License - https://github.com/Neill3d/OpenMoBu/blob/master/LICENSE

*/

//--- SDK include
#include <fbsdk/fbsdk.h>
#include <map>
#include <limits>

#include "GL/glew.h"

#include "graphics_framebuffer.h"
#include "postpersistentdata.h"

#include "glslShaderProgram.h"
#include "Framebuffer.h"

#include "postprocessing_fonts.h"
#include "posteffectbuffers.h"
#include "posteffectchain.h"
#include "standardeffectcollection.h"
#include "shaderpropertystorage.h"

/// <summary>
/// All post process render data for an ogl context
/// </summary>
struct PostProcessContextData
{
public:
	static const int MAX_PANE_COUNT = 4;

	FBTime				mStartSystemTime;
	double				mLastSystemTime{ std::numeric_limits<double>::max() };
	double				mLastLocalTime{ std::numeric_limits<double>::max() };

	bool			mIsTimeInitialized{ false };

	//
	int				mEvaluatePaneCount{ 0 }; // @see mEvaluatePanes
	int				mRenderPaneCount{ 0 }; // @see mRenderPanes
	
	bool			mSchematicView[MAX_PANE_COUNT];
	bool			mVideoRendering = false;
	std::atomic<bool> isReadyToEvaluate{ false };
	std::atomic<bool> isNeedToResetPaneSettings{ false };

	int				mViewport[4];		// x, y, width, height
	int				mViewerViewport[4];

	int				mEnterId = 0;
	size_t			mFrameId = 0;

	// number of entering in render callback
	constexpr static int MAX_ATTACH_STACK = 10;
	GLint			mAttachedFBO[MAX_ATTACH_STACK]{ 0 };


	//
	MainFrameBuffer						mMainFrameBuffer;

	std::unique_ptr<GLSLShaderProgram>	mShaderSimple;	//!< for simple blit quads on a screen

	struct SPaneData
	{
		PostEffectContextMoBu* fxContext{ nullptr };
		PostPersistentData* data{ nullptr };
		FBCamera* camera{ nullptr };

		void Clear()
		{
			fxContext = nullptr;
			data = nullptr;
			camera = nullptr;
		}
	};
	
	SPaneData	mEvaluatePanes[MAX_PANE_COUNT];	//!< choose a propriate settings according to a pane camera
	SPaneData	mRenderPanes[MAX_PANE_COUNT];
	std::array<std::unique_ptr<PostEffectContextMoBu>, MAX_PANE_COUNT> mFXContexts; //!< temporary contexts for each pane

	// for each persistent data object we have a separate post fx context
	//std::unordered_map<PostPersistentData*, std::unique_ptr<PostEffectContextMoBu>>	mPostFXContextsMap;

	// build-in effects collection to be re-used per effect chain
	StandardEffectCollection standardEffectsCollection;

	// if each pane has different size (in practice should be not more then 2
	std::array< std::unique_ptr<PostEffectBuffers>, MAX_PANE_COUNT> mPaneEffectBuffers;
	
	void    Init();
	
	void VideoRenderingBegin();
	void VideoRenderingEnd();

	void	PreRenderFirstEntry();

	// run in custom thread to evaluate the processing data
	void	Evaluate(FBTime systemTime, FBTime localTime, FBEvaluateInfo* pEvaluateInfoIn);
	void	Synchronize();

	void	RenderBeforeRender(bool processCompositions);
	bool	RenderAfterRender(bool processCompositions, FBTime systemTime, FBTime localTime, FBEvaluateInfo* pEvaluateInfoIn);

	// thread-safe, atomic read the ready to evaluate flag
	bool IsReadyToEvaluate() const;
	// thread-safe, atomic update the ready to evaluate flag
	void SetReadyToEvaluate(bool ready);

	bool IsNeedToResetPaneSettings() const;
	void SetNeedToResetPaneSettings(bool reset);

	bool HasAnyShadersReloadRequests(PostPersistentData* data) const;
	void ClearShadersReloadRequests(PostPersistentData* data);
	void ReloadShaders(PostPersistentData* data, PostEffectContextMoBu* fxContext,
		FBEvaluateInfo* pEvaluateInfoIn, FBCamera* pCamera, const PostEffectContextProxy::Parameters& contextParameters);

private:
    bool EmptyGLErrorStack();

	bool PrepPaneSettings();

	// manager shaders
	bool	LoadShaders();
	const bool CheckShadersPath(const char* path) const;
	void	FreeShaders();

	void	FreeBuffers();

	void PrepareContextParameters(PostEffectContextProxy::Parameters& contextParametersOut, FBTime systemTime, FBTime localTime) const;
	void PrepareContextParametersForCamera(PostEffectContextProxy::Parameters& contextParametersOut, FBCamera* pCamera, int nPane) const;

	// once we load file, we should reset pane user object pointers 
	// and wait for next PrepPaneSettings call
	void	ResetPaneSettings();

	void	DrawHUD(int panex, int paney, int panew, int paneh, int vieww, int viewh);
	void	DrawHUDRect(FBHUDRectElement *pElem, int panex, int paney, int panew, int paneh, int vieww, int viewh);
#if defined(HUD_FONT)
	void	DrawHUDText(FBHUDTextElement *pElem, CFont *pFont, int panex, int paney, int panew, int paneh, int vieww, int viewh);
#endif
	void	FreeFonts();


#if defined(HUD_FONT)
	std::vector<CFont*>					mElemFonts;
#endif
	std::vector<FBHUDRectElement*>		mRectElements;
	std::vector<FBHUDTextElement*>		mTextElements;

};


