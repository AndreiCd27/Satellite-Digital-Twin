
#include "GUI.h"

void GUI_SERVICE::LoadImGui(Window* win, const char* current_gl_version) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO(); (void)IO;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win->getWindow(), true);
    ImGui_ImplOpenGL3_Init(current_gl_version);

}

void GUI_SERVICE::ShutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GUI_SERVICE::BindToEngine(Engine3D* engine) {

    std::function<void(int)> ImGuiRender = [engine](int ctrl_var) {

        static float sunPitch = 45.0f;
        static float sunYaw = 90.0f;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui Window
        ImGui::Begin("Controls");
        ImGui::Text("Simulation Controls:");
        ImGui::SliderFloat("Sun Elevation", &sunPitch, 0.0f, 90.0f);
        ImGui::SliderFloat("Sun Azimuth", &sunYaw, 0.0f, 360.0f);

        ImGui::Separator();

        // Sliders for lighting parameters
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Lighting Menu:");

        auto& lparams = SHLM::LightParams;
        ImGui::SliderFloat("Direct Shadow Strength", &lparams.DirectShadowStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Shadow Contrast", &lparams.AmbientShadowContrast, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Shadow Intensity", &lparams.AmbientShadowIntensity, 0.0f, 1.0f);
        ImGui::SliderFloat("Direct Light Intensity", &lparams.DirectLightIntensity, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Light Intensity", &lparams.AmbientLightIntensity, 0.0f, 1.0f);

        ImGui::Separator();

        // Display system status or instructions
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Controls Menu:");
        ImGui::BulletText("Press [C] to segment next batch image.");
        ImGui::BulletText("Press [G] to export current town to GeoJSON.");

        ImGui::End();
        // ImGui Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        float azimuth = sunYaw / 180.0f * 3.1415926f;
        float elevation = sunPitch / 180.0f * 3.1415926f;
        engine->setSunAzimuthAndElevation(azimuth, elevation);
    };

    engine->getCFG()->AddAction(GUI_INSTANCES_STAGE, ImGuiRender, 0);
}