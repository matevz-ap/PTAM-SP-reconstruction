import os
import json
from PIL import Image
import shortuuid
import subprocess

from flask import Flask, request, send_file, jsonify
from flask_cors import CORS
from rq import Queue
from rq.job import Job
from rq.exceptions import NoSuchJobError
from worker import conn

from tasks import generate_ptam_task, generate_ply_task, init_reconstruction_task, extend_reconstruction_task

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
        file.write(f"{exif[37386] * 1000}\n" if exif[37386] > 0 else "4100\n") # focal length
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
        job = q.enqueue(init_reconstruction_task, uuid)
    else:
        job = q.enqueue(extend_reconstruction_task, uuid, number_of_images)

    return job.get_id()

@app.route("/<uuid>/generate/ply", methods=["GET"])
def generate_ply(uuid):
    job = q.enqueue(generate_ply_task, uuid)
    return job.get_id()

@app.route("/<uuid>/generate/ptam", methods=["GET"])
def generate_ptam(uuid):
    job = q.enqueue(generate_ptam_task, uuid)
    return job.get_id()

@app.route("/<uuid>/download/ply", methods=["GET"])
def download_ply(uuid):
    command = f"cd build/; ./reconstruction_cli download ply ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/"
    subprocess.run(command, capture_output=True, shell=True)
    return send_file(f"./data/{uuid}/ply.ply")

@app.route("/<uuid>/download/mvs", methods=["GET"])
def download_mvs(uuid):
    return send_file(f"./data/{uuid}/scene.mvs")

@app.route("/<uuid>/download/ptam", methods=["GET"])
def download_ptam(uuid):
    return send_file(f"./data/{uuid}/installer")

@app.route("/results/<job_key>", methods=['GET'])
def get_results(job_key):
    try:
        job = Job.fetch(job_key, connection=conn)
    except NoSuchJobError:
        return json.dumps({"status": "does_not_exist"}), 202

    if job.is_finished:
        return json.loads(job.result), 200
    else:
        return json.dumps({"status": "in_progress"}), 202

if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', port=5000)