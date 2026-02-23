//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_IMAGE_MANAGER_H_
#define RME_IMAGE_MANAGER_H_

#include <wx/wx.h>
#include <wx/bmpbndl.h>
#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>
#include <cstdint>
#include <vector>
#include <utility>

struct NVGcontext;

struct PairHash {
	template <class T1, class T2>
	std::size_t operator()(const std::pair<T1, T2>& p) const {
		auto h1 = std::hash<T1> {}(p.first);
		auto h2 = std::hash<T2> {}(p.second);
		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
	}
};

struct NvgCacheKey {
	NVGcontext* ctx;
	std::string assetPath;
	uint32_t tint;

	bool operator==(const NvgCacheKey& other) const {
		return ctx == other.ctx && assetPath == other.assetPath && tint == other.tint;
	}
};

struct NvgCacheKeyHash {
	std::size_t operator()(const NvgCacheKey& k) const {
		auto h1 = std::hash<NVGcontext*> {}(k.ctx);
		auto h2 = std::hash<std::string> {}(k.assetPath);
		auto h3 = std::hash<uint32_t> {}(k.tint);
		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2)) ^ (h3 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
	}
};

class ImageManager {
public:
	static ImageManager& GetInstance();

	// wxWidgets support
	wxBitmapBundle GetBitmapBundle(std::string_view assetPath, const wxColour& tint = wxNullColour);
	wxBitmap GetBitmap(std::string_view assetPath, const wxSize& size = wxDefaultSize, const wxColour& tint = wxNullColour);

	// NanoVG support
	int GetNanoVGImage(NVGcontext* vg, std::string_view assetPath, const wxColour& tint = wxNullColour);

	// OpenGL support
	uint32_t GetGLTexture(std::string_view assetPath);

	// Cleanup
	void ClearCache();

private:
	ImageManager();
	~ImageManager();

	std::string ResolvePath(std::string_view assetPath);

	// Caches
	std::unordered_map<std::string, wxBitmapBundle> m_bitmapBundleCache;
	std::unordered_map<std::pair<std::string, uint32_t>, wxBitmap, PairHash> m_tintedBitmapCache;
	std::unordered_map<NvgCacheKey, int, NvgCacheKeyHash> m_nvgImageCache;
	std::unordered_map<std::string, uint32_t> m_glTextureCache;

	// Helper for tinting
	wxImage TintImage(const wxImage& image, const wxColour& tint);

	// Helper for NanoVG image creation
	int CreateNanoVGImageFromWxImage(NVGcontext* vg, const wxImage& image);
};

// Helper macros for common assets
#define IMAGE_MANAGER ImageManager::GetInstance()

// Shortcut macros - Single Source of Truth for all asset paths
// Common Icons
#include "util/image_constants.h"

#endif
