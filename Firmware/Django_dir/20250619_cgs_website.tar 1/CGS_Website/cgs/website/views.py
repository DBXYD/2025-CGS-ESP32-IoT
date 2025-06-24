from django.shortcuts import render, get_object_or_404, redirect
from django.contrib.auth.decorators import login_required, user_passes_test
from django.http import JsonResponse
from rest_framework.decorators import api_view
from rest_framework.response import Response
from .models import StudioESP, StudioEspRackDevice, StudioEspDisplayDevice, News, Studio
import json
from django.contrib import messages
from django.utils.timezone import now, timedelta
from django.views.decorators.http import require_POST
from django.views.decorators.csrf import csrf_exempt
from .serializers import StudioEspRackDeviceSerializer, StudioEspDisplayDeviceSerializer
from django.db.models import Prefetch
from django.utils import timezone
from django.utils.timezone        import now
from datetime                     import datetime, time, timedelta, timezone as dt_tz
from zoneinfo                     import ZoneInfo
from website.calendar_utils       import calendar_service, CALENDAR_ID

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
def profil_view(request):

    # Pour l’instant page vide :
    return render(request, 'website/profil.html')


def reset_all_esp_connection_status():
    cutoff = now() - timedelta(seconds=15)
    StudioESP.objects.filter(last_seen__lt=cutoff).update(connected=False)

@login_required
@user_passes_test(lambda u: u.is_superuser)
def controle_view(request):
    # 1) on déconnecte tous les ESP dont le ping date de plus de 15 s
    reset_all_esp_connection_status()

    # 2) on récupère ensuite les ESP
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
    
    connected = esp.is_connected
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
    if state is not None:
        esp.status = "is_on" if state else "is_off"

    esp.save(update_fields=["last_seen", "status"])
    return JsonResponse({"ok": True})



####Gestion réservations

PARIS_TZ = ZoneInfo("Europe/Paris")
UTC      = dt_tz.utc

def parse_iso(dt_str: str) -> datetime:
    # transforme "2025-06-24T14:00:00Z" en datetime(…, tzinfo=UTC)
    if dt_str.endswith("Z"):
        dt_str = dt_str[:-1] + "+00:00"
    return datetime.fromisoformat(dt_str).astimezone(UTC)

@login_required
@user_passes_test(lambda u: u.is_superuser)
def gestion_reservations(request):
    # POST-Redirect-Get pour éviter double exécution au rafraîchissement
    if request.method == "POST":
        cleaned = 0

        # 1) Lecture des bornes en heure locale Paris
        start_local = datetime.fromisoformat(request.POST['start']).replace(tzinfo=PARIS_TZ)
        end_local = (datetime.fromisoformat(request.POST['end']) + timedelta(days=1)).replace(tzinfo=PARIS_TZ)

        # 2) Conversion en UTC pour l'API
        start_utc = start_local.astimezone(UTC)
        end_utc = end_local.astimezone(UTC)

        # 3) Récupération et mise à jour des events existants
        events = calendar_service.events().list(
            calendarId=CALENDAR_ID,
            timeMin=start_utc.isoformat(),
            timeMax=end_utc.isoformat(),
            singleEvents=True,
            orderBy='startTime'
        ).execute().get('items', [])

        # -> remplacer les "Sans titre" en "Disponible"
        for ev in events:
            if not ev.get('summary', '').strip():
                ev['summary'] = 'Disponible'
                ev['visibility'] = 'public'
                ev['transparency'] = 'transparent'
                ev['reminders'] = {'useDefault': True}
                calendar_service.events().update(
                    calendarId=CALENDAR_ID,
                    eventId=ev['id'],
                    body=ev
                ).execute()
                cleaned += 1

        # 4) Balayage jour par jour pour combler les trous par blocs d'1h
        current = start_local.date()
        last = (end_local - timedelta(seconds=1)).date()
        while current <= last:
            wd = current.weekday()
            # définir fenêtre locale selon jour
            if wd <= 4:  # Lun–Ven 16h→00h
                loc_start = datetime.combine(current, time(16,0), tzinfo=PARIS_TZ)
                loc_end = datetime.combine(current+timedelta(days=1), time(0,0), tzinfo=PARIS_TZ)
            elif wd == 5:  # Samedi 10h→18h
                loc_start = datetime.combine(current, time(10,0), tzinfo=PARIS_TZ)
                loc_end = datetime.combine(current, time(18,0), tzinfo=PARIS_TZ)
            else:  # Dimanche 13h→21h
                loc_start = datetime.combine(current, time(13,0), tzinfo=PARIS_TZ)
                loc_end = datetime.combine(current, time(21,0), tzinfo=PARIS_TZ)

            # conversion en UTC
            window_start = loc_start.astimezone(UTC)
            window_end = loc_end.astimezone(UTC)

            # events du créneau
            day_events = [
                ev for ev in events
                if 'dateTime' in ev['start']
                and window_start <= parse_iso(ev['start']['dateTime']) < window_end
            ]
            # timeline de base
            times = [window_start]
            for ev in sorted(day_events, key=lambda e: parse_iso(e['start']['dateTime'])):
                times.append(parse_iso(ev['start']['dateTime']))
                times.append(parse_iso(ev['end']['dateTime']))
            times.append(window_end)

            # insertion blocs de 1h
            for a, b in zip(times[::2], times[1::2]):
                gap = b - a
                if gap >= timedelta(hours=1):
                    n_blocks = int(gap / timedelta(hours=1))
                    for i in range(n_blocks):
                        bs = a + timedelta(hours=i)
                        be = bs + timedelta(hours=1)
                        calendar_service.events().insert(
                            calendarId=CALENDAR_ID,
                            body={
                                'summary': 'Disponible',
                                'visibility': 'public',
                                'transparency': 'transparent',
                                'reminders': {'useDefault': True},
                                'start': {'dateTime': bs.isoformat()},
                                'end': {'dateTime': be.isoformat()},
                            }
                        ).execute()
                        cleaned += 1
            current += timedelta(days=1)

        # message de succès et redirect
        messages.success(request, f"Calendrier nettoyé de {cleaned} créneaux “Disponible”.")
        return redirect('gestion_reservations')

    # GET simple
    return render(request, 'website/gestion_reservations.html')
