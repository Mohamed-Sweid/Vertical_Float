import os
import time
import datetime
import requests
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

# Configuration
API_KEY = "$2a$10$PUd3T5ngb5tPVdW5MQZn7.gOUUULwALgy0jEtu0lPBM5QYgFUmvpi"
BIN_ID = "67a127fbad19ca34f8f94a7c"  # Existing bin ID for updates
NUMBERS_FILE = "numbers.txt"  # File to watch for changes
CHECK_INTERVAL = 60  # Seconds between checks (for scheduler method)

class FileHandler(FileSystemEventHandler):
    def on_modified(self, event):
        if event.src_path.endswith(NUMBERS_FILE):
            print("\nFile modification detected!")
            process_and_send_numbers()

def get_last_line_numbers():
    try:
        with open(NUMBERS_FILE, "r") as f:
            lines = f.readlines()
            if not lines:
                raise ValueError("File is empty")
                
            last_line = lines[-1].strip()
            numbers = [float(num.strip()) for num in last_line.split(",")]
            
            if len(numbers) != 7:
                raise ValueError("Last line must contain exactly 7 numbers")
                
            return numbers
    except Exception as e:
        print(f"Error reading file: {str(e)}")
        return None

def send_to_jsonbin(new_numbers):
    url = f"https://api.jsonbin.io/v3/b/{BIN_ID}"
    headers = {
        "Content-Type": "application/json",
        "X-Master-Key": API_KEY
    }
    
    try:
        # 1. Get existing data
        response = requests.get(url, headers=headers)
        existing_data = response.json().get('record', {})
        
        # 2. Initialize or get full_data array
        full_data = existing_data.get('full_data', [])
        
        # 3. Create new entry with timestamp
        timestamp = int(datetime.datetime.now().timestamp() * 1000)
        new_entry = new_numbers + [timestamp]  # 8-element array
        
        # 4. Append new entry
        full_data.append(new_entry)
        
        # 5. Update the bin with combined data
        update_response = requests.put(
            url,
            headers=headers,
            json={"full_data": full_data}
        )
        update_response.raise_for_status()
        
        print(f"Successfully appended entry. Total entries: {len(full_data)}")
        print(f"New entry: {new_entry}")
        return True
        
    except Exception as e:
        print(f"API Error: {str(e)}")
        return False

def process_and_send_numbers():
    numbers = get_last_line_numbers()
    if numbers and send_to_jsonbin(numbers):
        # Optional: Add success logging/marking
        pass

def start_file_watcher():
    event_handler = FileHandler()
    observer = Observer()
    observer.schedule(event_handler, path=".", recursive=False)
    observer.start()
    
    print(f"Watching for changes to {NUMBERS_FILE}...")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()

if __name__ == "__main__":
    # Initial test
    #process_and_send_numbers()
    
    # Start file watcher
    start_file_watcher()
