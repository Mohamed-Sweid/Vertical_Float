import random
import time
from datetime import datetime

FILE_PATH = "numbers.txt"  # Change this to your desired file path

def generate_and_save_numbers():
    # Generate 7 random numbers (adjust range as needed)
    numbers = [random.randint(5, 200) for _ in range(7)]
    numbers = map(str,numbers)
    numbers2 = (",").join(numbers)
    # Get current timestamp
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Write to file
    with open(FILE_PATH, "a") as file:
        file.write(f"{numbers2}\n")
    print(f"Numbers saved at {timestamp},{numbers2}")

if __name__ == "__main__":
    try:
        while True:
##            generate_and_save_numbers()
            # Wait for 2 minutes (120 seconds)
            time.sleep(10)
    except KeyboardInterrupt:
        print("\nScript stopped by user")
