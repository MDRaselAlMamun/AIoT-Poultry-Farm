# Server Setup

This section covers:

- **Step 1:** Install VS Code
- **Step 2:** Django project setup (project + apps + models)
- **Step 3:** PostgreSQL configuration + Django DB connection
- **Step 4:** Run Django directly (test before deployment)
- **Step 5:** uWSGI + Nginx configuration (production)

> **Project folder:** `/var/www/poultry_farm`  
> **Virtual environment:** `/var/www/poultry_farm/venv`  
> **Hostname:** `kali`

## 1. Install VS Code

> Recommended: install VS Code before creating the Django project, so you can edit files easily.

### Download and install VS Code (.deb package)

Open the official VS Code download page and download the **Linux ARM64 .deb** package (Raspberry Pi 5 is usually ARM64):

> https://code.visualstudio.com/download

Then install it from terminal (example file name may change by version):

```bash
cd ~/Downloads
sudo apt install ./code_*_arm64.deb
```

### Open VS Code

You can open VS Code from:

Raspberry Pi menu → Programming → Visual Studio Code

or terminal:

```bash
code /var/www/poultry_farm
```

## 2. Django Project Setup (create project + apps + models)

### Go to project folder and activate virtual environment

```bash
cd /var/www/poultry_farm
source venv/bin/activate
```

### Create Django project

Create the Django project inside the current folder (**important:** `.` at the end):

```bash
django-admin startproject config .
```

This will create:

```txt
/var/www/poultry_farm
├── config/
│   ├── settings.py
│   ├── urls.py
│   ├── wsgi.py
│   └── asgi.py
├── manage.py
└── venv/
```

### Create Django apps

Create apps for a clean modular structure:

```bash
python manage.py startapp control
python manage.py startapp dashboard
```

### Register apps in Django settings

Edit: `config/settings.py` file

Add these apps in INSTALLED_APPS:

```py
INSTALLED_APPS = [
    # default django apps...
    'django.contrib.admin',
    'django.contrib.auth',
    'django.contrib.contenttypes',
    'django.contrib.sessions',
    'django.contrib.messages',
    'django.contrib.staticfiles',

    # third-party
    'rest_framework',

    # local apps
    'devices',
    'telemetry',
    'control',
    'dashboard',
    'alerts',
]
```

### Configure templates/static/media (important for your HTML frontend)

Edit `config/settings.py` file and set below:

```py
BASE_DIR = Path(__file__).resolve().parent.parent

TEMPLATES[0]['DIRS'] = [BASE_DIR / 'templates']

STATIC_URL = '/static/'
STATICFILES_DIRS = [BASE_DIR / 'static']   # your frontend static files (css/js/images)
STATIC_ROOT = BASE_DIR / 'staticfiles'     # collected static files for deployment

MEDIA_URL = '/media/'
MEDIA_ROOT = BASE_DIR / 'media'
```

### Create frontend folders and place your files

```bash
mkdir -p templates
mkdir -p static/css static/js static/images
mkdir -p media
```

Now copy your frontend files into:

HTML files → `templates/`

CSS → `static/css/`

JS → `static/js/`

Images → `static/images/`

Example: `templates/index.html`, `static/css/style.css`, etc.

### Create a basic dashboard view (test frontend rendering)

Edit: `dashboard/views.py` file

```py
from django.shortcuts import render

def home(request):
    return render(request, 'index.html')
```

### Create app URL file (dashboard)

Create and Edit: `dashboard/urls.py` file

```py
from django.urls import path
from . import views

urlpatterns = [
    path('', views.home, name='home'),
]
```

### Connect dashboard URLs to main project URLs

Edit: `config/urls.py` file:

```py
from django.contrib import admin
from django.urls import path, include
from django.conf import settings
from django.conf.urls.static import static

urlpatterns = [
    path('admin/', admin.site.urls),
    path('', include('dashboard.urls')),
]

if settings.DEBUG:
    urlpatterns += static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
```