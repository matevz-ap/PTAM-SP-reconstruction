import json
import subprocess
import os
import sys
import re

def make_response(numbers, success):
    return json.dumps({
        "success": success, 
        "duration": numbers[0],
        "views": numbers[1],
        "estimated_views": numbers[2],
        "tracks": numbers[3],
        "estimated_tracks": numbers[4],
        "vertices": numbers[12] if len(numbers) > 10 else 0,
        "faces": numbers[13] if len(numbers) > 10 else 0,
    })


def init_reconstruction_task(uuid):
    command = f"cd build/; ./reconstruction_cli init ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    numbers = re.findall("[-+]?(?:\d*\.\d+|\d+)", output)
    numbers.pop(1)
    print(output)
    print(make_response(numbers, "Initialization successful" in output))
    return make_response(numbers, "Initialization successful" in output)
    

def extend_reconstruction_task(uuid, number_of_images):
    command = f"cd build/; ./reconstruction_cli extend ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid} {number_of_images - 1}"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    numbers = re.findall("[-+]?(?:\d*\.\d+|\d+)", output)
    return make_response(numbers, "Extend failed" not in output)


def generate_ply(uuid):
    command = f"cd build/; ./reconstruction_cli download ply ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()


def download_ptam_task(uuid):
    os.system(f"""cd build/; ./reconstruction_cli download ptam ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/""")
