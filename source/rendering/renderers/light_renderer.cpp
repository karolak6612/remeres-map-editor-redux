//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "../light_drawer.h"

namespace rme {
	namespace render {

		LightRenderer::LightRenderer() {
			lightDrawer_ = std::make_unique<LightDrawer>();
		}

		LightRenderer::~LightRenderer() {
			shutdown();
		}

		void LightRenderer::initialize() {
			if (initialized_) {
				return;
			}
			initialized_ = true;
		}

		void LightRenderer::shutdown() {
			if (!initialized_) {
				return;
			}
			initialized_ = false;
		}

		void LightRenderer::renderLights(const RenderContext& ctx, const RenderOptions& opts) {
			if (!lightDrawer_) {
				return;
			}

			lightDrawer_->draw(ctx.startX, ctx.startY, ctx.endX, ctx.endY, ctx.scrollX, ctx.scrollY, false);
		}

		void LightRenderer::addLight(TileLocation* location) {
			if (!location || !lightDrawer_) {
				return;
			}

			auto tile = location->get();
			if (!tile) {
				return;
			}

			const auto& position = location->getPosition();

			if (tile->ground && tile->ground->hasLight()) {
				lightDrawer_->addLight(position.x, position.y, position.z, tile->ground->getLight());
			}

			if (!tile->items.empty()) {
				for (auto item : tile->items) {
					if (item->hasLight()) {
						lightDrawer_->addLight(position.x, position.y, position.z, item->getLight());
					}
				}
			}
		}

		void LightRenderer::clearLights() {
			if (!lightDrawer_) {
				return;
			}
			lightDrawer_->clear();
		}

	} // namespace render
} // namespace rme
