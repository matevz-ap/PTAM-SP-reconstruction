import codecs
import os
from flask import Flask, make_response

app = Flask(__name__)

@app.route("/", methods=["GET"])
def index():
    return "Hello"

@app.route("/test", methods=["GET"])
def test():
    os.system("cd build/; ./reconstruction_cli stay")
    return "ok"

@app.route("/init", methods=["GET"])
def initialize_reconstruction():
    os.system("cd build/; ./reconstruction_cli init")
    return "OK"

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