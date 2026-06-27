#pragma once

#include "Window.h"
#include "LightingModel.h"
#include "Engine3D.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

class GUI_SERVICE {

public:

    void LoadImGui(Window* win, const char* current_gl_version);

    void ShutdownImGui();

    void TestImGui() {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui Window
        ImGui::Begin("imgui_window");
        ImGui::Text("Hwllo world!");
        ImGui::End();

    }

    void BindToEngine(Engine3D* engine);
};