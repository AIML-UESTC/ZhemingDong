import json
import streamlit as st
import detect 
from PIL import Image
import io
import numpy as np
import cv2


@st.cache_data()
def load_model():
    """Retrieves the trained model and maps it to the CPU by default,
    can also specify GPU here."""
    model_path = 'yolov5-ghost.pt'
    model,names,pt,imgsz,stride=detect.load_model(model_path,(640,640))
    return model,names,pt,imgsz,stride


@st.cache_data()
def load_list_of_images_available(
        all_image_files: dict,
        image_files_dtype: str,
        bird_species: str
        ) -> list:
    """Retrieves list of available images given the current selections"""
    species_dict = all_image_files.get(image_files_dtype)
    list_of_files = species_dict.get(bird_species)
    return list_of_files


@st.cache_data()
def load_s3_file_structure(path: str = 'image.json'):
    """Retrieves JSON document outining the S3 file structure"""
    with open(path, 'r') as f:
        return json.load(f)
    
def load_image():
    all_image_files = load_s3_file_structure()
    index_of_image = sorted(list(all_image_files.keys()))

    dataset_type = st.sidebar.selectbox(
                "Select an image", index_of_image)
    image_files_subset = all_image_files[dataset_type]['data_path']
    return image_files_subset

def inference(model,names,pt,imgsz,stride,file,local):
    result_img = detect.run(model=model,source=file,names=names,pt=pt,imgsz=imgsz,stride=stride,local=local)
    col1, col2 = st.columns(2)
    with col1:
        st.image(file, caption='org img', use_column_width='auto')
    with col2:
        st.image(result_img, caption='pred result')
    return 0


def run():
    st.title('Welcome to YoloV5-Ghost inference demo!')
    instructions = """
    Please select an image on the left or upload an image to see the network inference results.
        """
    st.write(instructions)

    model,names,pt,imgsz,stride=load_model()

    file = st.file_uploader('Upload An Image')
    local = True
    if file:  # if user uploaded file
        local = False
        img_bytes = file.read()
        # 使用 Pillow 解码图片
        image = Image.open(io.BytesIO(img_bytes))
        # 将 Pillow 图片对象转换为 Numpy 数组
        inference(model,names,pt,imgsz,stride,image,local=local)
    else:
        image_files_subset = load_image()
        file = image_files_subset
        inference(model,names,pt,imgsz,stride,file,local=local)

if __name__ == "__main__":
    run()
