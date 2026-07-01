#pragma once

#include <atomic>

#include "Texture.h"
#include "Engine3D.h"
#include "Export.h"
#include "Mutex.h"

struct RenderCommand {
    std::vector<AVertex> vertices;
    std::vector<GLuint> indices;
    bool set_geom = false;
public:
    RenderCommand(const std::vector<AVertex>& verts, const std::vector<GLuint>& inds, bool geom) :
        vertices(verts), indices(inds), set_geom(geom) {
    }
    RenderCommand() = default;
};

struct DataCommand {
    std::vector<float> tmy;
    std::vector<float> sun_dirs;
    std::string dataType;
public:
    DataCommand(const std::vector<float>& _tmy, const std::vector<float>& _sun_dirs, const std::string& _dataType) :
        tmy(_tmy), sun_dirs(sun_dirs), dataType(_dataType) {
    }
    DataCommand() = default;
};

struct TextureCommand {
    std::vector<unsigned char> pixels;
    int width;
    int height;
    int channels;
    std::string textureKey;; //glTexture Key
    bool is_float;
};

struct GeoDataCommand {
    GeoData geodata;
public:
    GeoDataCommand(const GeoData& _geodata) : geodata(_geodata) {}
    GeoDataCommand() = default;
};

extern std::atomic<bool> __ENV_RESHADE_REQUEST;

extern std::atomic<bool> __SENT_PX_COMMAND;

extern std::atomic<bool> __PROCESS_PX_HALT_REQUEST;

extern std::atomic<bool> python_should_run;

extern MutexQueue<RenderCommand> PyRenderLoad;
extern MutexQueue<TextureCommand> PyPixelLoad;
extern MutexQueue<DataCommand> PyDataLoad;
extern MutexQueue<GeoDataCommand> PyGeoDataLoad;

class CommandBuffer {
public:
    //MutexQueue<RenderCommand> PythonToMainQueue;

    static std::unordered_map<std::string, std::shared_ptr<Texture>> TextureSlots;

    static std::shared_ptr<Texture> InitTexSlot(const std::string& TexKey);
    static std::shared_ptr<Texture> GetTexSlot(const std::string& TexKey);

    // PYTHON SYMBOLS

    static void ProcessPyRenderCommands(Scene* scene);
    static void ProcessPyPixelCommands();
    static void ProcessPyDataCommands();
    static GeoData ProcessPyGeoDataCommands();
};