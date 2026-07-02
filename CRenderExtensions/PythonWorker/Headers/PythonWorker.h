#pragma once

#include "precompile.h"
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <pybind11/embed.h>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

class PyWorker {
private:
    std::unique_ptr<pybind11::scoped_interpreter> interpreter;
    std::unique_ptr<pybind11::gil_scoped_release> main_gil_release;

    std::thread worker_thread;
    std::string py_script_path;
    std::atomic<bool> active = false;

    // All execution logic runs isolated inside this background thread
    void ThreadLoop() {
        std::cout << "[ThreadLoop] SECONDARY THREAD HAS EFFECTIVELY STARTED!\n";

        // Acquire the GIL exclusively on this background thread to run Python safely
        pybind11::gil_scoped_acquire acquire;

        try {
            {
                namespace fs = std::filesystem;

                fs::path exe_path;
#ifdef _WIN32
            char buffer[MAX_PATH];
            GetModuleFileNameA(NULL, buffer, MAX_PATH);
            exe_path = fs::path(buffer);
#elif __linux__
            exe_path = fs::read_symlink("/proc/self/exe");
#else
            exe_path = fs::current_path();
#endif

                fs::path base_dir = exe_path.parent_path();
                fs::path absolute_path = fs::absolute(base_dir / py_script_path);

                if (!fs::exists(absolute_path)) {
                    absolute_path = fs::absolute(py_script_path);
                }

                std::string script_dir = absolute_path.parent_path().generic_string();
                std::string build_dir = absolute_path.parent_path().parent_path().generic_string();

                std::cout << "[ThreadLoop] Attempting to evaluate file: " << absolute_path.generic_string() << "\n";

                // Get access from Python for this configuration script
                pybind11::module_ sys = pybind11::module_::import("sys");
                sys.attr("path").attr("insert")(0, pybind11::str(script_dir));
                sys.attr("path").attr("insert")(0, pybind11::str(build_dir));
                // Import the path_manage python script
                pybind11::module_ path_manage = pybind11::module_::import("path_manage");
                // Compute configured paths via this script
                path_manage.attr("configure_paths")(pybind11::str(script_dir));

                // Run the second thread (the python worker)
                auto main_module = pybind11::module_::import("__main__");
                auto global_dict = main_module.attr("__dict__");

                pybind11::eval_file(absolute_path.generic_string(), global_dict);
                std::cout << "[ThreadLoop] Script execution completed successfully.\n";
            }
        }
        catch (const pybind11::error_already_set& e) {
            std::cerr << "\n[Python Execution Error]:\n" << e.what() << "\n";
        }
        catch (const std::exception& e) {
            std::cerr << "\n[C++ Exception in ThreadLoop]:\n" << e.what() << "\n";
        }

        active = false;
    }

public:
    PyWorker() = default;

    ~PyWorker() {
        Deactivate();
    }

    // Prevent copies
    PyWorker(const PyWorker&) = delete;
    PyWorker& operator=(const PyWorker&) = delete;

    void Start(const std::string& scriptPath) {
        if (active) return;

        py_script_path = scriptPath;
        active = true;

#ifdef _WIN32
        std::cout << "[DEBUG] Specific Windows configurations for DLL not found errors\n";

        SetDllDirectoryA("C:\\Program Files\\Python313");
        Py_SetPythonHome(L"C:\\Program Files\\Python313");
#endif

        try {
            // Initialize the embedding interpreter on the main C++ render thread
            interpreter = std::make_unique<pybind11::scoped_interpreter>();
            std::cout << "[PyWorker] Python 3.13 subsystem initialized.\n";

            // Release the GIL from the main C++ thread
            // This prevents OpenGL from freezing or stalling
            main_gil_release = std::make_unique<pybind11::gil_scoped_release>();

            // 3. Spawn the worker thread which safely re-acquires the GIL as needed
            worker_thread = std::thread(&PyWorker::ThreadLoop, this);
            std::cout << "[PyWorker] std::thread instruction executed successfully.\n";
        }
        catch (const std::exception& e) {
            std::cerr << "[PyWorker Fatal] Exception in Start(): " << e.what() << "\n";
            // Clean up and restore safe C++ state if setup fails
            if (main_gil_release) main_gil_release.reset();
            if (interpreter) interpreter.reset();
            active = false;
        }
    }

    void Deactivate() {
        if (worker_thread.joinable()) {
            std::cout << "[PyWorker] Closing Python Worker Thread...\n";

            int timeout_counter = 0;
            while (active && timeout_counter < 8) {
                std::this_thread::sleep_for(std::chrono::milliseconds(125));
                timeout_counter++;
            }

            if (active) {
                std::cout << "[PyWorker Warning] Force Shutdown\n";
                worker_thread.detach(); // Detatch Python Thread
            }
            else {
                worker_thread.join(); // Clean shutdown
                std::cout << "[PyWorker] Python Thread Closed Successfully\n";
            }
        }

        // Destroy GIL components and Interpreter
        if (!active) {
            if (main_gil_release) main_gil_release.reset();
            if (interpreter) interpreter.reset();
        }

        active = false;
    }

};
