#include "Engine3D.h"
#include <iostream>
#include <chrono>

#include "LightingModel.h"
#include "Image.h"
#include "ShadowAnalyzer.h"
#include "PythonWorker.h"
#include "Bindings.h"
#include "Export.h"
#include "GUI.h"

#define scene engine->getScene()

void DebugTextureR32F(Texture* t) {
    int w = t->GetWidth();
    int h = t->GetHeight();

    std::cout << "Debug texture =======================================================================\n";

    std::vector<float> cpuPixels(w * h, 0.0f);

    glBindTexture(GL_TEXTURE_2D, t->GetTexID());
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, cpuPixels.data());

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "[GPU ERROR] glGetTexImage failed with error code: " << err << "\n";
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    float avgH = 0.0f;
    float maxH = 0.0f;
    int bcnt = 0;

    for (int i = 0; i < w * h; i++) {
        if (cpuPixels[i] > 0.0f) {
            avgH += cpuPixels[i];
            bcnt++;
        }
        if (cpuPixels[i] > maxH) {
            maxH = cpuPixels[i];
        }
    }

    std::cout << "--------------------------------------------------------------\n";
    std::cout << "[GPU TEXTURE CHECK] Non-Zero PX Count: " << bcnt << " / " << (w * h) << "\n";
    std::cout << "[GPU TEXTURE CHECK] Max PX value: " << maxH << "\n";
    std::cout << "[GPU TEXTURE CHECK] Average PX value: " << avgH / (float(bcnt) + 0.000001f) << "\n";
    std::cout << "--------------------------------------------------------------\n";
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
    if (tt == 12) key = "sum_irradiance_out";

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

void DebugTex(int tt) {
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

void DebugIrr(Engine3D* engine) {
    static int debugStage = 0;
    static double debugTimer = 0.0;


    if (debugStage == 0) {
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
        DebugTex(12);
        engine->Render();
        debugStage = 1;
        debugTimer = glfwGetTime();
    }

    double currentTime = glfwGetTime();

    if (debugStage == 1 && (currentTime - debugTimer >= 3.0)) {
        DebugTex(13);
        engine->Render();
        debugStage = 2;
        debugTimer = currentTime;
    }

    if (debugStage == 2 && (currentTime - debugTimer >= 3.0)) {
        debugStage = 0;
        __PROCESS_PX_HALT_REQUEST.store(false);
    }
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

    //////////////////////////////

    engine->SetupFull("static");

    engine->setBackground(0.0f, 0.0f, 0.0f, 1.0f);
    
    // PRE-GAME LOOP ---> ACTIVATE SHLM
    shlm.BindToEngine(45.0f, 0.01f, 5000.0f);

    SatelliteAnalyzer SatelliteImageAnalyze;
    ImageService* ims = ImageService::GetService();

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

    while (!engine->windowShouldClose()) {

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
        
    }

    python_should_run.store(false);
    pythonWorker.Deactivate();

    guis.ShutdownImGui();
    engine->EngineTerminate();

    return 0;
}