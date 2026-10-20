function setCookie(name, value) {
    const hours = 24;
    const date = new Date();
    date.setTime(date.getTime() + (hours * 3600 * 1000));
    const expires = "; expires=" + date.toUTCString();
    document.cookie = name + "=" + (value || "") + expires + "; path=/; Secure; SameSite=Strict";
}
if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
    document.body.classList.add('darkmode');
    setSecureCookie("darkmode", 1);
} else {
    document.body.classList.remove('darkmode');
    setSecureCookie("darkmode", 0);
}
window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', event => {
    if (event.matches) {
        document.body.classList.add('darkmode');
        setSecureCookie("darkmode", 0);
    } else {
        document.body.classList.remove('darkmode');
        setSecureCookie("darkmode", 0);
    }
});