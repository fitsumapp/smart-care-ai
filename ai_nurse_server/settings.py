"""
Django settings for ai_nurse_server project.
MedPulse Smart-Care AI — Production Ready Configuration
"""

from pathlib import Path
import os
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv(os.path.join(Path(__file__).resolve().parent.parent, '.env'))

# Build paths inside the project
BASE_DIR = Path(__file__).resolve().parent.parent

# ---------------------------------------------------------------
# SECURITY - Secret keys read from .env file
# ---------------------------------------------------------------
SECRET_KEY = os.environ.get(
    'SECRET_KEY',
    'django-insecure-@^(gfq2kk^@)0!4z!f)&^_-pubfy&c7shv!eom9izto-)q8qi^'
)

DEBUG = os.environ.get('DEBUG', 'True') == 'True'

ALLOWED_HOSTS = os.environ.get('ALLOWED_HOSTS', '*').split(',')

# ---------------------------------------------------------------
# Application definition
# ---------------------------------------------------------------
INSTALLED_APPS = [
    'unfold',                         # Modern Tailwind Admin Theme (must be before admin)
    'unfold.contrib.forms',           # Form widgets support
    'django.contrib.admin',
    'django.contrib.auth',
    'django.contrib.contenttypes',
    'django.contrib.sessions',
    'django.contrib.messages',
    'django.contrib.staticfiles',
    'rest_framework',
    'corsheaders',
    'core_api',
]

MIDDLEWARE = [
    'corsheaders.middleware.CorsMiddleware',
    'django.middleware.security.SecurityMiddleware',
    'whitenoise.middleware.WhiteNoiseMiddleware',  # Static files (Production)
    'django.contrib.sessions.middleware.SessionMiddleware',
    'django.middleware.common.CommonMiddleware',
    'django.middleware.csrf.CsrfViewMiddleware',
    'django.contrib.auth.middleware.AuthenticationMiddleware',
    'django.contrib.messages.middleware.MessageMiddleware',
    'django.middleware.clickjacking.XFrameOptionsMiddleware',
]

# Enable CORS for API communication
CORS_ALLOW_ALL_ORIGINS = True

ROOT_URLCONF = 'ai_nurse_server.urls'

TEMPLATES = [
    {
        'BACKEND': 'django.template.backends.django.DjangoTemplates',
        'DIRS': [BASE_DIR / 'templates'],
        'APP_DIRS': True,
        'OPTIONS': {
            'context_processors': [
                'django.template.context_processors.request',
                'django.contrib.auth.context_processors.auth',
                'django.contrib.messages.context_processors.messages',
            ],
        },
    },
]

WSGI_APPLICATION = 'ai_nurse_server.wsgi.application'

CSRF_TRUSTED_ORIGINS = [
    origin.strip() for origin in os.environ.get(
        'CSRF_TRUSTED_ORIGINS',
        'http://localhost:8000,http://127.0.0.1:8000'
    ).split(',') if origin.strip()
]

# ---------------------------------------------------------------
# Database — Flexible: MySQL (cPanel), PostgreSQL, or SQLite
# ---------------------------------------------------------------
DB_ENGINE = os.environ.get('DB_ENGINE', 'django.db.backends.sqlite3')

if DB_ENGINE == 'django.db.backends.mysql':
    DATABASES = {
        'default': {
            'ENGINE': 'django.db.backends.mysql',
            'NAME': os.environ.get('DB_NAME', 'medpulse_db'),
            'USER': os.environ.get('DB_USER', 'root'),
            'PASSWORD': os.environ.get('DB_PASSWORD', ''),
            'HOST': os.environ.get('DB_HOST', 'localhost'),
            'PORT': os.environ.get('DB_PORT', '3306'),
            'OPTIONS': {
                'charset': 'utf8mb4',
                'init_command': "SET NAMES 'utf8mb4' COLLATE 'utf8mb4_unicode_ci'",
            },
        }
    }
elif DB_ENGINE == 'django.db.backends.postgresql':
    DATABASES = {
        'default': {
            'ENGINE': 'django.db.backends.postgresql',
            'NAME': os.environ.get('DB_NAME', 'medpulse_db'),
            'USER': os.environ.get('DB_USER', 'postgres'),
            'PASSWORD': os.environ.get('DB_PASSWORD', ''),
            'HOST': os.environ.get('DB_HOST', 'localhost'),
            'PORT': os.environ.get('DB_PORT', '5432'),
        }
    }
else:
    # Default: SQLite (Development / Fallback / Shared Hosting file DB)
    DATABASES = {
        'default': {
            'ENGINE': 'django.db.backends.sqlite3',
            'NAME': BASE_DIR / 'db.sqlite3',
        }
    }

# ---------------------------------------------------------------
# Password validation
# ---------------------------------------------------------------
AUTH_PASSWORD_VALIDATORS = [
    {'NAME': 'django.contrib.auth.password_validation.UserAttributeSimilarityValidator',},
    {'NAME': 'django.contrib.auth.password_validation.MinimumLengthValidator',},
    {'NAME': 'django.contrib.auth.password_validation.CommonPasswordValidator',},
    {'NAME': 'django.contrib.auth.password_validation.NumericPasswordValidator',},
]

# ---------------------------------------------------------------
# Internationalization - Ethiopia timezone
# ---------------------------------------------------------------
LANGUAGE_CODE = 'en-us'
TIME_ZONE = 'Africa/Addis_Ababa'
USE_I18N = True
USE_TZ = True

# ---------------------------------------------------------------
# Static files - WhiteNoise for Production
# ---------------------------------------------------------------
STATIC_URL = '/static/'
STATICFILES_DIRS = [
    os.path.join(BASE_DIR, 'static'),
]
STATIC_ROOT = os.path.join(BASE_DIR, 'staticfiles')  # collected by collectstatic

# WhiteNoise - Static files in Production
STATICFILES_STORAGE = 'whitenoise.storage.CompressedManifestStaticFilesStorage'

# ---------------------------------------------------------------
# Auth URLs
# ---------------------------------------------------------------
LOGIN_URL = 'admin_login'
LOGIN_REDIRECT_URL = 'admin:index'

# Default primary key field type
DEFAULT_AUTO_FIELD = 'django.db.models.BigAutoField'

# ---------------------------------------------------------------
# Logging - Errors written to file (Production)
# ---------------------------------------------------------------
if not DEBUG:
    LOGS_DIR = os.path.join(BASE_DIR, 'logs')
    os.makedirs(LOGS_DIR, exist_ok=True)
    LOGGING = {
        'version': 1,
        'disable_existing_loggers': False,
        'handlers': {
            'file': {
                'level': 'ERROR',
                'class': 'logging.FileHandler',
                'filename': os.path.join(LOGS_DIR, 'django_errors.log'),
            },
        },
        'loggers': {
            'django': {
                'handlers': ['file'],
                'level': 'ERROR',
                'propagate': True,
            },
        },
    }
# ---------------------------------------------------------------
# Security Settings (Production Only)
# ---------------------------------------------------------------
if not DEBUG:
    # CSRF and Session cookies over HTTPS only (if SSL active)
    # Note: Set to False if hospital network doesn't have SSL/HTTPS
    CSRF_COOKIE_SECURE = True
    SESSION_COOKIE_SECURE = True
    
    # Protect against Browser XSS attacks
    SECURE_BROWSER_XSS_FILTER = True
    SECURE_CONTENT_TYPE_NOSNIFF = True
    
    # HSTS (Enforce SSL for a duration)
    SECURE_HSTS_SECONDS = 31536000  # 1 year
    SECURE_HSTS_INCLUDE_SUBDOMAINS = True
    SECURE_HSTS_PRELOAD = True
    
    # SSL Redirect (Set to True if needed, False is better for local network)
    SECURE_SSL_REDIRECT = False 

# ===============================================================
# 🎨 Django Unfold Modern Admin Theme Configuration
# ===============================================================
from django.urls import reverse_lazy
from django.utils.translation import gettext_lazy as _

UNFOLD = {
    "SITE_TITLE": "Apex | MedPulse Smart-Care AI",
    "SITE_HEADER": "Apex",
    "SITE_SUBHEADER": "Clinical Nurse Call & AI Safety Automation",
    "SITE_URL": "/",
    "SITE_SYMBOL": "dataset",
    "DASHBOARD_CALLBACK": "core_api.admin.dashboard_callback",
    "SHOW_HISTORY": True,
    "SHOW_VIEW_ON_SITE": True,
    "STYLES": [
        lambda request: "/static/css/admin_apex.css",
    ],
    "COLORS": {
        "primary": {
            "50": "236 253 245",
            "100": "209 250 229",
            "200": "167 243 208",
            "300": "110 231 183",
            "400": "52 211 153",
            "500": "16 185 129",
            "600": "5 150 105",
            "700": "4 120 87",
            "800": "6 95 70",
            "900": "6 78 59",
            "950": "2 44 34",
        },
    },
    "SIDEBAR": {
        "show_search": False,
        "show_all_applications": False,
        "navigation": [
            {
                "title": _("DASHBOARDS"),
                "separator": False,
                "collapsible": False,
                "items": [
                    {
                        "title": _("Overview"),
                        "icon": "grid_view",
                        "link": reverse_lazy("admin:index"),
                    },
                    {
                        "title": _("Nurse Live Station"),
                        "icon": "monitor_heart",
                        "link": "/nurse-dashboard/",
                        "target": "_blank",
                    },
                ],
            },
            {
                "title": _("REPORTS"),
                "separator": False,
                "collapsible": False,
                "items": [
                    {
                        "title": _("Reports"),
                        "icon": "pie_chart",
                        "link": "/admin/reports/",
                    },
                    {
                        "title": _("System Logs"),
                        "icon": "terminal",
                        "link": "/admin/system-logs/",
                    },
                ],
            },
            {
                "title": _("ANALYTICS DETAILS"),
                "separator": False,
                "collapsible": False,
                "items": [
                    {
                        "title": _("Station Load"),
                        "icon": "local_hospital",
                        "link": "/admin/analytics/stations/",
                    },
                    {
                        "title": _("Room Demand"),
                        "icon": "meeting_room",
                        "link": "/admin/analytics/rooms/",
                    },
                    {
                        "title": _("Nurse HR Analytics"),
                        "icon": "trending_up",
                        "link": "/admin/analytics/nurses/",
                    },
                ],
            },
            {
                "title": _("MANAGEMENT"),
                "separator": False,
                "collapsible": False,
                "items": [
                    {
                        "title": _("Call Logs & AI Alerts"),
                        "icon": "notifications_active",
                        "link": reverse_lazy("admin:core_api_aicalllog_changelist"),
                    },
                    {
                        "title": _("Nurses (HR)"),
                        "icon": "id_card",
                        "link": reverse_lazy("admin:core_api_nurse_changelist"),
                    },
                    {
                        "title": _("Nurse Stations"),
                        "icon": "desk",
                        "link": reverse_lazy("admin:core_api_nursestation_changelist"),
                    },
                    {
                        "title": _("Manage ER/OR"),
                        "icon": "emergency",
                        "link": reverse_lazy("admin:core_api_specialroom_changelist"),
                    },
                    {
                        "title": _("Patient Room Priorities"),
                        "icon": "hotel",
                        "link": reverse_lazy("admin:core_api_patientstatus_changelist"),
                    },
                ],
            },
            {
                "title": _("ADMINISTRATION & GENERAL"),
                "separator": False,
                "collapsible": False,
                "items": [
                    {
                        "title": _("Users & Staff Accounts"),
                        "icon": "person",
                        "link": reverse_lazy("admin:auth_user_changelist"),
                    },
                    {
                        "title": _("User Groups & Roles"),
                        "icon": "group",
                        "link": reverse_lazy("admin:auth_group_changelist"),
                    },
                    {
                        "title": _("System Hardware Settings"),
                        "icon": "settings",
                        "link": reverse_lazy("admin:core_api_systemsettings_changelist"),
                    },
                ],
            },
        ],
    },
}

# Media Files (User Uploads)
MEDIA_URL = '/media/'
MEDIA_ROOT = os.path.join(BASE_DIR, 'media')
