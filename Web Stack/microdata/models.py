from django.db import models
from django.conf import settings

# Create your models here.

class Appliance(models.Model):
    serial = models.IntegerField(unique=True)
    name = models.CharField(max_length=50, unique=True)

    def __unicode__(self):
        return self.name

class Device(models.Model):
    owner = models.ForeignKey(settings.AUTH_USER_MODEL)
    serial = models.IntegerField(unique=True)
    name = models.CharField(max_length=30)
    zipcode = models.CharField(max_length=5)
    private = models.BooleanField(default=False)
    
    def __unicode__(self):
        return self.name    

class Event(models.Model):
    device = models.ForeignKey(Device, related_name='events')
    event_code = models.IntegerField(blank=True, null=True)
    appliance = models.ForeignKey(Appliance, related_name='appliance', blank=True, null=True)
    timestamp = models.PositiveIntegerField(help_text='13 digits, millisecond resolution')
    wattage = models.FloatField()

    def __unicode__(self):
        return str(self.timestamp)

