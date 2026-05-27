#pragma once

#include "precompile.h"
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <pybind11/embed.h>
#include <Python.h>

class PyWorker {
private:
    std::unique_ptr<pybind11::scoped_interpreter> interpreter;
    std::unique_ptr<pybind11::gil_scoped_release> main_gil_release;

    std::thread worker_thread;
    std::string py_script_path;
    std::atomic<bool> active = false;

    void ThreadLoop() {
        std::cout << "[ThreadLoop] SECONDARY THREAD HAS EFFECTIVELY STARTED!\n";
        pybind11::gil_scoped_acquire acquire;

        try {
            std::cout << "[ThreadLoop] Attempting to evaluate file: " << py_script_path << "\n";

            pybind11::module_ sys = pybind11::module_::import("sys");
            // Get path to NumPy & OpenCV
            sys.attr("path").attr("append")("C:\\Program Files\\Python313\\Lib\\site-packages");
            // Add Python folder to find scripts
            sys.attr("path").attr("insert")(0, "./Python");

            auto main_module = pybind11::module_::import("__main__");
            auto global_dict = main_module.attr("__dict__");

            pybind11::eval_file(py_script_path, global_dict);
            std::cout << "[ThreadLoop] Script execution completed successfully.\n";
        }
        catch (const pybind11::error_already_set& e) {
            std::cerr << "\n[Python Execution Error]:\n" << e.what() << "\n";
        }
        active = false;
    }

public:
    PyWorker() = default;

    ~PyWorker() {
        Deactivate();
    }

    PyWorker(const PyWorker&) = delete;
    PyWorker& operator=(const PyWorker&) = delete;

    void Start(const std::string& scriptPath) {
        if (active) return;

#if defined(_WIN32)
        _wputenv_s(L"PYTHONHOME", L"C:\\Program Files\\Python313");
#endif

        py_script_path = scriptPath;
        active = true;

        try {
            // Init interpreter
            interpreter = std::make_unique<pybind11::scoped_interpreter>();
            std::cout << "[PyWorker] Python 3.13 subsystem initialized.\n";

            // Free GIL from the main thread
            main_gil_release = std::make_unique<pybind11::gil_scoped_release>();

            // Start second thread
            worker_thread = std::thread(&PyWorker::ThreadLoop, this);
            std::cout << "[PyWorker] std::thread instruction executed successfully.\n";
        }
        catch (const std::exception& e) {
            std::cerr << "[PyWorker Fatal] Exception in Start(): " << e.what() << "\n";
            active = false;
        }
    }

    void Deactivate() {
        if (!active) return;

        std::cout << "[PyWorker] Shutting down secondary thread...\n";
        if (worker_thread.joinable()) {
            worker_thread.join();
        }

        // Delete GIL release
        if (main_gil_release) {
            main_gil_release.reset();
        }

        if (interpreter) {
            interpreter.reset();
        }

        active = false;
        std::cout << "[PyWorker] Secondary thread deactivated\n";
    }
};
