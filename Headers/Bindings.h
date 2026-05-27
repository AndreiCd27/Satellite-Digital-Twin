#pragma once

#include "Engine3D.h"
#include "Mutex.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>

#include <atomic>

extern std::atomic<bool> python_should_run;

struct RenderCommand {
    std::vector<AVertex> vertices;
    std::vector<GLuint> indices;
    bool set_geom = false;
};

struct TextureCommand {
    std::vector<unsigned char> pixels;
    int width;
    int height;
    int channels;
    unsigned int textureID; //glTexture
};


extern MutexQueue<RenderCommand> PyRenderLoad;
extern MutexQueue<TextureCommand> PyPixelLoad;

class CommandBuffer {
public:
    //MutexQueue<RenderCommand> PythonToMainQueue;

    static void ProcessPyRenderCommands(Scene* scene);
    static void ProcessPyPixelCommands();

};