# website/models.py
from django.db import models
from django.utils import timezone
from datetime import timedelta
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


class EnrollmentUserMusicGroup(models.Model):
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    music_group = models.ForeignKey(MusicGroup, on_delete=models.CASCADE)

class Studio(models.Model):
    nom = models.CharField(max_length=100, unique=True)
    surface = models.PositiveIntegerField(null=True, blank=True)
    etat = models.CharField(max_length=4, default='free')
    panoramic = models.ImageField(upload_to='360/', blank=True)
    vignette = models.ImageField(upload_to='studios/', blank=True)
    calendar = models.CharField(max_length=255, blank=True)
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
    status = models.CharField(max_length=20, default='is_off')
    name = models.CharField(max_length=50, unique=True, null=True, blank=True)  # ← AJOUT
    connected = models.BooleanField(default=False)
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
