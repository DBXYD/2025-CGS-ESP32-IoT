from django.contrib import admin
from .models import News, Studio, StudioDevice, Entretien, StudioESP
from django.utils.timezone import now
from datetime import timedelta

class NewsAdmin(admin.ModelAdmin):
    list_display = ('title', 'price', 'start_date', 'end_date')
    ordering = ('title', 'price', 'start_date', 'end_date')
    list_filter = ('price', 'start_date', 'end_date')
    search_fields = ('title',)

@admin.register(StudioESP)
class StudioESPAdmin(admin.ModelAdmin):
    list_display = ('name', 'esp_ip', 'get_status_display', 'connected_display')  # ✅ Utilise une méthode ici
    list_editable = ()  # ❌ pas de champs éditables si on affiche une méthode (pas un champ)

    def get_status_display(self, obj):
        return "ON" if obj.state else "OFF"
    get_status_display.short_description = "Statut"

    def connected_display(self, obj):
        return "✅ Connecté" if obj.connected else "❌ Déconnecté"
    connected_display.short_description = 'Connecté ?'

# Enregistrements classiques
admin.site.register(News, NewsAdmin)
admin.site.register(Studio)
admin.site.register(StudioDevice)
admin.site.register(Entretien)
