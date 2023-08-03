import json
import subprocess
import re
import requests
import zipfile
import shutil

# server_url = "http://localhost:5000"
server_url = "https://testing-reconstruction-drainn.loca.lt"

def send_files(uuid):
    folder_path = f"./data/{uuid}"
    shutil.make_archive(folder_path, 'zip', folder_path)

    with open(f"{folder_path}.zip", 'rb') as f:
        data = f.read()
    requests.post(f"{server_url}/{uuid}/upload", data=data, headers={'Content-Type': 'application/zip'})


def download_files(uuid):
    response = requests.get(f"{server_url}/{uuid}/download")

    if response.status_code == 200:
        with open(f"./data/{uuid}.zip", "wb") as f:
            f.write(response.content)
        print("File downloaded successfully")
    else:
        print("File not found")

    with zipfile.ZipFile(f"./data/{uuid}.zip", 'r') as zip_ref:
        zip_ref.extractall(f"./data/{uuid}")
    
def delete_files(uuid):
    subprocess.run(f"rm -r ./data/{uuid}", shell=True)
    subprocess.run(f"rm -r ./data/{uuid}.zip", shell=True)

def make_response(uuid, numbers, success, output=None):
    try:
        return json.dumps({
            "reconstruction_id": uuid,
            "success": success, 
            "duration": numbers[0],
            "views": numbers[1],
            "estimated_views": numbers[2],
            "tracks": numbers[3],
            "estimated_tracks": numbers[4],
            "status": "finished",
            "vertices": numbers[12] if len(numbers) > 10 else 0,
            "faces": numbers[13] if len(numbers) > 10 else 0,
            "output": output or "",
        })
    except BaseException:
        return json.dumps({"status": "finished", "success": success, "output": output or ""})


def init_reconstruction_task(uuid):
    download_files(uuid)
    command = f"cd build/; ./reconstruction_cli init ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    numbers = re.findall("[-+]?(?:\d*\.\d+|\d+)", output)
    print(output)
    delete_files(uuid)
    if len(numbers) > 1: 
        numbers.pop(1)
    return make_response(uuid, list(map(float, numbers)), "Initialization successful" in output, output)
    

def extend_reconstruction_task(uuid, number_of_images):
    download_files(uuid)
    command = f"cd build/; ./reconstruction_cli extend ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid} {number_of_images - 1}"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    print(output)
    delete_files(uuid)
    numbers = re.findall("[-+]?(?:\d*\.\d+|\d+)", output)
    return make_response(uuid, list(map(float, numbers)), "Extend successful" in output, output)

def reconstruct_mesh_task(uuid):
    command = f"cd build/; ./reconstruction_cli reconstruct_mesh ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    return json.dumps({
        "finished": True,
        "reconstruction_id": uuid,
    })

def texture_task(uuid):
    download_files(uuid)
    command = f"cd build/; ./reconstruction_cli texture ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    send_files(uuid)
    delete_files(uuid)
    return json.dumps({
            "finished": True,
            "reconstruction_id": uuid,
        })


def generate_ply_task(uuid):
    download_files(uuid)
    command = f"cd build/; ./reconstruction_cli download ply ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    delete_files(uuid)
    return {
        "succes": True,
        "reconstruction_id": uuid,
    }


def generate_ptam_task(uuid):
    download_files(uuid)
    command = f"cd build/; ./reconstruction_cli ptam ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    print(output)
    delete_files(uuid)
    return json.dumps({
            "finished": True,
            "reconstruction_id": uuid,
        })

def refine_mesh_task(uuid):
    download_files(uuid)
    command = f"cd build/; ./reconstruction_cli refine ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/"
    output = subprocess.run(command, capture_output=True, shell=True).stdout.decode()
    print(output)
    send_files(uuid)
    delete_files(uuid)
    return json.dumps({
            "finished": True,
            "reconstruction_id": uuid,
        })