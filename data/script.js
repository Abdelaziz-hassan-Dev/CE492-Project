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
  
// Initialize Firebase
firebase.initializeApp(firebaseConfig);
const database = firebase.database();



// ================= Data Listener & Watchdog Logic =================

let watchdogTimer;

// Main Firebase Listener (Real-time updates)
database.ref('/sensor').on('value', (snapshot) => {
    const data = snapshot.val();
    
    if (data) {
        // 1. Data received indicates system is Online
        showOnlineStatus();

        // 2. Update UI elements
        document.getElementById("temperature").innerText = data.temperature.toFixed(1);
        document.getElementById("humidity").innerText = data.humidity.toFixed(1);
        
        updateFlameStatus(data.flame);
        updateGasStatus(data.gas);

        
        // 3. Watchdog Reset
        // Clear previous timer as we just received a heartbeat
        clearTimeout(watchdogTimer);

        // Set new timeout: if no data for 15s, mark system as Offline
        watchdogTimer = setTimeout(showOfflineStatus, 15000); 
    }
});

// UI State: Online
function showOnlineStatus() {
    const dot = document.getElementById("connectionDot");
    const text = document.getElementById("connectionText");
    
    dot.className = "dot online";
    text.innerText = "Live";
    
    // Restore opacity
    document.getElementById("temperature").style.opacity = "1";
    document.getElementById("humidity").style.opacity = "1";
    
    document.getElementById("lastUpdate").innerText = getCurrentTimeShort();
}

// UI State: Offline
function showOfflineStatus() {
    const dot = document.getElementById("connectionDot");
    const text = document.getElementById("connectionText");
    const flameText = document.getElementById("flame");

    dot.className = "dot offline";
    text.innerText = "Offline";
    
    // Dim values to indicate stale data
    document.getElementById("temperature").style.opacity = "0.4";
    document.getElementById("humidity").style.opacity = "0.4";
    
    // Update status indicators
    flameText.innerText = "No Signal";
    flameText.style.color = "gray";
    document.getElementById("flameCard").className = "card flame-card"; 
    document.getElementById("gas").innerText = "No Signal";
    document.getElementById("gas").style.color = "gray";
    document.getElementById("gasCard").className = "card gas-card";
}

// ================= Helper Functions =================

// Returns time in HH:MM format
function getCurrentTimeShort() {
    const now = new Date();
    return now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

function updateFlameStatus(status) {
    const el = document.getElementById("flame");
    const card = document.getElementById("flameCard");
    
    el.innerText = status;
    
    if(status === "Fire Detected") {
        card.classList.remove("flame-safe");
        card.classList.add("flame-danger");
        el.innerText = "Fire Detected! ⚠️";
        el.style.color = "#ff4444";
    } else {
        card.classList.remove("flame-danger");
        card.classList.add("flame-safe");
        el.innerText = "Safe ✅";
        el.style.color = "#00c851";
    }
}

function updateGasStatus(status) {
    const el   = document.getElementById("gas");
    const card = document.getElementById("gasCard");

    if (status === "Gas Leak Detected") {
        card.classList.remove("gas-safe");
        card.classList.add("gas-danger");
        el.innerText = "Gas Leak Detected! ⚠️";
        el.style.color = "#ff4444"; // غيّرنا اللون هنا للأحمر
    } else {
        card.classList.remove("gas-danger");
        card.classList.add("gas-safe");
        el.innerText = "Safe ✅";
        el.style.color = "#00c851";
    }
}

// Browser connection status (Debug only)
const connectedRef = firebase.database().ref(".info/connected");
connectedRef.on("value", (snap) => {
  if (snap.val() === true) {
    console.log("Browser connected to Firebase");
  } else {
    console.log("Browser disconnected from Firebase");
  }
});

