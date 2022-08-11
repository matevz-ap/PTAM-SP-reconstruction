import os
from PIL import Image
import shortuuid
from flask import Flask, request, send_file
from flask_cors import CORS
from rq import Queue
from worker import conn

from tasks import download_ptam_task, init_reconstruction_task, extend_reconstruction_task

app = Flask(__name__)
CORS(app)

q = Queue(connection=conn)

def _number_of_images(uuid):
    return len([file for file in os.scandir(f"data/{uuid}/images")])

def save_file(uuid, file):
    num_of_images = _number_of_images(uuid)
    file.save(f"data/{uuid}/images/{num_of_images}.jpg")

def get_camera_settings(uuid, image):
    exif = image.getexif().get_ifd(0x8769)
    
    with open(f"data/{uuid}/camera_settings.txt", 'a') as file:
        file.write(f"{exif[40962]}\n") # width
        file.write(f"{exif[40963]}\n") # heigt
        file.write(f"{exif[37386] * 1000}\n") # focal length
        file.write(f"{exif[40962] / 2}\n") # height / 2
        file.write(f"{exif[40963] / 2}\n") # width / 2
        file.write("1.0\n") # aspect ratio
        file.write("0.0\n\n") # skew

@app.route("/init", methods=["POST"])
def initialize_reconstruction():
    if "image" not in request.files:
        return "Missing requred request paramater: 'image' of type file", 400
    
    uuid = shortuuid.uuid()
    image = request.files['image']
    os.system(f"mkdir -p data/{uuid}/images")
    save_file(uuid, image)
    get_camera_settings(uuid, Image.open(image))
    return uuid

@app.route("/<uuid>/extend", methods=["POST"])
def extend_reconstruction(uuid):
    if "image" not in request.files:
        return "Missing requred reques paramater: 'image' of type file", 400

    save_file(uuid, request.files['image'])

    number_of_images = _number_of_images(uuid)
    if  number_of_images == 2: # also needs check that init is not in progress
        q.enqueue(init_reconstruction_task, uuid)
    else:
        q.enqueue(extend_reconstruction_task, uuid, number_of_images)

    return "OK"

@app.route("/<uuid>/download_ply", methods=["GET"])
def download_ply(uuid):
    os.system(f"""cd build/; ./reconstruction_cli download ply ../data/{uuid}/images/ ../dataset/opeka/prior_calibration.txt ../data/{uuid}/""")
    return send_file(f"./data/{uuid}/ply.ply")

@app.route("/<uuid>/download_mvs", methods=["GET"])
def download_mvs(uuid):
    return send_file(f"./data/{uuid}/scene.mvs")

@app.route("/<uuid>/download_ptam", methods=["GET"])
def download_ptam(uuid):
    q.enqueue(download_ptam_task, uuid)
    return send_file(f"./data/{uuid}/installer")

if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port=5000)