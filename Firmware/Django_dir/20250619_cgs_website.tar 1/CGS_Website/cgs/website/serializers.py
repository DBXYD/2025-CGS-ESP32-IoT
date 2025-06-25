from rest_framework import serializers
from .models import StudioEspRackDevice, StudioEspDisplayDevice, Equipment

class StudioEspRackDeviceSerializer(serializers.ModelSerializer):
    class Meta:
        model = StudioEspRackDevice
        fields = '__all__'

class StudioEspDisplayDeviceSerializer(serializers.ModelSerializer):
    class Meta:
        model = StudioEspDisplayDevice
        fields = '__all__'

class EquipmentSerializer(serializers.ModelSerializer):
    # pour afficher le libellé lisible de la catégorie
    category_display = serializers.CharField(source='get_category_display', read_only=True)

    class Meta:
        model = Equipment
        fields = ('id', 'brand', 'model', 'category_display')
