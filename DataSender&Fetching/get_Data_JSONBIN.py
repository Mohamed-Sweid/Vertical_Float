import os
import time
import datetime
import requests

API_KEY = "$2a$10$PUd3T5ngb5tPVdW5MQZn7.gOUUULwALgy0jEtu0lPBM5QYgFUmvpi"
BIN_ID = "67a127fbad19ca34f8f94a7c"  # Existing bin ID for data
response = requests.get(
    f"https://api.jsonbin.io/v3/b/{BIN_ID}",
    headers={"X-Master-Key": API_KEY}
)
full_history = response.json()['record']['full_data']

for entry in full_history:
    numbers = entry[:7]
    timestamp = datetime.datetime.fromtimestamp(entry[7]/1000)
    print(f"{timestamp}: {numbers}")
