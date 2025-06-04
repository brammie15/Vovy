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

    with urllib.request.urlopen(url) as response:
        total_size = int(response.headers.get("Content-Length", 0))
        downloaded = 0
        buffer = io.BytesIO()

        block_size = 8192
        last_reported_percent = -10  # Start below 0 to trigger first 0%

        while True:
            chunk = response.read(block_size)
            if not chunk:
                break
            buffer.write(chunk)
            downloaded += len(chunk)

            percent = downloaded * 100 // total_size if total_size else 0

            # Only print every 10%
            if percent >= last_reported_percent + 10:
                print(f"Progress: {percent:3d}%")
                last_reported_percent = percent - (percent % 10)

        print("Download complete. Extracting...")

        buffer.seek(0)
        with zipfile.ZipFile(buffer) as zip_ref:
            zip_ref.extractall(extract_to)

        print(f"Resources extracted to {extract_to}")

def main():
    if is_directory_empty(RESOURCE_DIR):
        print("Resources directory is empty. Downloading resources...", flush=True)
        print("This **will** take a while, please be patient.", flush=True)
        print("Also cats are cute, so you should pet one while waiting.", flush=True)
        os.makedirs(RESOURCE_DIR, exist_ok=True)
        download_and_extract_zip(ZIP_URL, RESOURCE_DIR)
    else:
        print("Resources already present. Skipping download.")

if __name__ == "__main__":
    main()
