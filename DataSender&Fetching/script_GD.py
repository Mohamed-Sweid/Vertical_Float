import os
import time
import datetime
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
import gspread
from oauth2client.service_account import ServiceAccountCredentials

# Configuration
CREDENTIALS_FILE = 'credentials.json'
SHEET_ID = '1Jowav3RktAQIUirp8BHtn8AI-6eG-cNpNwwp2gd43vU'
SHEET_NAME = 'Sheet1'  # Default worksheet name
NUMBERS_FILE = 'numbers.txt'

# Google Sheets setup
scope = ['https://www.googleapis.com/auth/spreadsheets']
creds = ServiceAccountCredentials.from_json_keyfile_name(CREDENTIALS_FILE, scope)
client = gspread.authorize(creds)
sheet = client.open_by_key(SHEET_ID).worksheet(SHEET_NAME)

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

def append_to_sheet(numbers):
    try:
        # Add timestamp
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        row_data = numbers + [timestamp]  # 7 numbers + timestamp
        
        # Append to the sheet
        sheet.append_row(row_data)
        print(f"Appended row: {row_data}")
        return True
    except Exception as e:
        print(f"Google Sheets Error: {str(e)}")
        return False

def process_and_send_numbers():
    numbers = get_last_line_numbers()
    if numbers and append_to_sheet(numbers):
        # Optional: Archive/mark processed lines
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
    process_and_send_numbers()
    
    # Start file watcher
    start_file_watcher()
