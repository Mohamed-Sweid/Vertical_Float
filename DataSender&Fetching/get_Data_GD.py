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

# Get all records
data = sheet.get_all_records()

# Get specific range
values = sheet.get("A1:H10")  # Gets first 10 rows

print(*values,sep='\n')
