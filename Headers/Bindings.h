#pragma once

#include "Scene.h"
#include "Mutex.h"
#include "Texture.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>

#include <atomic>


extern std::atomic<bool> __ENV_RESHADE_REQUEST;

extern std::atomic<bool> __PROCESS_PX_HALT_REQUEST;

extern std::atomic<bool> python_should_run;

struct RenderCommand {
    std::vector<AVertex> vertices;
    std::vector<GLuint> indices;
    bool set_geom = false;
};

struct DataCommand {
    std::vector<float> tmy;
    std::vector<float> sun_dirs;
    std::string dataType;
};

struct TextureCommand {
    std::vector<unsigned char> pixels;
    int width;
    int height;
    int channels;
    std::string textureKey;; //glTexture Key
    bool is_float;
};


extern MutexQueue<RenderCommand> PyRenderLoad;
extern MutexQueue<TextureCommand> PyPixelLoad;
extern MutexQueue<DataCommand> PyDataLoad;

class CommandBuffer {
public:
    //MutexQueue<RenderCommand> PythonToMainQueue;

    static std::unordered_map<std::string, std::shared_ptr<Texture>> TextureSlots;

    static void ProcessPyRenderCommands(Scene* scene);
    static void ProcessPyPixelCommands();
    static void ProcessPyDataCommands();
    static std::shared_ptr<Texture> InitTexSlot(const std::string& TexKey);
    static std::shared_ptr<Texture> GetTexSlot(const std::string& TexKey);
};