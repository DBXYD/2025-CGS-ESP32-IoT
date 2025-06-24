# website/calendar_utils.py

from pathlib import Path
from google.oauth2 import service_account
from googleapiclient.discovery import build

# 1) Où se trouve votre JSON (BASE → …/CGS_Website/cgs)
BASE = Path(__file__).resolve().parent.parent
SERVICE_ACCOUNT_FILE = BASE / "my-service-account.json"

# 2) On crée les credentials et le client
SCOPES = ["https://www.googleapis.com/auth/calendar"]
_creds = service_account.Credentials.from_service_account_file(
    str(SERVICE_ACCOUNT_FILE),
    scopes=SCOPES,
)
calendar_service = build("calendar", "v3", credentials=_creds)

# 3) L’ID de votre calendrier
CALENDAR_ID = "lhanzhon@gmail.com"
