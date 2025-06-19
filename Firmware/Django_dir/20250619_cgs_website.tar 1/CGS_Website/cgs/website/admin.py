from django.contrib import admin
from .models import News, Studio, StudioDevice, Entretien,ESPCommand, StudioESP

# Register your models here.

class NewsAdmin(admin.ModelAdmin):
    list_display = ('title', 'price', 'start_date', 'end_date')
    ordering = ('title', 'price', 'start_date', 'end_date')
    list_filter = ('price','start_date', 'end_date')
    search_fields = ('title',)

### pour la commande des esp32 dans les studio, par les admins
@admin.register(StudioESP)
class StudioESPAdmin(admin.ModelAdmin):
    list_display = ('name', 'esp_ip', 'status', 'connected')
    list_editable = ('status', 'connected')


# Enregistrer le modèle avec la configuration de l'admin
admin.site.register(News, NewsAdmin)

admin.site.register(Studio)
admin.site.register(StudioDevice)
admin.site.register(Entretien)
admin.site.register(ESPCommand)



