import glob

search_str = '''<a href="{% url 'add_nurse_station' %}" class="nav-link">
            <i class="fas fa-user-nurse"></i> Nurse Stations
        </a>'''

search_str_active = '''<a href="{% url 'add_nurse_station' %}" class="nav-link active">
            <i class="fas fa-user-nurse"></i> Nurse Stations
        </a>'''

replacement_str = '''<a href="{% url 'add_nurse_station' %}" class="nav-link">
            <i class="fas fa-user-nurse"></i> Nurse Stations
        </a>
        <a href="{% url 'manage_nurses' %}" class="nav-link">
            <i class="fas fa-id-badge"></i> Nurses (HR)
        </a>'''

replacement_str_active = '''<a href="{% url 'add_nurse_station' %}" class="nav-link active">
            <i class="fas fa-user-nurse"></i> Nurse Stations
        </a>
        <a href="{% url 'manage_nurses' %}" class="nav-link">
            <i class="fas fa-id-badge"></i> Nurses (HR)
        </a>'''

for filepath in glob.glob('templates/*.html'):
    if 'manage_nurses.html' in filepath:
        continue # Already handled this one

    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if search_str in content:
        content = content.replace(search_str, replacement_str)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f'Updated {filepath}')
    elif search_str_active in content:
        content = content.replace(search_str_active, replacement_str_active)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f'Updated {filepath} (active)')
