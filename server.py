import codecs
import os
import shortuuid
from flask import Flask, make_response, request

app = Flask(__name__)

@app.route("/", methods=["GET"])
def index():
    return "Hello"

@app.route("/test", methods=["GET"])
def test():
    os.system("cd build/; ./reconstruction_cli stay")
    return "ok"

@app.route("/init", methods=["POST"])
def initialize_reconstruction():
    print(request.__dict__)

    if "image" not in request.files:
        return "You need to provide 2 images in order to initialize reconstruction", 400
    # os.system("cd build/; ./reconstruction_cli init")

    uuid = shortuuid.uuid()
    os.system(f"mkdir -p data/{uuid}/images")

    file = request.files['image']
    file.save(f"data/{uuid}/images/{file.filename}")

    return uuid

@app.route("/extend", methods=["GET"])
def extend_reconstruction():
    os.system("cd build/; ./reconstruction_cli extend")
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