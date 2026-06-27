import os
import torch

def setup_hardware_environment(model_path_ov):
    """
    Optimizes environment variables for PyTorch / OpenVINO and detects the best hardware engine
    """
    # Optimize PyTorch thread behavior and memory allocation for fallback/CPU modes
    os.environ["MKL_NUM_THREADS"] = "4"
    os.environ["OMP_NUM_THREADS"] = "4"
    os.environ["PYTORCH_NO_CUDA_MEMORY_CACHING"] = "1"

    # 2. OpenVINO Intel GPU / CPU Detection
    if os.path.exists(model_path_ov):
        try:
            import openvino as ov
            core = ov.Core()
            available_devices = core.available_devices
            
            if "GPU" in available_devices:
                print("[CONFIG] Optimal hardware found: Intel GPU via OpenVINO")
                return "openvino", "GPU"
            else:
                print("[CONFIG] OpenVINO XML found, but no Intel GPU available. Using OpenVINO CPU")
                return "openvino", "CPU"
        except Exception as e:
            print(f"[CONFIG] OpenVINO error during search: {e}")

    # PyTorch Device Search (NVIDIA CUDA, Intel XPU, CPU)
    if torch.cuda.is_available():
        # optimization flags for high-performance NVIDIA cards
        os.environ["TORCH_CUDNN_V8_API_ENABLED"] = "1"
        torch.backends.cudnn.benchmark = True
        print("[CONFIG] Hardware found: NVIDIA CUDA via PyTorch")
        return "pytorch", "cuda"
        
    elif hasattr(torch, "xpu") and torch.xpu.is_available():
        print("[CONFIG] Hardware found: Intel GPU via PyTorch XPU")
        return "pytorch", "xpu"
        
    else:
        print("[CONFIG] No acceleration hardware found, using CPU via PyTorch")
        return "pytorch", "cpu"
