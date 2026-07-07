#include "Engine3D.h"
#include <iostream>
#include <chrono>

#include "LightingModel.h"
#include "ShadowAnalyzer.h"
#include "PythonWorker.h"
#include "Bindings.h"
#include "Export.h"
#include "Select.h"
#include "GUI.h"

#include <cstdlib>

bool isCI = false;
auto startTime = std::chrono::steady_clock::now();

#define scene engine->getScene()

static void CI_GITHUB_CHECK() {
    if (const char* ciEnv = std::getenv("GITHUB_ACTIONS")) {
        if (std::string(ciEnv) == "true") {
            isCI = true;
            std::cout << "[CI] Github Actions Detected\n";
        }
    }
}

static bool CI_GITHUB_TIMEOUT() {
    if (isCI) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

        if (elapsed >= 60) {
            std::cout << "[CI] Game Loop Shutdown" << std::endl;
            return true;
        }
    }
    return false;
}

std::string GetDebugTexKey(int tt) {
    std::string key;
    if (tt == 0) key = "satellite";
    if (tt == 1) key = "buildings_mask";
    if (tt == 2) key = "shadow_mask";
    if (tt == 3) key = "shadow_mask_ref";
    if (tt == 4) key = "sim_shadow_mask";
    if (tt == 5) key = "diff_shadow_mask";
    if (tt == 6) key = "norm_heights";
    if (tt == 7) key = "u_heights";
    if (tt == 8) key = "Vis0";
    if (tt == 9) key = "Vis1";
    if (tt == 10) key = "Vis2";
    if (tt == 11) key = "Vis3";
    if (tt == 12) key = "osm_heights";

    if (tt == 7) { Engine3D::DEBUG_TEXTURE_SCALAR = 1.0f / 60.0f; }
    else { Engine3D::DEBUG_TEXTURE_SCALAR = 1.0f; }

    if (tt == 12 || tt ==13) {
        Engine3D::DEBUG_TEXTURE_CHANNELS = 1;
        Engine3D::DEBUG_TEXTURE_SCALAR = 1.0f / 1000.0f;
    }
    if (tt == 7) { Engine3D::DEBUG_TEXTURE_CHANNELS = 2; }
    else { Engine3D::DEBUG_TEXTURE_CHANNELS = 3; }

    return key;
}

static void DebugTex(int tt) {
    std::shared_ptr<Texture> stex = nullptr;
    auto key = GetDebugTexKey(tt);
    stex = CommandBuffer::GetTexSlot(key);
    if (stex != nullptr) {
        std::cout << "[TEXTURE_DEBUG] Showing Texture " << key << " # # # # # # # # # # # # # # # # # # # # # # # \n";
        Engine3D::DEBUG_TEXTURE = true;
        Engine3D::debug_texture_target = stex.get();
    }
}

void DebugAllTex(Engine3D* e) {
    for (int i = 0; i < 13; i++) {
        DebugTex(i);
        e->Render();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    Engine3D::DEBUG_TEXTURE = false;
}

static bool ReshadeAndUpdateScene(Engine3D* engine, SatelliteAnalyzer& SatelliteImageAnalyze, SHLM& shlm, bool geom) {

    auto terrain_tex = CommandBuffer::GetTexSlot("heightmap");
    auto bmask_tex = CommandBuffer::GetTexSlot("buildings_mask");

    if (terrain_tex == nullptr || bmask_tex == nullptr) return false;
    if (__ENV_RESHADE_REQUEST == false) return false;

    __PROCESS_PX_HALT_REQUEST.store(true);
    std::cout << "[RENDER] Reshade requested. Locking Python intake...\n";

    std::cout << "[RENDER] ---------------------------------------------------------------------\n";
    std::cout << "[RENDER] Analyzing satellite image...\n";
    std::cout << "[RENDER] ---------------------------------------------------------------------\n";
    glFinish();
    SatelliteAnalyzer::ResetGLContexts();
    SatelliteImageAnalyze.ResetAnalysisState();

    // Geometry will reset, textures will regenerate, wait untill solar output computation is finished
    // Now we stop processing pixels input from python
    __PROCESS_PX_HALT_REQUEST.store(true);
    SatelliteImageAnalyze.AnalyzeFromQueue();

    auto u_heights_tex = CommandBuffer::GetTexSlot("u_heights");
    auto satellite_tex = CommandBuffer::GetTexSlot("satellite");

    std::cout << "[RENDER] ---------------------------------------------------------------------\n";
    std::cout << "[RENDER] Relighting scene...\n";
    std::cout << "[RENDER] ---------------------------------------------------------------------\n";
    //SatelliteAnalyzer::ResetGLContexts();
    if (u_heights_tex) shlm.SetUnifiedHeightmap(u_heights_tex.get());
    if (satellite_tex) shlm.SetSatelliteTexture(satellite_tex.get());
    shlm.SH_Heightmap_Shading_Compute();
    //SatelliteAnalyzer::ResetGLContexts();
    std::cout << "[RENDER] ---------------------------------------------------------------------\n";
    std::cout << "[RENDER] Relighting complete!\n";
    std::cout << "[RENDER] ---------------------------------------------------------------------\n";
    glFinish();

    while (Scene::SCENE_GEOMETRY_UPDATE == false) {
        CommandBuffer::ProcessPyRenderCommands(engine->getScene());
    }

    Scene::SCENE_GEOMETRY_UPDATE = false;
    if (geom) engine->UpdateBuffers();

    if (geom) {
        shlm.ElevateVBO();

        engine->getScene()->ClearVBO();
        engine->getScene()->ClearEBO();
    }

    //DebugAllTex(engine);

    return true;
}

void DebugTMY(Engine3D* engine) {
    float dhi = Engine3D::tmy_data.dni[engine->render_t];
    float dni = Engine3D::tmy_data.dni[engine->render_t];
    float ghi = Engine3D::tmy_data.ghi[engine->render_t];
    std::cout << "DHI = " << dhi << " DNI = " << dni << " GHI = " << ghi << "\n";
}


int main() {

    CI_GITHUB_CHECK();

    Engine3D* engine = Engine3D::GetEngine3D();

    int success = engine->setupWindow(1200, 900, "window");
    if (!success) { std::cerr << "Error at setup \n"; Engine3D::EngineTerminate(); return -1; }


    PyWorker pythonWorker;
    pythonWorker.Start("Python/inference.py");

    // SH soft shadows

    float px2meters = 0.3f;
    float imgSize = 5000.0f * px2meters / 2.0f;

    SHLM shlm(engine, engine->getCFG(),
        AVector3(2048, 128, 2048), AVector3(-imgSize, -128.0f, -imgSize), AVector3(imgSize, 128.0f, imgSize));

    WorldBBox bbox{ AVector3(-imgSize, -128.0f, -imgSize), AVector3(imgSize, 128.0f, imgSize) };

    //////////////////////////////

    engine->SetupFull("static");

    engine->setBackground(0.0f, 0.0f, 0.0f, 1.0f);
    
    // PRE-GAME LOOP ---> ACTIVATE SHLM
    shlm.BindToEngine(45.0f, 0.01f, 5000.0f);

    SatelliteAnalyzer SatelliteImageAnalyze;

    bool init_global_sys = false;

    float azimuth = 0.5f, elevation = 0.9f;

    // IMGUI
    GUI_SERVICE guis;
    guis.LoadImGui(engine->GetWindow(),"#version 430");
    guis.TestImGui();
    guis.BindToEngine(engine);
    //

    GeoData GEO_DATA;
    GeoExporter GeoExporterService{ &GEO_DATA };

    // Selecter

    FBO_Sampler fbosampler; fbosampler.genFBO();
    TexturePxSelecter tex_px_selecter(&fbosampler);

    while (!engine->windowShouldClose()) {

        if (CI_GITHUB_TIMEOUT()) break;

        if (Camera::StopMotion == false && __PROCESS_PX_HALT_REQUEST == false) {
            
            CommandBuffer::ProcessPyRenderCommands(engine->getScene());
            CommandBuffer::ProcessPyPixelCommands();
            CommandBuffer::ProcessPyDataCommands();
            GeoData geodata_temp = CommandBuffer::ProcessPyGeoDataCommands();
            if (geodata_temp.GeoFile.filename != "N/A") GEO_DATA = geodata_temp;

            if (__SENT_PX_COMMAND == true) {
                std::cout << "\n[MAIN THEREAD PIPELINE] WAITING FOR PIXEL DATA ON MAIN THREAD...\n";
                while (__SENT_PX_COMMAND == true || !PyPixelLoad.Empty()) {
                    CommandBuffer::ProcessPyPixelCommands();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                std::cout << "final pixel process command\n";
                CommandBuffer::ProcessPyPixelCommands();
                glFinish();
            }
        }
        if (__ENV_RESHADE_REQUEST == true) {
            bool reshaded = ReshadeAndUpdateScene(engine, SatelliteImageAnalyze, shlm, true);
            if (reshaded) {
                std::cout << "\n[ RESHADE REQUEST FINALIZED ] -------"<<
                    "----------------- [ PRESS C TO CONTINUE ] -------\n";
                __ENV_RESHADE_REQUEST.store(false);
                init_global_sys = true;
            }
        }
        engine->initGameFrame();
        // GAME-LOOP CODE HERE
        


        // ENGINE RENDER FUNCTION
        engine->Render();

        
        if (__PROCESS_PX_HALT_REQUEST == true && 
            glfwGetKey(engine->GetWindow()->getWindow(), GLFW_KEY_C) == GLFW_PRESS) {

            //SatelliteImageAnalyze.ResetTextures();
            //shlm.ResetPipelineForNextImage();
            //glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

            std::cout << "[MAIN_THREAD] Continue...\n";
            __PROCESS_PX_HALT_REQUEST.store(false);
            init_global_sys = false;
        }
        if (__PROCESS_PX_HALT_REQUEST == true &&  
            glfwGetKey(engine->GetWindow()->getWindow(), GLFW_KEY_G) == GLFW_PRESS) {
            // Export to GeoJSON
            GeoExporterService.ExportVBOtoGeoJSON(engine, GEO_DATA.GeoFile.filename + ".geojson");
        }
        if (__PROCESS_PX_HALT_REQUEST == true &&  
            glfwGetKey(engine->GetWindow()->getWindow(), GLFW_KEY_V) == GLFW_PRESS) {
            // Debug All Textures
            DebugAllTex(engine);
        }
        if (__PROCESS_PX_HALT_REQUEST == true &&  
            glfwGetMouseButton(engine->GetWindow()->getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            // Selecter for heightmap (WORK IN PROGRESS)
            fbosampler.attachTexture(CommandBuffer::GetTexSlot("u_heights").get());
            double mouseX, mouseY;
            Window* _window = engine->GetWindow();
            Camera& _cam = engine->getCamera(false);
            glfwGetCursorPos(_window->getWindow(), &mouseX, &mouseY);
            int pixelX = static_cast<int>(mouseX);
            int pixelY = static_cast<int>(mouseY);
            float pixelData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            tex_px_selecter.Select(_window, &_cam, &bbox, pixelX, pixelY, true, pixelData);
            //std::cout <<"[SELECTER] HEIGHT DATA: " << pixelData[0] << " " << pixelData[1] << "\n";
        }
        
    }

    python_should_run.store(false);
    pythonWorker.Deactivate();

    guis.ShutdownImGui();
    engine->EngineTerminate();

    return 0;
}