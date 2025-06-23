
from rest_framework import serializers
from .models import StudioEspRackDevice, StudioEspDisplayDevice

class StudioEspRackDeviceSerializer(serializers.ModelSerializer):
    class Meta:
        model = StudioEspRackDevice
        fields = '__all__'

class StudioEspDisplayDeviceSerializer(serializers.ModelSerializer):
    class Meta:
        model = StudioEspDisplayDevice
        fields = '__all__'
