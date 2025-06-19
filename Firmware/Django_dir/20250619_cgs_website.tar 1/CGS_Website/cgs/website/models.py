from django.db import models

# Create your models here.
from django.db import models
from django.utils import timezone
from datetime import timedelta

class Studio(models.Model):
    nom = models.CharField(max_length=100)
    surface = models.DecimalField(max_digits=6, decimal_places=2)  # Surface en m², par exemple 100.50 m²
    nbr_max_personnes = models.PositiveIntegerField()  # Nombre maximum de personnes pouvant être accueillies
    image = models.ImageField(upload_to='studios/')  # Upload d'images vers le dossier 'studios/'

    def __str__(self):
        return self.nom
    
class StudioDevice(models.Model):
    TYPE_CHOICES = [
        ('ampli', 'Amplificateur'),
        ('baffle guitare', 'Baffle guitare'),
        ('baffle basse', 'Baffle basse'),
        ('batterie', 'Batterie'),
        ### ajoutez d'autres types si besoin
    ]

    studio = models.ForeignKey(Studio, on_delete=models.CASCADE, related_name='devices')
    marque = models.CharField(max_length=100)
    modele = models.CharField(max_length=100)
    numero_serie = models.CharField(max_length=100, unique=True)
    date_achat = models.DateField()
    type_appareil = models.CharField(max_length=50, choices=TYPE_CHOICES)
    duree_garantie_annees = models.PositiveIntegerField()
    
    is_on = models.BooleanField(default=False)  # État de l’appareil
    esp_id = models.CharField(max_length=50, unique=True)  # ID de l'ESP32 associé

    @property
    def date_fin_garantie(self):
        return self.date_achat + timedelta(days=365 * self.duree_garantie_annees)

    def __str__(self):
        return f"{self.marque} {self.modele} - {self.studio.nom}"





class Entretien(models.Model):
    studio_device = models.ForeignKey(StudioDevice, on_delete=models.CASCADE, related_name='entretiens')
    date_entretien = models.DateField()
    description = models.TextField()

    def __str__(self):
        return f"Entretien du {self.date_entretien} pour {self.studio_device.marque} {self.studio_device.modele}"


class News(models.Model):
    title = models.CharField(max_length=50)
    description = models.TextField(blank=True, null=True)
    img = models.ImageField(upload_to='images/', blank=True, null=True)  
    price = models.DecimalField(max_digits=6, decimal_places=2, blank=True, null=True)
    start_date = models.DateField(auto_now_add=True)
    end_date = models.DateField()
    link_insta = models.URLField(max_length=200, blank=True, null=True)
    link_facebook = models.URLField(max_length=200, blank=True, null=True)
    link_ext = models.URLField(max_length=200, blank=True, null=True)

    class Meta:
        verbose_name = "News"
        verbose_name_plural = "News"

    def __str__(self):
        return self.title





### class pour afficher les etats des esp dans les differents studios
class StudioESP(models.Model):
    name = models.CharField(max_length=100, unique=True)
    esp_ip = models.GenericIPAddressField()
    state = models.BooleanField(default=False)  # ON = True, OFF = False
    last_seen = models.DateTimeField(null=True, blank=True)

    def is_connected(self):
        if not self.last_seen:
            return False
        return timezone.now() - self.last_seen < timedelta(seconds=30)

    @property
    def connected(self):
        return self.is_connected()

    def __str__(self):
        return self.name