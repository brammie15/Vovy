import os
import urllib.request
import zipfile
import io
import sys

RESOURCE_DIR = os.path.join(os.path.dirname(__file__), "resources")
ZIP_URL = "https://cdn.brammie15.dev/-fCepCh2TJm/resources.zip"

def is_directory_empty(path):
    return not os.path.exists(path) or len(os.listdir(path)) == 0

def download_and_extract_zip(url, extract_to):
    print(f"Downloading resources from {url}...")
    response = urllib.request.urlopen(url)
    with zipfile.ZipFile(io.BytesIO(response.read())) as zip_ref:
        zip_ref.extractall(extract_to)
    print(f"Resources extracted to {extract_to}")

def main():
    if is_directory_empty(RESOURCE_DIR):
        os.makedirs(RESOURCE_DIR, exist_ok=True)
        download_and_extract_zip(ZIP_URL, RESOURCE_DIR)
    else:
        print("Resources already present. Skipping download.")

if __name__ == "__main__":
    main()
