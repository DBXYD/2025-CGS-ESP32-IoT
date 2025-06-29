from django.urls import path
from django.conf import settings
from django.conf.urls.static import static
from . import views
from .views import control_device, esp_status_api


urlpatterns = [
    path('', views.index, name='index'),
    path('studios', views.studios, name='studios'),
    path('bar', views.bar, name='bar'),
    path('booking', views.booking, name='booking'),
    path('contact', views.contact, name='contact'),
    path('cgu', views.cgu, name='cgu'),
    path('control/', views.control_panel, name='control_panel'),
    path('api/control/', control_device),
    path('controle/', views.controle_view, name='controle'),
    path('api/control/', views.control_device, name='control_device'),
    path('api/esp/status/', esp_status_api, name="esp_status"),
    path("api/esp/<int:esp_id>/toggle/", views.toggle_esp_status, name="toggle_esp_status"),
    path('api/esp/status/', views.esp_status_api, name='esp_status'),
    path("api/esp/ping/", views.esp_ping, name="esp_ping"),
    path('api/studio/<slug:slug>/devices/', views.studio_devices, name='studio_devices'),
    path('gestion-reservations/', views.gestion_reservations, name='gestion_reservations'),
    path('profil/', views.profil_view, name='profil'),
]

if settings.DEBUG:
    urlpatterns += static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
