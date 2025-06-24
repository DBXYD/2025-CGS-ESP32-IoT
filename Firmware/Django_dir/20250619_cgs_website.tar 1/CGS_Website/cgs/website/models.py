# website/models.py
from django.db import models
from django.utils import timezone
from django.utils.timezone import now, timedelta
from django.utils.text import slugify
from django.contrib.auth.models import User



class MusicGroup(models.Model):
    name = models.CharField(max_length=255)

class News(models.Model):
    title = models.CharField(max_length=50)
    description = models.TextField(blank=True)
    img = models.ImageField(upload_to="images/", blank=True)
    start_date = models.DateField()
    end_date = models.DateField()
    price = models.DecimalField(max_digits=6, decimal_places=2, blank=True, null=True)

    def __str__(self):
        return self.title


class Calendar(models.Model):
    """
    Représente un calendrier (Google ou futur interne).
    Pour l'instant, on laisse google_id vide et on continue
    d'utiliser Google Agenda via settings.CALENDAR_ID.
    À terme, on pourra stocker ici les événements.
    """
    name = models.CharField("Nom du calendrier", max_length=100, unique=True)
    google_id = models.CharField(
        "Google Calendar ID",
        max_length=255,
        blank=True,
        null=True,
        help_text="L’ID Google (ex. votre@mail.com)."
    )

    class Meta:
        verbose_name = "Calendrier"
        verbose_name_plural = "Calendriers"

    def __str__(self):
        return self.name

class Event(models.Model):
    calendar   = models.ForeignKey(Calendar, on_delete=models.CASCADE)
    summary    = models.CharField(max_length=255)
    start_dt   = models.DateTimeField()
    end_dt     = models.DateTimeField()
    is_available = models.BooleanField(default=False)



class EnrollmentUserMusicGroup(models.Model):
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    music_group = models.ForeignKey(MusicGroup, on_delete=models.CASCADE)

class Studio(models.Model):
    nom = models.CharField(max_length=100, unique=True)
    surface = models.PositiveIntegerField(null=True, blank=True)
    etat = models.CharField(max_length=4, default='free')
    panoramic = models.ImageField(upload_to='360/', blank=True)
    vignette = models.ImageField(upload_to='studios/', blank=True)
    calendar = models.ForeignKey(
        Calendar,
        on_delete=models.SET_NULL,
        blank=True,
        null=True,
        related_name="studios",
        verbose_name="Calendrier associé",
        help_text="Choisir le calendrier lié à ce studio."
    )
    ordre_tri = models.SmallIntegerField(default=0)
    slug = models.SlugField(unique=True, blank=True)

    class Meta:
        ordering = ["ordre_tri"]

    def save(self, *args, **kwargs):
        if not self.slug:
            self.slug = slugify(self.nom)
        super().save(*args, **kwargs)
    def __str__(self):  
        return self.nom
    




# models.py
class StudioESP(models.Model):
    studio = models.ForeignKey(Studio, on_delete=models.SET_NULL, null=True, blank=True)
    role = models.CharField(max_length=30, default='other')
    ip = models.GenericIPAddressField()
    last_seen = models.DateTimeField(null=True, blank=True)
    status    = models.CharField(max_length=16, choices=(("is_on","On"),("is_off","Off")))
    name = models.CharField(max_length=50, unique=True, null=True, blank=True)  # ← AJOUT
    
    @property
    def is_connected(self):
        if not self.last_seen:
            return False
        return (now() - self.last_seen) < timedelta(seconds=15)
    
    def __str__(self):
        return self.name or f"ESP-{self.id}"


class StudioEspRackDevice(models.Model):
    esp = models.ForeignKey(StudioESP, on_delete=models.SET_NULL, null=True, blank=True)
    type = models.CharField(max_length=20, default='ampli')
    marque = models.CharField(max_length=100)
    modele = models.CharField(max_length=100)
    esp_output_id = models.IntegerField()
    is_on = models.BooleanField(default=False)  # ← ajouté ici

class StudioEspDisplayDevice(models.Model):
    esp = models.ForeignKey(StudioESP, on_delete=models.SET_NULL, null=True, blank=True)
    type = models.CharField(max_length=20, default='display')
    is_on = models.BooleanField(default=False)  # ← ajouté ici aussi
