import os

def init_reconstruction_task(uuid):
    os.system(f"""cd build/; ./reconstruction_cli init ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}""")

def extend_reconstruction_task(uuid, number_of_images):
    os.system(f"""cd build/; ./reconstruction_cli extend ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid} {number_of_images - 1}""")

def download_ptam_task(uuid):
    os.system(f"""cd build/; ./reconstruction_cli download ptam ../data/{uuid}/images/ ../data/{uuid}/camera_settings.txt ../data/{uuid}/""")
