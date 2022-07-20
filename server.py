import codecs
from mimetypes import init
import os
import shortuuid
from flask import Flask, make_response, request

app = Flask(__name__)

def save_file(uuid, file):
    suffix = file.filename.split(".")[-1]
    num_of_images = len([file for file in os.scandir(f"data/{uuid}/images")])
    file.save(f"data/{uuid}/images/{num_of_images}.{suffix}")

@app.route("/", methods=["GET"])
def index():
    return "Hello"

@app.route("/test", methods=["GET"])
def test():
    os.system("cd build/; ./reconstruction_cli stay")
    return "ok"

@app.route("/init", methods=["POST"])
def initialize_reconstruction():
    if "image" not in request.files:
        return "Missing requred reques paramater: 'image' of type file", 400

    uuid = shortuuid.uuid()
    os.system(f"mkdir -p data/{uuid}/images")
    save_file(uuid, request.files['image'])

    return uuid

@app.route("/extend/<uuid>", methods=["POST"])
def extend_reconstruction(uuid):
    if "image" not in request.files:
        return "Missing requred reques paramater: 'image' of type file", 400

    save_file(uuid, request.files['image'])

    number_of_images = len([file for file in os.scandir(f"data/{uuid}/images")])
    if  number_of_images == 2: # also needs check that init is not in progress
        os.system(f"""cd build/; ./reconstruction_cli init ../data/{uuid}/images/ ../dataset/opeka/prior_calibration.txt ../data/{uuid}""")
    else:
        os.system(f"""cd build/; ./reconstruction_cli extend ../data/{uuid}/images/ ../dataset/opeka/prior_calibration.txt ../data/{uuid} {number_of_images - 1}""")
    return "OK"


@app.route("/download_mvs", methods=["GET"])
def download_mvs():
    file_data = codecs.open("dataset_new/opeka/reconstruction/opeka.mvs", 'rb').read()
    response = make_response()
    response.data = file_data
    return response

@app.route("/download_ply", methods=["GET"])
def download_ply():
    file_data = codecs.open("dataset_new/opeka/reconstruction/opeka.ply", 'rb').read()
    response = make_response()
    response.data = file_data
    return response