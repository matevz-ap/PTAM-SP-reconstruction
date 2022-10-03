import json
import subprocess
import re

def make_response(uuid, numbers, success):
    try:
        return json.dumps({
            "reconstruction_id": uuid,
            "success": success, 
            "duration": numbers[0],
            "views": numbers[1],
            "estimated_views": numbers[2],
            "tracks": numbers[3],
            "estimated_tracks": numbers[4],
            "vertices": numbers[12] if len(numbers) > 10 else 0,
            "faces": numbers[13] if len(numbers) > 10 else 0,
            "status": "finished",
        })
    except BaseException:
        return json.dumps({"success": success})


def init_reconstruction_task(uuid):
    command = f"cd build/; ./reconstruction_cli init ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    numbers = re.findall("[-+]?(?:\d*\.\d+|\d+)", output)
    numbers.pop(1)
    return make_response(uuid, list(map(float, numbers)), "Initialization successful" in output)
    

def extend_reconstruction_task(uuid, number_of_images):
    command = f"cd build/; ./reconstruction_cli extend ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid} {number_of_images - 1}"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    numbers = re.findall("[-+]?(?:\d*\.\d+|\d+)", output)
    return make_response(uuid, list(map(float, numbers)), "Extend successful" in output)


def generate_ply_task(uuid):
    command = f"cd build/; ./reconstruction_cli download ply ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    return {
        "succes": True,
        "reconstruction_id": uuid,
    }


def generate_ptam_task(uuid):
    command = f"cd build/; ./reconstruction_cli download ptam ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
