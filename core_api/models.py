from django.db import models
from django.contrib.auth.models import User
from django.utils import timezone
from django.core.exceptions import ValidationError
import threading
import time

class Nurse(models.Model):
    full_name = models.CharField(max_length=100)
    phone_number = models.CharField(max_length=20, blank=True, null=True)
    email = models.EmailField(blank=True, null=True)
    nurse_id = models.CharField(max_length=2, unique=True)
    rfid_uid = models.CharField(max_length=50, unique=True, help_text="NFC Card UID")
    created_at = models.DateTimeField(auto_now_add=True)

    def clean(self):
        super().clean()
        if self.full_name:
            self.full_name = self.full_name.strip()
        if self.nurse_id:
            self.nurse_id = self.nurse_id.strip().upper()
            existing = Nurse.objects.filter(nurse_id__iexact=self.nurse_id)
            if self.pk:
                existing = existing.exclude(pk=self.pk)
            if existing.exists():
                raise ValidationError({'nurse_id': f"Nurse ID '{self.nurse_id}' is already registered to {existing.first().full_name}."})

        if self.rfid_uid:
            self.rfid_uid = self.rfid_uid.strip().upper()
            existing_rfid = Nurse.objects.filter(rfid_uid__iexact=self.rfid_uid)
            if self.pk:
                existing_rfid = existing_rfid.exclude(pk=self.pk)
            if existing_rfid.exists():
                raise ValidationError({'rfid_uid': f"NFC Card UID '{self.rfid_uid}' is already assigned to {existing_rfid.first().full_name} (ID: {existing_rfid.first().nurse_id})."})

        if self.phone_number:
            self.phone_number = self.phone_number.strip()
            existing_phone = Nurse.objects.filter(phone_number=self.phone_number)
            if self.pk:
                existing_phone = existing_phone.exclude(pk=self.pk)
            if existing_phone.exists():
                raise ValidationError({'phone_number': f"Phone number '{self.phone_number}' is already registered to {existing_phone.first().full_name}."})

        if self.email:
            self.email = self.email.strip().lower()
            existing_email = Nurse.objects.filter(email__iexact=self.email)
            if self.pk:
                existing_email = existing_email.exclude(pk=self.pk)
            if existing_email.exists():
                raise ValidationError({'email': f"Email '{self.email}' is already registered to {existing_email.first().full_name}."})

    def save(self, *args, **kwargs):
        self.full_clean()
        super().save(*args, **kwargs)

    def __str__(self):
        return f"{self.full_name} ({self.nurse_id})"

class NurseStation(models.Model):
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    station_name = models.CharField(max_length=100, unique=True)
    start_room = models.IntegerField(default=1)
    end_room = models.IntegerField(default=20)
    nurses = models.ManyToManyField(Nurse, blank=True, related_name='stations')

    def clean(self):
        super().clean()
        if self.station_name:
            self.station_name = self.station_name.strip()
            existing_name = NurseStation.objects.filter(station_name__iexact=self.station_name)
            if self.pk:
                existing_name = existing_name.exclude(pk=self.pk)
            if existing_name.exists():
                raise ValidationError({'station_name': f"A Nurse Station with the name '{self.station_name}' already exists (names must be unique)."})

        if self.start_room is None or self.start_room < 1:
            raise ValidationError({'start_room': "Start room must be a positive number (>= 1)."})

        if self.end_room is None or self.end_room < 1:
            raise ValidationError({'end_room': "End room must be a positive number (>= 1)."})

        if self.start_room > self.end_room:
            raise ValidationError({
                'end_room': f"End room ({self.end_room}) must be greater than or equal to start room ({self.start_room})."
            })

        # Overlapping Room Range Prevention
        overlapping = NurseStation.objects.filter(
            start_room__lte=self.end_room,
            end_room__gte=self.start_room
        )
        if self.pk:
            overlapping = overlapping.exclude(pk=self.pk)

        if overlapping.exists():
            conflict = overlapping.first()
            raise ValidationError({
                'start_room': (
                    f"Room range {self.start_room}–{self.end_room} conflicts with existing station "
                    f"'{conflict.station_name}' (covering rooms {conflict.start_room}–{conflict.end_room}). "
                    f"Each room can only belong to one Nurse Station!"
                )
            })

    def save(self, *args, **kwargs):
        self.full_clean()
        super().save(*args, **kwargs)

    def __str__(self):
        return self.station_name

# --- Model for Special Rooms (ER, OR) ---
class SpecialRoom(models.Model):
    # Identifiers
    emergency_id = models.CharField(max_length=50, unique=True, help_text="e.g. ER1")
    regular_id = models.CharField(max_length=50, unique=True, help_text="e.g. RR1")
    
    room_name_display = models.CharField(max_length=100, unique=True, help_text="Display name on dashboard (e.g. OR BED 1)")

    # Multiple nurse stations called during Emergency (Many-to-Many)
    emergency_stations = models.ManyToManyField(NurseStation, related_name='emergency_mapping', blank=True)
    
    # Specific nurse stations called during Regular (Many-to-Many)
    regular_stations = models.ManyToManyField(NurseStation, related_name='regular_mapping', blank=True)

    created_at = models.DateTimeField(auto_now_add=True)

    def clean(self):
        super().clean()
        if self.emergency_id:
            self.emergency_id = self.emergency_id.strip().upper()
        if self.regular_id:
            self.regular_id = self.regular_id.strip().upper()
        if self.room_name_display:
            self.room_name_display = self.room_name_display.strip()

        # 1. Emergency ID and Regular ID cannot be identical within the same room
        if self.emergency_id and self.regular_id and self.emergency_id == self.regular_id:
            raise ValidationError({
                'regular_id': "Regular ID must be different from Emergency ID (they cannot share the same code)."
            })

        # 2. Check collisions across other Special Rooms (both emergency_id and regular_id)
        other_rooms = SpecialRoom.objects.all()
        if self.pk:
            other_rooms = other_rooms.exclude(pk=self.pk)

        if self.emergency_id:
            dup_e = other_rooms.filter(models.Q(emergency_id__iexact=self.emergency_id) | models.Q(regular_id__iexact=self.emergency_id)).first()
            if dup_e:
                raise ValidationError({
                    'emergency_id': f"ID '{self.emergency_id}' is already used by '{dup_e.room_name_display}' (as {dup_e.emergency_id}/{dup_e.regular_id})."
                })

        if self.regular_id:
            dup_r = other_rooms.filter(models.Q(emergency_id__iexact=self.regular_id) | models.Q(regular_id__iexact=self.regular_id)).first()
            if dup_r:
                raise ValidationError({
                    'regular_id': f"ID '{self.regular_id}' is already used by '{dup_r.room_name_display}' (as {dup_r.emergency_id}/{dup_r.regular_id})."
                })

        if self.room_name_display:
            dup_name = other_rooms.filter(room_name_display__iexact=self.room_name_display).first()
            if dup_name:
                raise ValidationError({
                    'room_name_display': f"Special Room name '{self.room_name_display}' already exists (names must be unique)."
                })

        # 3. Prevent Special Room IDs from being a plain integer overlapping standard station rooms
        for test_id, field_name in [(self.emergency_id, 'emergency_id'), (self.regular_id, 'regular_id')]:
            if test_id and test_id.isdigit():
                num = int(test_id)
                overlapping_st = NurseStation.objects.filter(start_room__lte=num, end_room__gte=num).first()
                if overlapping_st:
                    raise ValidationError({
                        field_name: (
                            f"ID '{test_id}' cannot be a plain number because Room {num} "
                            f"is covered by '{overlapping_st.station_name}'. Use a code like ER{num}, OR{num}."
                        )
                    })

    def save(self, *args, **kwargs):
        self.full_clean()
        super().save(*args, **kwargs)

    def __str__(self):
        return f"{self.room_name_display} ({self.emergency_id}/{self.regular_id})"

class PatientStatus(models.Model):
    PRIORITY_CHOICES = [
        ('1', 'Critical (High Priority)'),
        ('2', 'Urgent'),
        ('3', 'Normal'),
    ]
    room_number = models.IntegerField(unique=True)
    priority_level = models.CharField(max_length=1, choices=PRIORITY_CHOICES, default='3')
    last_updated = models.DateTimeField(auto_now=True)

    def clean(self):
        super().clean()
        if self.room_number is None or self.room_number < 1:
            raise ValidationError({'room_number': "Room number must be a positive integer (>= 1)."})

        # Room must belong to a registered Nurse Station
        if NurseStation.objects.exists():
            matching_st = NurseStation.objects.filter(
                start_room__lte=self.room_number,
                end_room__gte=self.room_number
            ).first()
            if not matching_st:
                raise ValidationError({
                    'room_number': (
                        f"Room {self.room_number} does not belong to any registered Nurse Station! "
                        f"Please assign Room {self.room_number} to a Nurse Station first."
                    )
                })

    def save(self, *args, **kwargs):
        self.full_clean()
        super().save(*args, **kwargs)

    def __str__(self):
        return f"Room {self.room_number} - Priority {self.priority_level}"

class SystemSettings(models.Model):
    twilio_sid = models.CharField(max_length=100, blank=True, null=True)
    twilio_auth_token = models.CharField(max_length=100, blank=True, null=True)
    twilio_from_number = models.CharField(max_length=20, blank=True, null=True)
    notification_phone_number = models.CharField(max_length=20, blank=True, null=True)
    escalation_delay_seconds = models.IntegerField(default=30)

    def clean(self):
        super().clean()
        existing = SystemSettings.objects.all()
        if self.pk:
            existing = existing.exclude(pk=self.pk)
        if existing.exists():
            raise ValidationError("Only one SystemSettings instance can exist in the system.")

    def save(self, *args, **kwargs):
        self.full_clean()
        super().save(*args, **kwargs)

    def __cloning_singleton__(self):
        return SystemSettings.objects.get_or_create(id=1)[0]

    def __str__(self):
        return "System Settings"

class AICallLog(models.Model):
    CALL_TYPES = [
        ('Normal', 'Normal'),
        ('Urgent', 'Urgent'),
        ('Critical', 'Critical'),
        ('Emergency', 'Emergency'),
        ('Regular', 'Regular'),
        ('AI_FALL', 'AI Fall Detected'),
        ('AI_TREMOR', 'AI Tremor / Seizure'),
    ]
    room_number = models.CharField(max_length=50)
    bed_number = models.CharField(max_length=50, default='General')
    call_type = models.CharField(max_length=30, choices=CALL_TYPES, default='Normal')
    location_type = models.CharField(max_length=50, default='ROOM', help_text="e.g. ROOM, CORRIDOR, STAIRS, WAITING_AREA")
    priority = models.IntegerField(default=1) # Normal=1, Urgent=2, Critical=3
    notes = models.CharField(max_length=255, blank=True, null=True)
    
    is_active = models.BooleanField(default=True)
    is_acknowledged = models.BooleanField(default=False) # When nurse presses Enter
    is_notified = models.BooleanField(default=False)
    is_special_call = models.BooleanField(default=False) # If it's an ER/OR call
    is_escalated = models.BooleanField(default=False) # If SMS was sent
    
    stations = models.ManyToManyField(NurseStation, related_name='calls')
    responded_by = models.ForeignKey(Nurse, null=True, blank=True, on_delete=models.SET_NULL, related_name='responded_calls')
    
    created_at = models.DateTimeField(auto_now_add=True)
    acknowledged_at = models.DateTimeField(null=True, blank=True)
    arrived_at = models.DateTimeField(null=True, blank=True)
    cleared_at = models.DateTimeField(null=True, blank=True)
 
    updated_at = models.DateTimeField(auto_now=True)

    def get_acknowledgment_time(self):
        """Time from call to nurse acknowledgment (seconds)"""
        if self.acknowledged_at and self.created_at:
            delta = self.acknowledged_at - self.created_at
            return int(delta.total_seconds())
        return None

    def get_service_time(self):
        """Time from acknowledgment to service completion (seconds)"""
        if self.cleared_at and self.acknowledged_at:
            delta = self.cleared_at - self.acknowledged_at
            return int(delta.total_seconds())
        return None

    def get_total_response_time(self):
        """Total duration of the call (seconds)"""
        if self.cleared_at and self.created_at:
            delta = self.cleared_at - self.created_at
            return int(delta.total_seconds())
        return None

    def __str__(self):
        return f"Room {self.room_number} ({self.bed_number}) - {self.created_at}"


