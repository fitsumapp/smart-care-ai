"""
passenger_wsgi.py — cPanel Python WSGI Entry Point for MedPulse Smart-Care AI
This file is required by cPanel's 'Setup Python App' (Phusion Passenger).
"""

import os
import sys

# 1. Add application directory to sys.path
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, BASE_DIR)

# 2. Set Django settings module
os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'ai_nurse_server.settings')

# 3. Load Django WSGI application
from django.core.wsgi import get_wsgi_application
application = get_wsgi_application()
