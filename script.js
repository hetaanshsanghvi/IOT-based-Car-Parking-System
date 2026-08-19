import {
    initializeApp
} from
"https://www.gstatic.com/firebasejs/10.12.2/firebase-app.js";

import {
    getDatabase,
    ref,
    onValue
} from
"https://www.gstatic.com/firebasejs/10.12.2/firebase-database.js";


// ================================================
// FIREBASE CONFIGURATION
// ================================================

const firebaseConfig = {
    apiKey: "AIzaSyAsXN03thBp16wPMpYLUCkA4-47fTrZ1Wg",
    authDomain: "smart-parking-system-ce530.firebaseapp.com",
    databaseURL: "https://smart-parking-system-ce530-default-rtdb.asia-southeast1.firebasedatabase.app/",
    projectId: "smart-parking-system-ce530",
    storageBucket: "smart-parking-system-ce530.firebasestorage.app",
    messagingSenderId: "272259809328",
    appId: "1:272259809328:web:835aad2b3511d19157465d"
};


// ================================================
// INITIALIZE FIREBASE
// ================================================

const app = initializeApp(firebaseConfig);
const database = getDatabase(app);


// ================================================
// PARKING DATA TELEMETRY
// ================================================

const rootRef = ref(database);
const parkingGrid = document.getElementById("parkingGrid");

// Render parking slots visually based on total capacity and vehicles list
function updateParkingSlots(total, vehiclesData) {
    if (!parkingGrid) return;
    
    parkingGrid.innerHTML = "";
    
    for (let i = 1; i <= total; i++) {
        // Map slot i to vehicle at index i (Firebase array is 1-indexed due to null at index 0)
        const vehicle = vehiclesData ? vehiclesData[i] : null;
        const isOccupied = vehicle && vehicle.inside === true;
        const plateNumber = isOccupied ? (vehicle.vehicleNumber || 'MH01EF9012') : '';
        
        const slotEl = document.createElement("div");
        slotEl.className = `slot ${isOccupied ? 'occupied' : 'vacant'}`;
        
        let svgContent = '';
        if (isOccupied) {
            // Neon red top-down view car SVG
            svgContent = `
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                    <!-- Tires -->
                    <rect x="3" y="4" width="2" height="4" rx="0.5" fill="#f43f5e" stroke="none" />
                    <rect x="19" y="4" width="2" height="4" rx="0.5" fill="#f43f5e" stroke="none" />
                    <rect x="3" y="16" width="2" height="4" rx="0.5" fill="#f43f5e" stroke="none" />
                    <rect x="19" y="16" width="2" height="4" rx="0.5" fill="#f43f5e" stroke="none" />
                    
                    <!-- Side Mirrors -->
                    <path d="M4 10.5C3.5 10.5 3 10 3 9.5V9C3 8.5 3.5 8 4 8" stroke="#f43f5e" stroke-width="1.5" />
                    <path d="M20 10.5C20.5 10.5 21 10 21 9.5V9C21 8.5 20.5 8 20 8" stroke="#f43f5e" stroke-width="1.5" />
                    
                    <!-- Car Body -->
                    <rect x="5" y="2" width="14" height="20" rx="4" fill="rgba(244, 63, 94, 0.15)" stroke="#f43f5e" stroke-width="2" />
                    
                    <!-- Windshield -->
                    <path d="M7 8h10c0.5 0 0.8-0.3 0.9-0.8l0.4-2c0.1-0.6-0.3-1.2-0.9-1.2H6.6c-0.6 0-1 0.6-0.9 1.2l0.4 2C6.2 7.7 6.5 8 7 8z" fill="rgba(244, 63, 94, 0.3)" stroke="#f43f5e" stroke-width="1.5" />
                    
                    <!-- Rear Window -->
                    <path d="M7 17h10c0.5 0 0.8 0.3 0.9 0.8l0.2 1c0.1 0.6-0.3 1.2-0.9 1.2H6.8c-0.6 0-1-0.6-0.9-1.2l0.2-1C6.2 17.3 6.5 17 7 17z" fill="rgba(244, 63, 94, 0.3)" stroke="#f43f5e" stroke-width="1.5" />
                    
                    <!-- Headlights -->
                    <circle cx="8" cy="3.5" r="0.75" fill="#fff" stroke="none" />
                    <circle cx="16" cy="3.5" r="0.75" fill="#fff" stroke="none" />
                </svg>
            `;
        } else {
            // Vacant slot: Neon green outlined P symbol
            svgContent = `
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                    <circle cx="12" cy="12" r="9" stroke="rgba(16, 185, 129, 0.15)" stroke-dasharray="4" />
                    <path d="M9.5 16V8h3.5c1.4 0 2.5 1.1 2.5 2.5s-1.1 2.5-2.5 2.5H9.5" stroke="#10b981" stroke-width="2.5" />
                </svg>
            `;
        }

        slotEl.innerHTML = `
            <span class="slot-name">BAY-0${i}</span>
            ${isOccupied ? `<div class="plate-number">${plateNumber}</div>` : `<div class="plate-placeholder">&mdash;</div>`}
            <div class="slot-visual">
                ${svgContent}
            </div>
            <span class="slot-badge">${isOccupied ? 'Occupied' : 'Vacant'}</span>
        `;
        
        parkingGrid.appendChild(slotEl);
    }
}


onValue(
    rootRef,
    (snapshot) => {
        const data = snapshot.val();
        if (!data) return;

        const parkingData = data.parking || { totalCapacity: 4, occupied: 0, available: 4 };
        const vehiclesData = data.vehicles || [];

        // Update connection state to Live
        const statusBadge = document.getElementById("connectionStatus");
        if (statusBadge) {
            statusBadge.classList.remove("offline");
            document.getElementById("connectionText").textContent = "Firebase Live";
        }

        // TOTAL CAPACITY
        document.getElementById("total").textContent = parkingData.totalCapacity;

        // OCCUPIED
        document.getElementById("occupied").textContent = parkingData.occupied;

        // AVAILABLE
        document.getElementById("available").textContent = parkingData.available;

        // Visual Parking Bays Rendering - Map specific slot index to corresponding vehicle inside state
        updateParkingSlots(parkingData.totalCapacity || 4, vehiclesData);

        // PARKING LOT STATUS SUMMARY TEXT
        const slotStatusEl = document.getElementById("slotStatus");
        if (slotStatusEl) {
            if (parkingData.available === 0) {
                slotStatusEl.textContent = "Parking FULL. No slots currently available.";
            } else {
                slotStatusEl.textContent = `${parkingData.available} parking spaces vacant and ready.`;
            }
        }
    },
    (error) => {
        console.error("Firebase Connection Error:", error);

        // Update connection state to Offline
        const statusBadge = document.getElementById("connectionStatus");
        if (statusBadge) {
            statusBadge.classList.add("offline");
            document.getElementById("connectionText").textContent = "Firebase Offline";
        }

        const slotStatusEl = document.getElementById("slotStatus");
        if (slotStatusEl) {
            slotStatusEl.textContent = "Unable to sync with Firebase. Please check hardware connection.";
        }
    }
);