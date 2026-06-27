
# /////////////////////////////////////////////////////////////////////////////////////////

# RUN THIS SCRIPT TO GENERATE OPENVINO .XML FILE FOR YOUR INTEL GPU

# /////////////////////////////////////////////////////////////////////////////////////////

import torch
import openvino as ov
import segmentation_models_pytorch as smp

# Init PyTorch model with our weights
model = smp.Unet(encoder_name="resnet34", encoder_weights=None, in_channels=3, classes=1)
model.load_state_dict(torch.load("building_segmentation/weights/building_unet_model_2.pth", map_location="cpu"))
model.eval()

# Test tensor (512x512px)
dummy_input = torch.randn(1, 3, 512, 512)

# Convert model into OpenVINO IR
ov_model = ov.convert_model(model, example_input=dummy_input)

# Save the model
ov.save_model(ov_model, "building_segmentation/openvino_intelgpu/building_unet_openvino.xml")
print("The model was successfully converted to OpenVINO!")
