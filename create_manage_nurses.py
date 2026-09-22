with open('templates/add_nurse_station.html', 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('Nurse Stations', 'Nurses (HR)')
content = content.replace('add_nurse_station', 'manage_nurses')
content = content.replace('Register and manage operational communication hubs', 'Register and manage hospital nursing staff and NFC cards')

# Replace the form part
import re
form_start = content.find('<form method="POST">')
form_end = content.find('</form>') + 7

new_form = '''<form method="POST">
            {% csrf_token %}
            {% if messages %}
            <div class="mb-3">
                {% for message in messages %}
                <div class="alert alert-{{ message.tags }}">{{ message }}</div>
                {% endfor %}
            </div>
            {% endif %}
            <div class="mb-4">
                <label class="form-label">FULL NAME</label>
                <input type="text" name="full_name" class="form-control" placeholder="e.g. Abebe Kebede" required>
            </div>
            <div class="mb-4">
                <label class="form-label">RFID CARD UID (From Scanner)</label>
                <input type="text" name="rfid_uid" class="form-control" placeholder="e.g. A1B2C3D4" required>
            </div>
            
            <button type="submit" class="btn btn-submit w-100 py-3 mt-2">
                <i class="fas fa-plus-circle me-2"></i> Register New Nurse
            </button>
        </form>'''

content = content[:form_start] + new_form + content[form_end:]

# Replace the table part
table_start = content.find('<table class="custom-table">')
table_end = content.find('</table>') + 8

new_table = '''<table class="custom-table">
                <thead>
                    <tr>
                        <th>ID</th>
                        <th>Name</th>
                        <th>RFID UID</th>
                        <th>Assigned Station(s)</th>
                        <th>Actions</th>
                    </tr>
                </thead>
                <tbody>
                    {% for nurse in nurses %}
                    <tr>
                        <td><span class="status-badge" style="background: var(--accent-blue); color: white;">{{ nurse.nurse_id }}</span></td>
                        <td class="fw-bold">{{ nurse.full_name }}</td>
                        <td><span class="text-muted small font-monospace">{{ nurse.rfid_uid }}</span></td>
                        <td>
                            {% for station in nurse.stations.all %}
                            <span class="badge bg-secondary">{{ station.station_name }}</span>
                            {% empty %}
                            <span class="badge bg-light text-dark border">Unassigned (Available)</span>
                            {% endfor %}
                        </td>
                        <td>
                            <a href="{% url 'delete_nurse' nurse.id %}" class="text-danger text-decoration-none small fw-bold" onclick="return confirm('Remove this nurse?');">
                                <i class="fas fa-trash-alt me-1"></i> Remove
                            </a>
                        </td>
                    </tr>
                    {% empty %}
                    <tr><td colspan="5" class="text-center py-5 text-muted">No nurses registered yet</td></tr>
                    {% endfor %}
                </tbody>
            </table>'''

content = content[:table_start] + new_table + content[table_end:]

# Add sidebar link for HR
# We need to replace the active state
content = content.replace('class="nav-link active"', 'class="nav-link"')

nav_addition = '''
        <a href="{% url 'add_nurse_station' %}" class="nav-link">
            <i class="fas fa-user-nurse"></i> Nurse Stations
        </a>
        <a href="{% url 'manage_nurses' %}" class="nav-link active">
            <i class="fas fa-id-badge"></i> Nurses (HR)
        </a>'''

content = content.replace('''<a href="{% url 'add_nurse_station' %}" class="nav-link">
            <i class="fas fa-user-nurse"></i> Nurse Stations
        </a>''', nav_addition)


with open('templates/manage_nurses.html', 'w', encoding='utf-8') as f:
    f.write(content)
