#include "rendering/core/game_sprite.h"
#include "game/outfit.h"
#include "app/application.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include <wx/app.h>
#include <wx/init.h>

class BenchmarkGameSprite : public GameSprite {
public:
    using GameSprite::getTemplateImage;
};

// Implement Application methods to satisfy linker
Application::~Application() {}
bool Application::OnInit() { return true; }
void Application::OnEventLoopEnter(wxEventLoopBase* loop) {}
void Application::MacOpenFiles(const wxArrayString& fileNames) {}
int Application::OnExit() { return 0; }
void Application::Unload() {}
void Application::FixVersionDiscrapencies() {}
bool Application::ParseCommandLineMap(wxString& fileName) { return false; }
void Application::OnFatalException() {}

// Implement wxGetApp
Application& wxGetApp() {
    return *static_cast<Application*>(wxApp::GetInstance());
}

int main(int argc, char** argv) {
    // Mock Application instance for wxGetApp calls (if any)
    // We don't call wxEntryStart so we don't need X11
    wxApp::SetInstance(new Application());

    BenchmarkGameSprite sprite;

    // Setup some dummy data for sprite
    sprite.width = 1;
    sprite.height = 1;
    sprite.numsprites = 1;

    // Generate random outfits
    std::vector<Outfit> outfits;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> colorDist(0, 132);

    for (int i = 0; i < 100; ++i) { // 100 different outfits
        Outfit o;
        o.lookHead = colorDist(rng);
        o.lookBody = colorDist(rng);
        o.lookLegs = colorDist(rng);
        o.lookFeet = colorDist(rng);
        outfits.push_back(o);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();

    int iterations = 1000000;
    for (int i = 0; i < iterations; ++i) {
        // Access outfits in a way that simulates usage
        const auto& outfit = outfits[i % outfits.size()];
        // access a few times per outfit to simulate multiple lookups (e.g. per frame or per layer)
        sprite.getTemplateImage(0, outfit);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "Time: " << diff.count() << " s" << std::endl;
    std::cout << "Time per op: " << (diff.count() / iterations) * 1e9 << " ns" << std::endl;

    // Cleanup
    delete wxApp::GetInstance();

    return 0;
}
