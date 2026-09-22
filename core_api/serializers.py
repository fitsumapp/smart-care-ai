from rest_framework import serializers
from .models import AICallLog

class AICallLogSerializer(serializers.ModelSerializer):
    class Meta:
        model = AICallLog
        fields = '__all__'
