"""
URL configuration for ai_nurse_server project.

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/6.0/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""
from django.contrib import admin
from django.urls import path, include
from django.conf import settings
from django.conf.urls.static import static

from core_api import views as core_views

urlpatterns = [
    # 🌟 Unified Single Login Portal for Admins & Nurses
    path('admin/login/', core_views.unified_login_view, name='admin_login'),

    # Custom Native Admin Views (Supervisor & Analytics features inside Admin)
    path('admin/reports/', core_views.reports_page, name='admin_reports'),
    path('admin/analytics/stations/', core_views.station_analytics, name='admin_station_analytics'),
    path('admin/analytics/rooms/', core_views.room_analytics, name='admin_room_analytics'),
    path('admin/analytics/nurses/', core_views.nurse_analytics, name='admin_nurse_analytics'),
    path('admin/system-logs/', core_views.system_logs_view, name='admin_system_logs'),
    path('admin/settings/', core_views.settings_page, name='admin_settings'),

    # Admin interface
    path('admin/', admin.site.urls),
    
    # Include all routes from core_api.urls
    path('', include('core_api.urls')),
]

if settings.DEBUG:
    urlpatterns += static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
    urlpatterns += static(settings.STATIC_URL, document_root=settings.STATICFILES_DIRS[0] if settings.STATICFILES_DIRS else settings.STATIC_ROOT)

