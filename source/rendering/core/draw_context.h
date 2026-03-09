//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_RENDERING_CORE_DRAW_CONTEXT_H_
#define RME_RENDERING_CORE_DRAW_CONTEXT_H_

class SpriteBatch;
class PrimitiveRenderer;
class Editor;
struct RenderView;
struct DrawingOptions;

/// Bundles the most commonly passed rendering parameters to reduce
/// parameter bloat across drawer classes. Passed by reference.
struct DrawContext {
	SpriteBatch& sprite_batch;
	const RenderView& view;
	const DrawingOptions& options;
	Editor& editor;
};

#endif
