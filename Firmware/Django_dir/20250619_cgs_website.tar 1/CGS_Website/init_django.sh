#!/bin/bash

# Step 1: Create a Python virtual environment
echo "Creating Python virtual environment..."
python3 -m venv env

# Step 2: Activate the virtual environment
echo "Activating virtual environment..."
source env/bin/activate

# Step 3: Install Django
echo "Installing Django..."
pip install django

# Step 4: Create a new Django project named 'cgs'
echo "Creating Django project 'cgs'..."
django-admin startproject cgs

cd cgs

# Step 5: Create a new Django app named 'website'
echo "Creating Django app 'website'..."
python manage.py startapp website

# Step 6: Add the 'website' app to the project settings
echo "Configuring project settings..."
sed -i "/INSTALLED_APPS = \[/a\    'website'," cgs/settings.py

# Step 7: Create a basic view in 'website/views.py'
echo "Creating a default view..."
cat <<EOT >> website/views.py
from django.http import HttpResponse

def index(request):
    return HttpResponse("Hello, welcome to the CGS website!")
EOT

# Step 8: Configure URL routing in 'website/urls.py'
echo "Configuring URLs for the website app..."
mkdir website/urls

cat <<EOT >> website/urls.py
from django.urls import path
from . import views

urlpatterns = [
    path('', views.index, name='index'),
]
EOT

# Step 9: Include 'website' URLs in the main project URLs
echo "Including 'website' URLs in project URLs..."
sed -i "/from django.urls import path/i from django.urls import include" cgs/urls.py
sed -i "/urlpatterns = \[/a\    path('', include('website.urls'))," cgs/urls.py

# Step 10: Apply migrations
echo "Running migrations..."
python manage.py migrate

# Step 11: Prompt for the admin credentials
echo "Please enter the admin username:"
read admin_user
echo "Please enter the admin email:"
read admin_email
echo "Please enter the admin password:"
read -s admin_password

# Step 12: Create a superuser with the entered credentials
echo "Creating superuser..."
echo "from django.contrib.auth.models import User; User.objects.create_superuser('$admin_user', '$admin_email', '$admin_password')" | python manage.py shell

# Step 13: Run the development server
echo "Starting the Django development server..."
python manage.py runserver
