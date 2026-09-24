function switchTab(event, tabId) {
    const contents = document.querySelectorAll('.tab-content');
    contents.forEach(content => content.classList.remove('active'));

    const buttons = document.querySelectorAll('.tab-btn');
    buttons.forEach(btn => btn.classList.remove('active'));

    document.getElementById(tabId).classList.add('active');
    event.currentTarget.classList.add('active');
}

function copyCode(btn) {
    const codeBlock = btn.nextElementSibling;
    const text = codeBlock.innerText;

    navigator.clipboard.writeText(text).then(() => {
        btn.innerText = 'Copied!';
        btn.style.color = '#3fb950';
        btn.style.borderColor = '#3fb950';
        setTimeout(() => {
            btn.innerText = 'Copy';
            btn.style.color = '';
            btn.style.borderColor = '';
        }, 2000);
    }).catch(err => {
        console.error('Failed to copy: ', err);
    });
}
