#include "Engine3D.h"
#include <iostream>
#include <chrono>

#include "LightingModel.h"
#include "Image.h"
#include "ShadowAnalyzer.h"
#include "PythonWorker.h"

#define scene engine->getScene()

int main() {

    Engine3D* engine = Engine3D::GetEngine3D();

    std::cout << "Start main\n";

    int success = engine->setupWindow(1200, 900, "window");
    if (!success) { std::cerr << "Error at setup \n"; Engine3D::EngineTerminate(); return -1; }


    PyWorker pythonWorker;
    pythonWorker.Start("Python/test.py");

    // SH soft shadows

    SHLM shlm(engine, engine->getCFG(), 
        AVector3(1024,64,1024), AVector3(-1024.0f, -16.0f, -1024.0f), AVector3(1024.0f, 48.0f, 1024.0f));

    AVertex v0(0.0f, 0.0f, 0.0f, 255, 0, 0, 255);
    AVertex v1(100.0f, 0.0f, 0.0f, 0, 255, 0, 255);
    AVertex v2(100.0f, 0.0f, 100.0f, 255, 0, 255, 255);
    AVertex v3(0.0f, 0.0f, 100.0f, 0, 255, 0, 255);
    std::vector<AVertex> vert = {
        v0, v1, v2, v3
    };
    std::vector<GLuint> ind = {
        0,2,1, 0,3,2
    };
    scene->PushGeometry(vert, ind);

    
    //////////////////////////////
    engine->DEBUG_ArrayOrganizers();
    
    engine->SetupFull("static");

    engine->setBackground(0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << "Printing Instance VBO dimensions: \n";
    
    float t = 8.0f;

    // PRE-GAME LOOP ---> ACTIVATE SHLM
    shlm.BindToEngine(45.0f, 0.01f, 1000.0f);

    shlm.Load_Cubemap_GPU_ComputeShader_Extended();

    auto ims = ImageService::GetService();

    auto IMG_raw_pixels = ims->ExtractPixelData(IMG_TYPE::TIFF, "resources/innsbruck10.tif");
    auto IMG_raw_dim = ims->GetImageDimensions(IMG_TYPE::TIFF, "resources/innsbruck10.tif");
    auto IMG_mask_pixels = ims->ExtractPixelData(IMG_TYPE::PNG, "resources/innsbruck10_mask.png");
    auto IMG_mask_dim = ims->GetImageDimensions(IMG_TYPE::PNG, "resources/innsbruck10_mask.png");
    std::cout << "IMG RAW DIM: " << IMG_raw_dim.first << "x" << IMG_raw_dim.second <<" px: "<<IMG_raw_pixels.size() << "\n";
    std::cout << "IMG MASK DIM: " << IMG_mask_dim.first << "x" << IMG_mask_dim.second << " px: " << IMG_mask_pixels.size() << "\n";

    ShadowAnalyzer ShdwAnalyze;
    ShdwAnalyze.PushInQueue(
        IMG_raw_pixels, IMG_raw_dim.first, IMG_raw_dim.second,
        IMG_mask_pixels, IMG_mask_dim.first, IMG_mask_dim.second
    );
    AnalysisResult a;
    ShdwAnalyze.AnalyzeFromQueue(a);
    ShdwAnalyze.SaveShadowMaskToDisk(*a.shadow_mask, "resources/shadow_mask_buildings.png");
    ShdwAnalyze.SaveHeightmapToDisk(*a.heightmap_buildings, "resources/heightmap_buildings.png", (float)__PI * 35.0f / 180.0f);

    while (!engine->windowShouldClose()) {

        CommandBuffer::ProcessPyRenderCommands(engine->getScene());

        // GAME-LOOP CODE HERE
        engine->Render( 
            t / 64.0f * 12.0f  + 8.0f
        );

        t += 0.01f;

        //std::cout << t / 64.0f * 24.0f << " TIME \n";
        if (t > 64.0f) {
            t = 0.0f;
        }
    }

    python_should_run.store(false);
    pythonWorker.Deactivate();

    engine->EngineTerminate();

    return 0;
}