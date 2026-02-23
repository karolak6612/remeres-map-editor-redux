#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <thread>
#include <random>

// Mock dependencies
#include "app/main.h"

// Define GLAD_H to prevent glad from being included again by headers that check it
// But we need the symbols.
// The issue is collision between system gl.h and our mock glad.h
// Since this is a test and we are linking against objects that use glad, we must use glad.
// However, wxWidgets includes gl.h which conflicts.
// We need to disable GL inclusions in the test if possible, or make them compatible.
// For the benchmark, we don't need wxGLCanvas, but MapCanvas inherits from it.
// We can try to rely on the fact that we don't actually call GL functions.

#include "app/definitions.h"
#include "editor/editor.h"
#include "map/map.h"
#include "map/tile.h"
#include "rendering/drawers/map_layer_drawer.h"
#include "rendering/drawers/tiles/tile_renderer.h"
#include "rendering/core/render_view.h"
#include "rendering/core/drawing_options.h"
#include "rendering/core/light_buffer.h"
#include "rendering/utilities/render_benchmark.h"
#include "rendering/drawers/overlays/grid_drawer.h"
#include "rendering/drawers/entities/item_drawer.h"
#include "rendering/drawers/entities/sprite_drawer.h"
#include "rendering/drawers/entities/creature_drawer.h"
#include "rendering/drawers/entities/creature_name_drawer.h"
#include "rendering/drawers/tiles/floor_drawer.h"
#include "rendering/drawers/overlays/marker_drawer.h"
#include "rendering/ui/tooltip_drawer.h"
#include "rendering/core/sprite_batch.h"
#include "rendering/core/primitive_renderer.h"
#include "ui/gui.h"
#include "game/items.h" // Include ItemDatabase definition

// Define global GUI object (needed by some components)
GUI g_gui;
ItemDatabase g_items; // Use correct type
Materials g_materials;

// Mock implementations or minimal setup for OpenGL-dependent classes
// We need to ensure they don't crash when called in headless mode.
// Since we are linking against the real object files, we rely on the fact that
// we won't initialize an OpenGL context, so GL calls might fail or do nothing.
// However, SpriteBatch buffers data to CPU memory first, so as long as we don't Flush to GPU, we might be safe.

int main() {
    std::cout << "Starting Rendering Benchmark..." << std::endl;

    // 1. Setup Editor and Map
    CopyBuffer copyBuffer;
    MapVersion version = MapVersion::v860; // Example version
    Editor editor(copyBuffer, version);

    // Create a 2000x2000 map
    int width = 2000;
    int height = 2000;
    int z = 7;

    std::cout << "Generating " << width << "x" << height << " map..." << std::endl;

    // Use a deterministic seed for reproducibility
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> itemDist(100, 200); // Random item IDs

    // Populate g_items to prevent crashes
    // Ensure vector is large enough
    if (g_items.items.size() <= 200) {
        g_items.items.resize(201);
    }
    // Populate with dummy ItemTypes
    for (int i = 100; i <= 200; ++i) {
        if (!g_items.items[i]) {
            g_items.items[i] = std::make_unique<ItemType>();
            g_items.items[i]->id = i;
            g_items.items[i]->clientID = i;
            // Note: sprite is nullptr, so actual drawing of sprites will be skipped,
            // but traversal logic will run.
        }
    }

    // Populate map
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            Tile* tile = editor.map.createTile(x, y, z);

            // Add ground
            // Just create simple items to populate lists
            tile->ground = std::make_unique<Item>(100); // Dummy ID

            // Add some items on top
            if ((x + y) % 5 == 0) {
                tile->addItem(std::make_unique<Item>(itemDist(rng)));
            }
        }
    }

    std::cout << "Map generation complete." << std::endl;

    // 2. Setup Rendering Components
    // We can use the real classes, but we need to be careful about OpenGL init.
    // We will NOT call SetupGL() on them.

    ItemDrawer itemDrawer;
    SpriteDrawer spriteDrawer;
    CreatureDrawer creatureDrawer;
    CreatureNameDrawer creatureNameDrawer;
    FloorDrawer floorDrawer;
    MarkerDrawer markerDrawer;
    TooltipDrawer tooltipDrawer;
    GridDrawer gridDrawer;

    TileRenderer tileRenderer(&itemDrawer, &spriteDrawer, &creatureDrawer, &creatureNameDrawer, &floorDrawer, &markerDrawer, &tooltipDrawer, &editor);
    MapLayerDrawer mapLayerDrawer(&tileRenderer, &gridDrawer, &editor);

    // Mock RenderView
    RenderView view;
    DrawingOptions options;
    LightBuffer lightBuffer;

    // Configure Viewport to cover the entire 2000x2000 map
    // Assuming 32x32 pixels per tile
    view.tile_size = 1; // Very small tiles
    view.zoom = 33.0f; // Zoom out significantly (Zoom Factor > 1.0 means Minified)

    // Logical viewport size in map coordinates
    view.start_x = 0;
    view.start_y = 0;
    view.end_x = width;
    view.end_y = height;
    view.start_z = z;
    view.end_z = z;
    view.floor = z;

    // Screen dimensions (arbitrary large number to ensure "visible" check passes if based on screen pixels)
    // view.screensize_x = width * 32 / zoom
    view.screensize_x = 1920;
    view.screensize_y = 1080;
    view.logical_width = width * 32.0f; // Mock logical width coverage
    view.logical_height = height * 32.0f;

    view.view_scroll_x = 0;
    view.view_scroll_y = 0;

    // Initialize SpriteBatch (Mock or Real but no GL)
    SpriteBatch spriteBatch;

    std::cout << "Starting Benchmark Loop (100 frames)..." << std::endl;

    RenderBenchmark::Get().Reset();

    int frames = 100;
    for (int i = 0; i < frames; ++i) {
        RenderBenchmark::Get().StartFrame();

        spriteBatch.begin(view.projectionMatrix);

        // Simulate drawing layer 7
        // We pass 'false' for live_client
        mapLayerDrawer.Draw(spriteBatch, z, false, view, options, lightBuffer);

        // We don't flush sprite batch to GPU, just measuring CPU submission/traversal
        // But in real code, we would end(). In headless, end() just clears pending.
        // We need an AtlasManager for end().
        // Mock AtlasManager? Or just let it leak/clear in next begin?
        // SpriteBatch::begin clears pending_sprites_.

        RenderBenchmark::Get().EndFrame();

        if (i % 10 == 0) std::cout << "." << std::flush;
    }
    std::cout << std::endl;

    std::cout << RenderBenchmark::Get().GetReport() << std::endl;

    return 0;
}
