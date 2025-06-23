from django.shortcuts import render, get_object_or_404, redirect
from django.contrib.auth.decorators import login_required, user_passes_test
from django.http import JsonResponse
from rest_framework.decorators import api_view
from rest_framework.response import Response
from django.utils import timezone
from .models import StudioESP, StudioEspRackDevice, StudioEspDisplayDevice, News, Studio
import json
from django.utils.timezone import now, timedelta
from django.views.decorators.http import require_POST
from django.views.decorators.csrf import csrf_exempt
from .serializers import StudioEspRackDeviceSerializer, StudioEspDisplayDeviceSerializer
from django.db.models import Prefetch


def index(request):
    upcoming_news = (
        News.objects.filter(end_date__gte=timezone.now())
        .order_by('end_date')
    )[:3]
    return render(request, 'website/index.html', {'upcoming_news': upcoming_news})


def bar(request):
    return render(request, 'website/bar.html')

def booking(request):
    studio_slug = request.GET.get("studio")
    studio = Studio.objects.filter(slug=studio_slug).first() if studio_slug else None
    return render(request, 'website/booking.html', {
        'studio': studio
    })


def contact(request):
    return render(request, 'website/contact.html')

def cgu(request):
    return render(request, 'website/cgu.html')



@api_view(['GET'])
def studio_devices(request, slug):
    studio = get_object_or_404(Studio, slug=slug)
    esps = StudioESP.objects.filter(studio=studio)

    rack_devices = StudioEspRackDevice.objects.filter(esp__in=esps)
    display_devices = StudioEspDisplayDevice.objects.filter(esp__in=esps)

    rack_serializer = StudioEspRackDeviceSerializer(rack_devices, many=True)
    display_serializer = StudioEspDisplayDeviceSerializer(display_devices, many=True)

    return Response({
        "rack_devices": rack_serializer.data,
        "display_devices": display_serializer.data
    })



# ----- PAGE HTML -----
def studios(request):
    studios = Studio.objects.order_by("ordre_tri")
    return render(request, "website/studios.html", {"studios": studios})



@login_required
def control_panel(request):
    if request.method == "POST":
        device_type = request.POST.get("device_type")
        device_id = request.POST.get("device_id")

        if device_type == "rack":
            device = StudioEspRackDevice.objects.get(id=device_id)
        elif device_type == "display":
            device = StudioEspDisplayDevice.objects.get(id=device_id)
        else:
            return redirect('control_panel')  # sécurité

        device.is_on = not device.is_on
        device.save()
        return redirect('control_panel')

    rack_devices = StudioEspRackDevice.objects.all()
    display_devices = StudioEspDisplayDevice.objects.all()

    return render(request, 'website/control_panel.html', {
        'rack_devices': rack_devices,
        'display_devices': display_devices,
    })


@login_required
def gestion_reservations(request):
    # Si vous avez un modèle Reservation lié à l’utilisateur :
    # reservations = Reservation.objects.filter(user=request.user)
    # return render(request, 'website/gestion_reservations.html', {'reservations': reservations})

    # Pour l’instant page vide :
    return render(request, 'website/gestion_reservation.html')


@login_required
def profil_view(request):
    # Si vous avez un modèle Reservation lié à l’utilisateur :
    # reservations = Reservation.objects.filter(user=request.user)
    # return render(request, 'website/gestion_reservations.html', {'reservations': reservations})

    # Pour l’instant page vide :
    return render(request, 'website/profil.html')


@login_required
def controle_view(request):
    if not request.user.is_superuser:
        return render(request, 'website/controle.html')

    esps = StudioESP.objects.prefetch_related(
        Prefetch('studioesprackdevice_set'),
        Prefetch('studioespdisplaydevice_set'),
    )
    return render(request, 'website/controle.html', {'esps': esps})



@api_view(['GET'])
def control_device(request):
    if not request.user.is_authenticated or not request.user.is_superuser:
        return Response({'error': 'Unauthorized'}, status=403)
    cmd = request.GET.get('cmd')
    return Response({'status': f'Commande {cmd} envoyée'})



@require_POST
@csrf_exempt
def toggle_esp_status(request, esp_id):
    try:
        esp = StudioESP.objects.get(pk=esp_id)
        if esp.status == "is_on":
            esp.status = "is_off"
        else:
            esp.status = "is_on"
        esp.save(update_fields=["status"])
        return JsonResponse({"status": "pending"})   # aucune certitude encore
    except StudioESP.DoesNotExist:
        return JsonResponse({"error": "ESP not found"}, status=404)







@csrf_exempt
def esp_status_api(request):
    # prise en charge ?id= ou ?name=
    esp_id  = request.GET.get("id")
    name    = request.GET.get("name")

    if not esp_id and not name:
        return JsonResponse({"error": "missing id or name"}, status=400)

    try:
        esp = StudioESP.objects.get(pk=esp_id) if esp_id else StudioESP.objects.get(name=name)
    except StudioESP.DoesNotExist:
        return JsonResponse({"error": "not found"}, status=404)
    
    connected = esp.connected
    # ton champ s'appelle `status`, par exemple 'is_on' ou 'is_off'
    is_on = (esp.status == "is_on")
    return JsonResponse({
        "state": "ON" if is_on else "OFF",
         "connected": connected,
         "last_seen": int(esp.last_seen.timestamp() * 1000)
                      if esp.last_seen else None
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
    esp.save(update_fields=['last_seen'])

    is_on = (esp.status == "is_on")
    return JsonResponse({
        'state': is_on,  # Booléen simple
        'connected': esp.connected  # propriété booléenne (is_connected)
    })


@login_required
@user_passes_test(lambda u: u.is_superuser)
def toggle_esp_state(request, esp_id):
    if request.method == 'POST':
        esp = get_object_or_404(StudioESP, id=esp_id)
        esp.status = 'is_off' if esp.status == 'is_on' else 'is_on'
        esp.save(update_fields=['status'])
        return JsonResponse({'state': esp.status == 'is_on'})
    return JsonResponse({'error': 'Invalid request'}, status=400)

def reset_all_esp_connection_status():
    cutoff = now() - timedelta(seconds=15)
    StudioESP.objects.filter(last_seen__lt=cutoff).update(last_seen=None)



@require_POST
@csrf_exempt
def esp_ping(request):
    """
    Body JSON : {"name":"Rainbow", "state": true/false}
    """
    data  = json.loads(request.body or "{}")
    name  = data.get("name")
    state = data.get("state")  # bool ou None

    try:
        esp = StudioESP.objects.get(name=name)
    except StudioESP.DoesNotExist:
        return JsonResponse({"error": "unknown esp"}, status=404)

    # 2) vérification IP
    if request.META["REMOTE_ADDR"] != esp.ip:
        return JsonResponse({"error": "ip mismatch"}, status=400)

    # 3) mise à jour
    esp.last_seen = timezone.now()
    esp.connected = True
    if state is not None:
        esp.status = "is_on" if state else "is_off"

    esp.save(update_fields=["last_seen", "connected", "status"])
    return JsonResponse({"ok": True})
