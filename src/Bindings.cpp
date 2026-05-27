
#include "Bindings.h"

std::atomic<bool> python_should_run{ true };

MutexQueue<RenderCommand> PyRenderLoad;
MutexQueue<TextureCommand> PyPixelLoad;

void CommandBuffer::ProcessPyRenderCommands(Scene* scene) {

    while (auto cmd = PyRenderLoad.TryPop()) {
        ;
        // PushGeometry sets UpdateBuffers = true automatically
        if (cmd->set_geom) {
            scene->SetGeometry(cmd->vertices, cmd->indices);
        }
        else {
            scene->PushGeometry(cmd->vertices, cmd->indices);
        }
    };
}
void CommandBuffer::ProcessPyPixelCommands() {
    // Extract all images from Python
    while (auto cmd = PyPixelLoad.TryPop()) {

        // Get OpenGL format based on image channels
        GLenum format = GL_RGB;
        if (cmd->channels == 1) format = GL_RED;
        else if (cmd->channels == 3) format = GL_BGR;  // OpenCV standard (BGR)
        else if (cmd->channels == 4) format = GL_BGRA; // OpenCV w Alpha

        glBindTexture(GL_TEXTURE_2D, cmd->textureID);
        // Send pixel data to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cmd->width, cmd->height, 0, format, GL_UNSIGNED_BYTE, cmd->pixels.data());
        // Filter params
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

PYBIND11_EMBEDDED_MODULE(py_engine3d, m) {
    m.doc() = "TinyCRender Python Worker API";


    m.def("should_run", []() {
        return python_should_run.load();
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
    m.def("push_texture_pixels", [](pybind11::buffer b, unsigned int textureSlotID) {
        // Get info about NumPy buffer
        pybind11::buffer_info info = b.request();

        if (info.format != pybind11::format_descriptor<unsigned char>::value) {
            throw std::runtime_error("Incompatible format! Expected uint8 (unsigned char) array");
        }

        // info.shape is [height, width, channels] for colored images
        // or [height, width] for grayscale
        int height = static_cast<int>(info.shape[0]);
        int width = static_cast<int>(info.shape[1]);
        int channels = (info.ndim == 3) ? static_cast<int>(info.shape[2]) : 1;

        // Total image bytes
        size_t total_bytes = info.size * info.itemsize;

        // Pointer to pixels in Python Memory
        unsigned char* ptr = static_cast<unsigned char*>(info.ptr);

        // Fast copy python data to C++ array
        auto cmd = std::make_unique<TextureCommand>();
        cmd->pixels.assign(ptr, ptr + total_bytes);
        cmd->width = width;
        cmd->height = height;
        cmd->channels = channels;
        cmd->textureID = textureSlotID;

        // Push command to mutex queue
        PyPixelLoad.Push(std::move(cmd));
        }, pybind11::arg("image_array"), pybind11::arg("texture_id")
    );
}
