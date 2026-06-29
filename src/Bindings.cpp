
#include "Bindings.h"
#include "Engine3D.h"

std::atomic<bool> python_should_run{ true };
std::atomic<bool> __ENV_RESHADE_REQUEST{ true };
std::atomic<bool> __PROCESS_PX_HALT_REQUEST{ false };
std::atomic<bool> __SENT_PX_COMMAND{ false };

MutexQueue<RenderCommand> PyRenderLoad;
MutexQueue<TextureCommand> PyPixelLoad;
MutexQueue<DataCommand> PyDataLoad;

std::unordered_map<std::string, std::shared_ptr<Texture>> CommandBuffer::TextureSlots;

std::shared_ptr<Texture> CommandBuffer::InitTexSlot(const std::string& TexKey) {
    
    auto tex = std::make_shared<Texture>();

    TextureSlots.insert({ TexKey, tex });
    std::cout << "[COMMAND_BUFFER] Generated texture with key: " << TexKey << "\n";
    return tex;
}
std::shared_ptr<Texture> CommandBuffer::GetTexSlot(const std::string& TexKey) {
    if (TextureSlots.find(TexKey) == TextureSlots.end()) {
        //std::cout << "[COMMAND_BUFFER] ERROR: Texture with key: " << TexKey << " not found!\n";
        return nullptr;
    }
    return TextureSlots[TexKey];
}

void CommandBuffer::ProcessPyRenderCommands(Scene* scene) {

    while (auto cmd = PyRenderLoad.TryPop()) {

        std::cout << "[RENDER] PROCESSED RENDER GEOMETRY COMMAND FROM PYTHON\n";
        // PushGeometry sets UpdateBuffers = true automatically
        if (cmd->set_geom) {
            scene->SetGeometry(cmd->vertices, cmd->indices);
        }
        else {
            scene->PushGeometry(cmd->vertices, cmd->indices);
        }
    };
}
void CommandBuffer::ProcessPyDataCommands() {

    while (auto cmd = PyDataLoad.TryPop()) {

        std::cout << "[RENDER] PROCESSED DATA STORE COMMAND FROM PYTHON\n";
        // PushGeometry sets UpdateBuffers = true automatically
        if (cmd->dataType == "TMY") {

            int w = 365 * 24; int channels = 4;

            // Parse all 8760 hours in a year
            for (int hour = 0; hour < w; hour++) {
                int idx = hour * channels;

                auto& tmyData = cmd->tmy;
                auto& sunData = cmd->sun_dirs;

                // Extract TMY Weather Metrics
                float ghi = tmyData[idx + 0];
                float dni = tmyData[idx + 1];
                float dhi = tmyData[idx + 2];
                float temp = tmyData[idx + 3];

                // Extract Sun Vector Information
                float sx = sunData[idx + 0];
                float sy = sunData[idx + 1];
                float sz = sunData[idx + 2];
                float elev = sunData[idx + 3];

                if (sx == 0.0f && sy == 0.0f && sz == 0.0f) {
                    continue; // Skip invalid data points
                }
                if (dni < 0.0f || dhi < 0.0f) {
                    continue; // Skip invalid data points
                }

                Engine3D::tmy_data.ghi.push_back(ghi);
                Engine3D::tmy_data.dni.push_back(dni);
                Engine3D::tmy_data.dhi.push_back(dhi);
                Engine3D::tmy_data.temp.push_back(temp);

                Engine3D::tmy_data.sun_x.push_back(sx);
                Engine3D::tmy_data.sun_y.push_back(sy);
                Engine3D::tmy_data.sun_z.push_back(sz);
            }
        }
    };
}
void CommandBuffer::ProcessPyPixelCommands() {

    while (auto cmd = PyPixelLoad.TryPop()) {

        std::cout << "\n[RENDER] PROCESSED PIXELS TO TEXTURE COMMAND FROM PYTHON\n";

        if (TextureSlots.find(cmd->textureKey) == TextureSlots.end()) {
            auto t = std::make_shared<Texture>();
            TextureSlots[cmd->textureKey] = t;
            t->GenTex2D();
            std::cout << "[COMMAND_BUFFER] Generated texture ID: " << t->GetTexID() << " for key: " << cmd->textureKey << "\n";
        }

        auto& targetTex = TextureSlots[cmd->textureKey];

        glBindTexture(GL_TEXTURE_2D, targetTex->GetTexID());

        if (cmd->is_float) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            targetTex->CreateTex2D(cmd->width, cmd->height, GL_R32F, GL_RED, GL_FLOAT, cmd->pixels.data());
            std::cout << "[TEXTURE FROM PYTHON] FLOAT TEXTURE --------------------\n";
        }
        else {
            GLenum format = GL_RED;
            GLenum internalFormat = GL_R8;

            if (cmd->channels == 1) {
                format = GL_RED;
                internalFormat = GL_R8;
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
            }
            else if (cmd->channels == 3) {
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                format = GL_RGB;
                internalFormat = GL_RGB8;
            }
            else if (cmd->channels == 4) {
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                format = GL_RGBA;
                internalFormat = GL_RGBA8;
            }

            targetTex->CreateTex2D(cmd->width, cmd->height, internalFormat, format, GL_UNSIGNED_BYTE, cmd->pixels.data());


            std::cout << "[DEBUG TEXTURE] IMAGE DIMENSIONS: " << cmd->width << "x" << cmd->height
                << " | InternalFormat: " << internalFormat << " | Format: "
                << format << " | Channels: " << cmd->channels << "\n";
        }

        targetTex->MinMagFilter(GL_LINEAR, GL_LINEAR);
        targetTex->WrapFilter(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        __SENT_PX_COMMAND.store(false);
    }
}


PYBIND11_EMBEDDED_MODULE(py_engine3d, m) {
    m.doc() = "TinyCRender Python Worker API";

    m.def("sent_px_cmd", []() {
        __SENT_PX_COMMAND.store(true);
    });
    m.def("should_run", []() {
        return python_should_run.load();
    });
    m.def("should_halt", []() {
        return __PROCESS_PX_HALT_REQUEST.load();
    });
    m.def("reshade", []() {
        __ENV_RESHADE_REQUEST.store(true);
    });

    // AVertex class structure
    pybind11::class_<AVertex>(m, "AVertex")
    .def(pybind11::init<float, float, float, uint8_t, uint8_t, uint8_t, uint8_t, uint32_t>(),
        pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("z"), 
        pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("a"), pybind11::arg("id"));

    // allow to convert from pyton tuple or list into AVertex
    pybind11::implicitly_convertible<pybind11::tuple, AVertex>();
    pybind11::implicitly_convertible<pybind11::list, AVertex>();

    // Only push to mutex thread function is exposed to Python
    m.def("push_geometry", [](std::vector<AVertex> verts, std::vector<GLuint> inds) {
        auto cmd = std::make_unique<RenderCommand>( std::move(verts), std::move(inds), false);
        PyRenderLoad.Push(std::move(cmd));
        }, pybind11::arg("vertices"), pybind11::arg("indices")
    );
    m.def("set_geometry", [](std::vector<AVertex> verts, std::vector<GLuint> inds) {
        auto cmd = std::make_unique<RenderCommand>(std::move(verts), std::move(inds), true);
        PyRenderLoad.Push(std::move(cmd));
        }, pybind11::arg("vertices"), pybind11::arg("indices")
    );
    m.def("push_tmy_data", [](std::vector<float> tmy, std::vector<float> sun_dirs) {
        auto cmd = std::make_unique<DataCommand>(std::move(tmy), std::move(sun_dirs), "TMY");
        PyDataLoad.Push(std::move(cmd));
        }, pybind11::arg("tmy_f4"), pybind11::arg("sun_dirs_f4")
    );
    m.def("push_texture_pixels", [](pybind11::buffer b, std::string textureKey) {
        // Get info about NumPy buffer
        pybind11::buffer_info info = b.request();

        bool is_float = false;
        if (info.format == pybind11::format_descriptor<float>::value) {
            is_float = true;
        }
        else if (info.format != pybind11::format_descriptor<unsigned char>::value) {
            throw std::runtime_error("Incompatible format! Expected float32 or uint8 array.");
        }

        int height = static_cast<int>(info.shape[0]);
        int width = static_cast<int>(info.shape[1]);
        int channels = (info.ndim == 3) ? static_cast<int>(info.shape[2]) : 1;

        size_t total_bytes = info.size * info.itemsize;
        unsigned char* ptr = reinterpret_cast<unsigned char*>(info.ptr);

        auto cmd = std::make_unique<TextureCommand>();

        cmd->pixels.resize(total_bytes);
        std::memcpy(cmd->pixels.data(), ptr, total_bytes);

        cmd->width = width;
        cmd->height = height;
        cmd->channels = channels;
        cmd->textureKey = textureKey; // Save Texture Key
        cmd->is_float = is_float;

        // Push command to mutex queue
        PyPixelLoad.Push(std::move(cmd));
        }, pybind11::arg("image_array"), pybind11::arg("texture_key")
    );
}
