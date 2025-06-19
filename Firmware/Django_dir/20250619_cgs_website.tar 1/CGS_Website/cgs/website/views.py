from django.shortcuts import render, get_object_or_404, redirect
from django.contrib.auth.decorators import login_required, user_passes_test
from django.http import JsonResponse
from rest_framework.decorators import api_view
from rest_framework.response import Response
from django.utils import timezone
from .models import StudioESP, StudioDevice, News
import json
from django.utils.timezone import now, timedelta
from django.views.decorators.http import require_POST
from django.views.decorators.csrf import csrf_exempt


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
    StudioESP.objects.filter(last_seen__lt=cutoff).update(last_seen=None)

    esps = StudioESP.objects.all()
    return render(request, 'website/profil.html', {'esps': esps})


@api_view(['GET'])
def control_device(request):
    if not request.user.is_authenticated or not request.user.is_superuser:
        return Response({'error': 'Unauthorized'}, status=403)
    cmd = request.GET.get('cmd')
    return Response({'status': f'Commande {cmd} envoyée'})



@require_POST
@csrf_exempt                       # ← garde-le le temps de tes tests, puis enlève-le
def toggle_esp_status(request, esp_id):
    try:
        esp = StudioESP.objects.get(pk=esp_id)
        esp.state = not esp.state
        esp.save(update_fields=["state"])
        return JsonResponse({"status": "ON" if esp.state else "OFF"})
    except StudioESP.DoesNotExist:
        return JsonResponse({"error": "ESP not found"}, status=404)






@csrf_exempt
def esp_status_api(request):
    name = request.GET.get("name")
    try:
        esp = StudioESP.objects.get(name=name)
    except StudioESP.DoesNotExist:
        return JsonResponse({"error": "not found"}, status=404)

    connected = False
    if esp.last_seen:
        connected = timezone.now() - esp.last_seen < timedelta(seconds=30)

    return JsonResponse({
        "state": "ON" if esp.state else "OFF",
        "connected": connected
    })



@csrf_exempt
def get_state_api(request):
    name = request.GET.get("name")
    if not name:
        return JsonResponse({'error': 'Missing name'}, status=400)
    
    try:
        esp = StudioESP.objects.get(name=name)
    except StudioESP.DoesNotExist:
        return JsonResponse({'error': 'ESP not found'}, status=404)

    # Met à jour l’ESP comme "vu récemment"
    esp.last_seen = timezone.now()
    esp.save()

    return JsonResponse({
        'state': esp.state,  # Booléen simple
        'connected': esp.connected  # propriété booléenne (is_connected)
    })


@login_required
@user_passes_test(lambda u: u.is_superuser)
def toggle_esp_state(request, esp_id):
    if request.method == 'POST':
        esp = get_object_or_404(StudioESP, id=esp_id)
        esp.state = not esp.state  # toggle le booléen
        esp.save()
        return JsonResponse({'state': esp.state})
    return JsonResponse({'error': 'Invalid request'}, status=400)

def reset_all_esp_connection_status():
    cutoff = now() - timedelta(seconds=15)
    StudioESP.objects.filter(last_seen__lt=cutoff).update(last_seen=None)




@require_POST
@csrf_exempt                # en production, remplace par un token partagé
def esp_ping(request):
    """
    Appelée par l’ESP pour dire « je suis en vie ».
    Body (JSON) : { "name": "Supernova" }
    """
    data = json.loads(request.body or "{}")
    name = data.get("name")
    try:
        esp = StudioESP.objects.get(name=name)
    except StudioESP.DoesNotExist:
        return JsonResponse({"error": "unknown esp"}, status=404)

    # met à jour uniquement si l’IP de la requête = IP déclarée
    if request.META["REMOTE_ADDR"] == esp.esp_ip:
        esp.last_seen = timezone.now()
        esp.save(update_fields=["last_seen"])
        return JsonResponse({"ok": True})
    return JsonResponse({"error": "ip mismatch"}, status=400)