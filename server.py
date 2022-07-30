import codecs
import os
import shortuuid
from flask import Flask, make_response, request, send_file
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

def save_file(uuid, file):
    suffix = file.filename.split(".")[-1]
    num_of_images = len([file for file in os.scandir(f"data/{uuid}/images")])
    file.save(f"data/{uuid}/images/{num_of_images}.png")

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

@app.route("/<uuid>/extend", methods=["POST"])
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
def download_mvs_old():
    file_data = codecs.open("dataset_new/opeka/reconstruction/opeka.mvs", 'rb').read()
    response = make_response()
    response.data = file_data
    return response

@app.route("/<uuid>/download_ply", methods=["GET"])
def download_ply(uuid):
    os.system(f"""cd build/; ./reconstruction_cli download ply ../data/{uuid}/images/ ../dataset/opeka/prior_calibration.txt ../data/{uuid}/""")
    return send_file(f"./data/{uuid}/ply.ply")

@app.route("/<uuid>/download_mvs", methods=["GET"])
def download_mvs(uuid):
    return send_file(f"./data/{uuid}/scene.mvs")

@app.route("/<uuid>/download_ptam", methods=["GET"])
def download_ptam(uuid):
    os.system(f"""cd build/; ./reconstruction_cli download ptam ../data/{uuid}/images/ ../dataset/opeka/prior_calibration.txt ../data/{uuid}/""")
    return send_file(f"./data/{uuid}/ptam")

if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port=5000)