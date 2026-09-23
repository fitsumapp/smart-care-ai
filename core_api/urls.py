from django.urls import path
from . import views

urlpatterns = [
    path('', views.login_view, name='login_view'),
    path('login/', views.login_view, name='login_page'),
    path('reports/', views.reports_page, name='reports_page'),
    path('reports/export/', views.export_report, name='export_report'),
    path('logout/', views.logout_view, name='logout_view'),
    path('nurse-dashboard/', views.nurse_dashboard, name='nurse_dashboard'),
    path('supervisor-dashboard/', views.supervisor_dashboard, name='supervisor_dashboard'),
    path('supervisor/add-station/', views.add_nurse_station, name='add_nurse_station'),
    path('supervisor/manage-nurses/', views.manage_nurses, name='manage_nurses'),
    path('supervisor/delete-nurse/<int:nurse_id>/', views.delete_nurse, name='delete_nurse'),
    # --- New: Added for ER/OR mapping ---
    path('supervisor/manage-special-rooms/', views.manage_special_rooms, name='manage_special_rooms'),
    
    path('export/csv/', views.export_calls_csv, name='export_csv'),
    path('api/calls/', views.call_log_api, name='call_log_api'),
    path('api/calls/clear/', views.clear_call_api, name='clear_call_api'),
    path('api/acknowledge_nfc/', views.acknowledge_nfc_api, name='acknowledge_nfc_api'),
    path('api/calls/check_reset/', views.check_reset_api, name='check_reset_api'),
    path('api/update-priority/', views.update_priority_ajax, name='update_priority_ajax'),
    path('supervisor/delete-special-room/<int:room_id>/', views.delete_special_room, name='delete_special_room'),
    path('supervisor/edit-special-room/<int:room_id>/', views.edit_special_room, name='edit_special_room'),
    path('supervisor/settings/', views.settings_page, name='settings_page'),
    path('supervisor/update-profile/', views.update_profile, name='update_profile'),
    path('supervisor/delete-station/<int:station_id>/', views.delete_nurse_station, name='delete_nurse_station'),
    path('supervisor/edit-station/<int:station_id>/', views.edit_nurse_station, name='edit_nurse_station'),
    
    # --- New Detailed Analytics ---
    path('supervisor/analytics/stations/', views.station_analytics, name='station_analytics'),
    path('supervisor/analytics/rooms/', views.room_analytics, name='room_analytics'),
    path('supervisor/analytics/nurses/', views.nurse_analytics, name='nurse_analytics'),

    # Heartbeat & Status
    path('api/heartbeat/', views.heartbeat_api, name='heartbeat_api'),
    path('api/system-status/', views.system_status_api, name='system_status_api'),
    path('api/diagnostics/action/', views.diagnostic_action_api, name='diagnostic_action_api'),
    path('api/tts/', views.tts_api, name='tts_api'),
    path('supervisor/system-logs/', views.system_logs_view, name='system_logs'),
]
