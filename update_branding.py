import os
import re

templates_dir = r'c:\ai_nurse_system\templates'

# Mapping of old names to new names
replacements = {
    'SMART<span>CARE</span>': 'MedPulse<span>AI</span>',
    'SMART CARE <span style="color: var(--accent-blue);">AI</span>': 'MedPulse <span style="color: var(--accent-blue);">AI</span>',
    'AI BASED NURSE CALLING SYSTEM': 'INTELLIGENT NURSE CALL SYSTEM',
    'AI Nurse Pro': 'MedPulse Smart-Care AI',
}

for filename in os.listdir(templates_dir):
    if filename.endswith('.html'):
        filepath = os.path.join(templates_dir, filename)
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_content = content
        for old, new in replacements.items():
            content = content.replace(old, new)
        
        if content != original_content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Updated branding in {filename}")
