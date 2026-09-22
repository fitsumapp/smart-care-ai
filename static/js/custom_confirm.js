// Global interceptor for confirmation dialogs
document.addEventListener('DOMContentLoaded', () => {
    initCustomConfirm();
    // Scan and convert old confirm() calls to custom ones
    document.querySelectorAll('a[onclick*="confirm"]').forEach(link => {
        const onclickAttr = link.getAttribute('onclick');
        const match = onclickAttr.match(/confirm\('([^']+)'\)/);
        if (match) {
            const msg = match[1];
            link.removeAttribute('onclick');
            link.setAttribute('data-confirm-msg', msg);
            link.addEventListener('click', (e) => {
                e.preventDefault();
                showCustomConfirm("Confirm Action", msg, () => {
                    window.location.href = link.href;
                });
            });
        }
    });
});

let confirmCallback = null;

function initCustomConfirm() {
    if (!document.getElementById('customConfirm')) {
        const modalHtml = `
            <div id="customConfirm" class="confirm-overlay">
                <div class="confirm-modal">
                    <div class="confirm-icon"><i class="fas fa-exclamation-triangle"></i></div>
                    <h3 id="confirmTitle">Confirm Action</h3>
                    <p id="confirmMessage">Are you sure you want to proceed?</p>
                    <div class="confirm-actions">
                        <button id="confirmCancelBtn" class="confirm-btn confirm-btn-cancel">Cancel</button>
                        <button id="confirmOkBtn" class="confirm-btn confirm-btn-ok">Yes, Proceed</button>
                    </div>
                </div>
            </div>
        `;
        document.body.insertAdjacentHTML('beforeend', modalHtml);

        document.getElementById('confirmCancelBtn').onclick = closeCustomConfirm;
        document.getElementById('confirmOkBtn').onclick = handleConfirmOk;
        document.getElementById('customConfirm').onclick = (e) => { if (e.target.id === 'customConfirm') closeCustomConfirm(); };
    }
}

function showCustomConfirm(title, message, callback) {
    initCustomConfirm();
    document.getElementById('confirmTitle').innerText = title;
    document.getElementById('confirmMessage').innerText = message;
    confirmCallback = callback;

    const overlay = document.getElementById('customConfirm');
    overlay.style.display = 'flex';
    overlay.offsetHeight; // Force reflow
    overlay.classList.add('active');
}

function closeCustomConfirm() {
    const overlay = document.getElementById('customConfirm');
    overlay.classList.remove('active');
    setTimeout(() => { overlay.style.display = 'none'; }, 400);
}

function handleConfirmOk() {
    if (confirmCallback) confirmCallback();
    closeCustomConfirm();
}
