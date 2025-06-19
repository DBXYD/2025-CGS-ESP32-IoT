from django.apps import AppConfig
from django.utils.timezone import now
from datetime import timedelta

class WebsiteConfig(AppConfig):
    default_auto_field = 'django.db.models.BigAutoField'
    name = 'website'

    def ready(self):
        from django.db.models.signals import post_migrate
        from django.dispatch import receiver

        @receiver(post_migrate)
        def reset_all_esp_connection_status(sender, **kwargs):
            from .models import StudioESP
            cutoff = now() - timedelta(seconds=15)
            StudioESP.objects.filter(last_seen__lt=cutoff).update(last_seen=None)
