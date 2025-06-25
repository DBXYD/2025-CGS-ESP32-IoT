from django.contrib import admin
from .models import Studio, StudioESP, StudioEspRackDevice, StudioEspDisplayDevice, News, Calendar, Equipment


class ESPInline(admin.TabularInline):
    model   = StudioESP
    extra   = 0
    fields  = ("role", "ip", "last_seen", "status")
    readonly_fields = ("last_seen",)


class RackDeviceInline(admin.TabularInline):
    model = StudioEspRackDevice
    extra = 0
    fields = ("type", "marque", "modele", "esp_output_id")

@admin.register(Calendar)
class CalendarAdmin(admin.ModelAdmin):
    list_display = ('name', 'google_id')
    search_fields = ('name', 'google_id')

@admin.register(Studio)
class StudioAdmin(admin.ModelAdmin):
    list_display = ("nom", "surface", "etat")
    prepopulated_fields = {"slug": ("nom",)}
    fields = ("nom", "slug", "surface", "panoramic", "vignette", "calendar", "ordre_tri", "etat")
    list_filter = ('calendar',)
    inlines = (ESPInline,)
    search_fields = ("nom",)


@admin.register(StudioESP)
class StudioESPAdmin(admin.ModelAdmin):
    list_display = ("id", "studio", "role", "ip", "status", "last_seen")
    list_filter = ("studio", "role", "status")
    search_fields = ("ip",)


@admin.register(StudioEspRackDevice)
class StudioEspRackDeviceAdmin(admin.ModelAdmin):
    list_display = ("type", "marque", "modele", "esp", "esp_output_id","is_on")
    list_filter = ("type", "esp","is_on")
    search_fields = ("marque", "modele")


@admin.register(StudioEspDisplayDevice)
class StudioEspDisplayDeviceAdmin(admin.ModelAdmin):
    list_display = ("type", "esp","is_on")
    list_filter = ("type", "esp","is_on")


@admin.register(News)
class NewsAdmin(admin.ModelAdmin):
    list_display = ("title", "start_date", "end_date", "price")


@admin.register(Equipment)
class EquipmentAdmin(admin.ModelAdmin):
    list_display   = ('name', 'studio', 'category', 'description', 'order')
    list_filter    = ('studio', 'category')
    search_fields  = ('name', 'description')
    ordering       = ('studio','order','name')
