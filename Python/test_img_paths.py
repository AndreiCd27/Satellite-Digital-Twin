
import os
import glob
from paths_config import PATHS

image_extensions = ["*.tif", "*.tiff", "*.png", "*.jpg", "*.jpeg"]

INPUT_TEST_DIR = PATHS["INPUT"]["TEST_DIR"]

def GET_INRIA_TEST_IMG_PATHS():
    paths = []

    for ext in image_extensions:
        paths.extend(glob.glob(os.path.join(INPUT_TEST_DIR, ext)))
        paths.extend(glob.glob(os.path.join(INPUT_TEST_DIR, ext.upper())))
    
    
    if not paths:
        print(f"No images inside directory: {INPUT_TEST_DIR}")
    else:
        print(f"Identified {len(paths)} Images")
        print("From INRIA dataset at https://www.kaggle.com/datasets/sagar100rathod/inria-aerial-image-labeling-dataset/data")
    
    return paths

def GET_TEST_IMG_FROM_FILE(filename_with_img_paths):
    '''
    Image paths in the given file should be relative to main directory
    '''
    paths = []

    with open(filename_with_img_paths, "r") as f:
        for line in f:
            if line.strip() != "":
                line = line.strip()
                ext = line.split(".")[1]
                valid_ext = False
                for e in image_extensions:
                    if ("*."+ext) == e:
                        valid_ext = True
                        break
                if valid_ext == True:
                    paths.append(line)
                else:
                    print(f"File extension not supported as image ({line})")

    
    if not paths:
        print("No images identified")
    else:
        print(f"Identified {len(paths)} Images")
    
    return paths
