import glob
import os
import re

# Flexible regex to match the Nurses (HR) link regardless of indentation
search_pattern = r'(<a href="{% url \'manage_nurses\' %}" class="nav-link \{% if request\.resolver_match\.url_name == \'manage_nurses\' %\}active\{% endif %\}">\s*<i class="fas fa-id-badge"></i> Nurses \(HR\)\s*</a>)'

replacement_template = r'\1\n        <a href="{% url \'system_logs\' %}" class="nav-link {% if request.resolver_match.url_name == \'system_logs\' %}active{% endif %}">\n            <i class="fas fa-terminal"></i> System Logs\n        </a>'

# Templates directory
templates_path = r'c:\ai_nurse_system\templates\*.html'

for filepath in glob.glob(templates_path):
    if 'system_logs.html' in filepath:
        continue
        
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if '{% url \'manage_nurses\' %}' in content:
        # Use regex to replace
        new_content = re.sub(search_pattern, replacement_template, content)
        if new_content != content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Updated {os.path.basename(filepath)}")
        else:
            print(f"Regex match failed in {os.path.basename(filepath)}")
    else:
        # print(f"Link not found in {os.path.basename(filepath)}")
        pass
