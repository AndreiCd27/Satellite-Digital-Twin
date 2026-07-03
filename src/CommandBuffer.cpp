
#include "CommandBuffer.h"

// Python Needed Symbols
std::atomic<bool> python_should_run{ true };
std::atomic<bool> __ENV_RESHADE_REQUEST{ true };
std::atomic<bool> __PROCESS_PX_HALT_REQUEST{ false };
std::atomic<bool> __SENT_PX_COMMAND{ false };

MutexQueue<RenderCommand> PyRenderLoad;
MutexQueue<TextureCommand> PyPixelLoad;
MutexQueue<DataCommand> PyDataLoad;
MutexQueue<GeoDataCommand> PyGeoDataLoad;

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

        std::cout << "[RENDER] PROCESSED DATA STORE COMMAND FROM PYTHON\n"; /*
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
        }*/
    };
}
GeoData CommandBuffer::ProcessPyGeoDataCommands() {

    while (auto cmd = PyGeoDataLoad.TryPop()) {

        std::cout << "[RENDER] PROCESSED GEODATA STORE COMMAND FROM PYTHON\n";

        return cmd->geodata;
    };
    return GeoData();
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

