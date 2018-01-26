// Copyright (C) 2009-2018, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/renderer/RendererObject.h>
#include <anki/resource/TextureResource.h>

namespace anki
{

/// @addtogroup renderer
/// @{

/// Post-processing stage.
class FinalComposite : public RendererObject
{
public:
	/// Load the color grading texture.
	Error loadColorGradingTexture(CString filename);

anki_internal:
	static const PixelFormat RT_PIXEL_FORMAT;

	FinalComposite(Renderer* r);
	~FinalComposite();

	ANKI_USE_RESULT Error init(const ConfigSet& config);

	/// Populate the rendergraph.
	void populateRenderGraph(RenderingContext& ctx);

	RenderTargetHandle getRt() const
	{
		return m_runCtx.m_rt;
	}

private:
	static const U LUT_SIZE = 16;

	FramebufferDescription m_fbDescr;
	RenderTargetDescription m_rtDescr;

	ShaderProgramResourcePtr m_prog;
	Array<ShaderProgramPtr, 2> m_grProgs; ///< One with Dbg and one without

	TextureResourcePtr m_lut; ///< Color grading lookup texture.
	TextureResourcePtr m_blueNoise;

	Bool8 m_sharpenEnabled = false;

	class
	{
	public:
		RenderTargetHandle m_rt;
		RenderingContext* m_ctx = nullptr;
	} m_runCtx;

	ANKI_USE_RESULT Error initInternal(const ConfigSet& config);

	void run(const RenderingContext& ctx, RenderPassWorkContext& rgraphCtx);

	/// A RenderPassWorkCallback for the composite pass.
	static void runCallback(RenderPassWorkContext& rgraphCtx)
	{
		FinalComposite* self = scast<FinalComposite*>(rgraphCtx.m_userData);
		self->run(*self->m_runCtx.m_ctx, rgraphCtx);
	}
};
/// @}

} // end namespace anki