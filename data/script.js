// 1. Firebase Configuration
const firebaseConfig = {
    apiKey: "AIzaSyDDdgCi-ZwiVJN9xIBd-BsopL8tWbnfZWo",
    authDomain: "esp32-ce491.firebaseapp.com",
    databaseURL: "https://esp32-ce491-default-rtdb.firebaseio.com",
    projectId: "esp32-ce491",
    storageBucket: "esp32-ce491.firebasestorage.app",
    messagingSenderId: "1012960274280",
    appId: "1:1012960274280:web:84a6c1800fb722cb6d58dd"
};
  
firebase.initializeApp(firebaseConfig);
const database = firebase.database();

// ================= Watchdog =================
let watchdogTimer;

// ================= Main Listener =================
database.ref('/sensor').on('value', (snapshot) => {
    const data = snapshot.val();
    if (!data) return;

    showOnlineStatus();

    // Update numeric values
    const temp = parseFloat(data.temperature);
    const hum  = parseFloat(data.humidity);

    document.getElementById("temperature").innerText = isNaN(temp) ? "--" : temp.toFixed(1);
    document.getElementById("humidity").innerText    = isNaN(hum)  ? "--" : hum.toFixed(1);

    // Update progress bars (temp 0–60°C range, hum 0–100%)
    const tempBar = document.getElementById("tempBar");
    const humBar  = document.getElementById("humBar");
    if (tempBar) tempBar.style.width = Math.min(Math.max((temp / 60) * 100, 0), 100) + "%";
    if (humBar)  humBar.style.width  = Math.min(Math.max(hum, 0), 100) + "%";

    updateFlameStatus(data.flame);
    updateGasStatus(data.gas);

    // Watchdog reset
    clearTimeout(watchdogTimer);
    watchdogTimer = setTimeout(showOfflineStatus, 15000);
});

// ================= UI States =================

function showOnlineStatus() {
    document.getElementById("connectionDot").className  = "dot online";
    document.getElementById("connectionText").innerText = "Live";
    document.getElementById("temperature").style.opacity = "1";
    document.getElementById("humidity").style.opacity    = "1";
    document.getElementById("lastUpdate").innerText = getCurrentTimeShort();
}

function showOfflineStatus() {
    document.getElementById("connectionDot").className  = "dot offline";
    document.getElementById("connectionText").innerText = "Offline";
    document.getElementById("temperature").style.opacity = "0.35";
    document.getElementById("humidity").style.opacity    = "0.35";

    const flame = document.getElementById("flame");
    flame.innerText    = "No Signal";
    flame.style.color  = "var(--text-sec)";
    document.getElementById("flameCard").className = "card flame-card";

    const gas = document.getElementById("gas");
    gas.innerText    = "No Signal";
    gas.style.color  = "var(--text-sec)";
    document.getElementById("gasCard").className = "card gas-card";
}

// ================= Status Helpers =================

function updateFlameStatus(status) {
    const el   = document.getElementById("flame");
    const card = document.getElementById("flameCard");

    if (status === "Fire Detected") {
        card.className  = "card flame-card flame-danger";
        el.innerText    = "🔥 Fire Detected!";
        el.style.color  = "var(--danger)";
    } else {
        card.className  = "card flame-card flame-safe";
        el.innerText    = "✅ Safe";
        el.style.color  = "var(--safe)";
    }
}

function updateGasStatus(status) {
    const el   = document.getElementById("gas");
    const card = document.getElementById("gasCard");

    if (status === "Gas Leak Detected") {
        card.className  = "card gas-card gas-danger";
        el.innerText    = "⚠️ Gas Leak!";
        el.style.color  = "#a29bfe";
    } else {
        card.className  = "card gas-card gas-safe";
        el.innerText    = "✅ Safe";
        el.style.color  = "var(--safe)";
    }
}

// ================= Utilities =================

function getCurrentTimeShort() {
    return new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

// Debug: Firebase connection
firebase.database().ref(".info/connected").on("value", (snap) => {
    console.log(snap.val() ? "Connected to Firebase" : "Disconnected from Firebase");
});
