import os
import re

templates_dir = r'c:\ai_nurse_system\templates'
footer_html = """
    <footer class="footer mt-5 pb-4">
        <div class="container-fluid text-center">
            <p class="text-muted small mb-0">© 2026 <span class="fw-bold" style="color: var(--accent-blue);">ACRMA Tech Solution PLC</span>. All Rights Reserved.</p>
        </div>
    </footer>
"""

for filename in os.listdir(templates_dir):
    if filename.endswith('.html'):
        filepath = os.path.join(templates_dir, filename)
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        if 'ACRMA Tech Solution PLC' in content:
            print(f"Footer already exists in {filename}")
            continue
            
        # Try to insert before the last script tag or before </body>
        if '</body>' in content:
            # Look for the last closing div before scripts/end of body
            parts = content.split('</body>')
            new_content = parts[0].rstrip() + footer_html + '\n</body>' + parts[1]
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"Added footer to {filename}")
