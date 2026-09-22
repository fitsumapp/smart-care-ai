import glob
import os
import re

# በስህተት የገቡትን (Backslashes) እና ድግግሞሾችን ለማስተካከል
# 1. Broken pattern with backslashes
broken_pattern = r"\\'system_logs\\'"
fixed_str = "'system_logs'"

templates_path = r'c:\ai_nurse_system\templates\*.html'

for filepath in glob.glob(templates_path):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if "\\'system_logs\\'" in content:
        # Fix backslashes
        new_content = content.replace("\\'system_logs\\'", "'system_logs'")
        
        # ደግሞ የገባ (Duplicate) ካለ አንድ ብቻ እንዲቀር ማድረግ
        duplicate_block = '''        <a href="{% url 'system_logs' %}" class="nav-link {% if request.resolver_match.url_name == 'system_logs' %}active{% endif %}">
            <i class="fas fa-terminal"></i> System Logs
        </a>'''
        
        # Count occurrences
        count = new_content.count(duplicate_block)
        if count > 1:
            # Replace all and then add back one
            new_content = new_content.replace(duplicate_block, "")
            # Find the end of Nurses (HR) block to insert it once
            nurse_hr_block = '''        <a href="{% url 'manage_nurses' %}" class="nav-link {% if request.resolver_match.url_name == 'manage_nurses' %}active{% endif %}">
            <i class="fas fa-id-badge"></i> Nurses (HR)
        </a>'''
            if nurse_hr_block in new_content:
                new_content = new_content.replace(nurse_hr_block, nurse_hr_block + "\n" + duplicate_block)

        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Fixed {os.path.basename(filepath)}")
