function applyTheme(theme) {
    const body = document.body;
    if (theme === 'light') {
        body.classList.add('light-mode');
        localStorage.setItem('theme', 'light');
    } else {
        body.classList.remove('light-mode');
        localStorage.setItem('theme', 'dark');
    }
}

// Load saved theme on DOM ready
document.addEventListener('DOMContentLoaded', () => {
    const savedTheme = localStorage.getItem('theme');
    if (savedTheme === 'light') {
        applyTheme('light');
    } else {
        applyTheme('dark');
    }
});
