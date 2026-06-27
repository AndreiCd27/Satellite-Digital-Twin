

# DATASET //////////////////////////////////////////////////////////////

# USING THE INRIA AERIAL IMAGE LABELING DATASET FOR IMAGE SEGMENTATION:
# https://www.kaggle.com/datasets/sagar100rathod/inria-aerial-image-labeling-dataset/data

# ALL CREDITS TO INRIA AERIAL IMAGE LABELING DATASET DEVELOPED BY SAGAR RATHOD:
# https://www.kaggle.com/sagar100rathod

DATASET = {
#    "TRAIN_DIR": "dataset/train_images",
#    "TRAIN_MASKS": "dataset/train_masks"
}

# MODEL STORED WEIGHTS ////////////////////////////////////////////////////

WEIGHTS = {
    "WEIGHTS_DIR": "Python/weights",
    "SELECTED_WEIGHTS": "Python/weights/building_unet_model_blurry_imgs.pth"
}

# SUPPORTED INTEL GPU HARDWARE ////////////////////////////////////////////////////////

OPENVINO = {
    "MODEL_PATH_OV": "Python/openvino_intelgpu/building_unet_openvino.xml"
}

# INFERENCE STAGE INPUT DICTIONARY ///////////////////////////////////////////

INPUT = {
    # Test images directory for infer script
    "TEST_DIR": "test_images"
}

# INFERENCE STAGE OUTPUT DICTIONARY //////////////////////////////////////////

# Output directories for segmentation model predictions
OUTPUT = {
    # Predicted building masks directory
    "MASK_DIR": "Python/predictions_output/masks",
    # Predicted building masks overlayed with satellite images
    "OVERLAY_DIR": "Python/predictions_output/overlays",

    # Enviroment var to clear the output dir before inference stage
    "CLEAR_ON_RUN": True
}

PATHS = {

    "DATASET": DATASET,

    "WEIGHTS": WEIGHTS,

    "OPENVINO": OPENVINO,

    "INPUT": INPUT,
    "OUTPUT": OUTPUT,
   
    
}