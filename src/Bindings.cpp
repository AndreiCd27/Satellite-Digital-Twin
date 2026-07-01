
#include "Bindings.h"

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

    // GeoTIFF class structure
    pybind11::class_<GeoData>(m, "GeoData")
    .def(pybind11::init<float, float, float, float, float, int, int, std::string, std::string, std::string, float>(),
        pybind11::arg("lat0"), pybind11::arg("lon0"), pybind11::arg("lat1"), pybind11::arg("lon1"),
        pybind11::arg("res"), pybind11::arg("width"), pybind11::arg("height"),
        pybind11::arg("filename"), pybind11::arg("filepath"), pybind11::arg("CRS"),
        pybind11::arg("avg_elevation"));

    // allow to convert from pyton tuple or list into AVertex
    pybind11::implicitly_convertible<pybind11::tuple, AVertex>();
    pybind11::implicitly_convertible<pybind11::list, AVertex>();

    // allow to convert from pyton tuple or list into GeoData
    pybind11::implicitly_convertible<pybind11::tuple, GeoData>();
    pybind11::implicitly_convertible<pybind11::list, GeoData>();

    // Export GeoData to C++ Main Thread
    m.def("push_geodata", [](GeoData geodata) {
        auto cmd = std::make_unique<GeoDataCommand>(std::move(geodata));
        PyGeoDataLoad.Push(std::move(cmd));
        }, pybind11::arg("GeoData"));

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
