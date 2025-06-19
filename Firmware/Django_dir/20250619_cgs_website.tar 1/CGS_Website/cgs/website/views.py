from django.shortcuts import render, get_object_or_404, redirect
from django.contrib.auth.decorators import login_required, user_passes_test
from django.http import JsonResponse
from rest_framework.decorators import api_view
from rest_framework.response import Response
from django.utils import timezone
from .models import StudioESP, StudioDevice, News, ESPCommand
import json
from django.utils.timezone import now, timedelta

def index(request):
    upcoming_news = (
        News.objects.filter(end_date__gte=timezone.now())
        .order_by('end_date')
    )[:3]
    return render(request, 'website/index.html', {'upcoming_news': upcoming_news})

def studios(request):
    return render(request, 'website/studios.html')

def bar(request):
    return render(request, 'website/bar.html')

def booking(request):
    return render(request, 'website/booking.html')

def contact(request):
    return render(request, 'website/contact.html')

def cgu(request):
    return render(request, 'website/cgu.html')


@login_required
def control_panel(request):
    devices = StudioDevice.objects.all()
    if request.method == "POST":
        device_id = request.POST.get("device_id")
        device = StudioDevice.objects.get(id=device_id)
        device.is_on = not device.is_on
        device.save()
        return redirect('control_panel')
    return render(request, 'website/control_panel.html', {'devices': devices})


@login_required
def profil_view(request):
    #  on réinitialise les ESP vieux de +15s
    cutoff = now() - timedelta(seconds=15)
    StudioESP.objects.filter(last_ping__lt=cutoff).update(connected=False)

    esps = StudioESP.objects.all()
    return render(request, 'website/profil.html', {'esps': esps})


@api_view(['GET'])
def control_device(request):
    if not request.user.is_authenticated or not request.user.is_superuser:
        return Response({'error': 'Unauthorized'}, status=403)
    cmd = request.GET.get('cmd')
    return Response({'status': f'Commande {cmd} envoyée'})


@login_required
@user_passes_test(lambda u: u.is_superuser)
def toggle_esp_status(request, esp_id):
    if request.method == 'POST':
        esp = get_object_or_404(StudioESP, id=esp_id)
        esp.status = 'OFF' if esp.status == 'ON' else 'ON'
        esp.save()
        return JsonResponse({'status': esp.status})
    return JsonResponse({'error': 'Invalid request'}, status=400)

def reset_all_esp_connection_status():  ## forcer tous les connected = False au démarrage du serveur
    cutoff = now() - timedelta(seconds=15)
    StudioESP.objects.filter(last_ping__lt=cutoff).update(connected=False)


def esp_status_api(request):
    studio_name = request.GET.get('name')
    esp = get_object_or_404(StudioESP, name=studio_name)

    # Marquer l'ESP comme connecté
    esp.connected = True
    esp.last_ping = now()
    esp.save()

    # Vérifie si last_ping est récent (< 15 secondes)
    recently_seen = (now() - esp.last_ping) < timedelta(seconds=15)

    return JsonResponse({
        'state': esp.status,
        'connected': recently_seen
    })
