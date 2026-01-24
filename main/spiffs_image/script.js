let ws = null;

let last_counter = 0;


// gpio pin map: id -> physical pin number
const gpioMap = [
    { id: 0, pin: 8 },
    { id: 1, pin: 9 },
    { id: 2, pin: 10 },
    { id: 3, pin: 11 },
    { id: 4, pin: 12 },
    { id: 5, pin: 13 },
    { id: 6, pin: 19 },
    { id: 7, pin: 20 },
    { id: 8, pin: 21 },
    { id: 9, pin: 22 },
    { id: 10, pin: 23 },
    { id: 11, pin: 39 },
    { id: 12, pin: 40 },
    { id: 13, pin: 41 },
    { id: 14, pin: 42 },
    { id: 15, pin: 43 },
    { id: 16, pin: 44 },
    { id: 17, pin: 53 },
    { id: 18, pin: 54 }
];

// GPIO status storage: id -> {mode, level, pin}
let gpioStates = {};

function connectWS() {
    ws = new WebSocket(`ws://${window.location.host}/ws`);

    ws.onopen = () => {
        console.log("WebSocket connected");
        ws.send(JSON.stringify({ type: "get_status" }));
        ws.send(JSON.stringify({ type: "get_gpio_status" }));
    };

    ws.onmessage = (event) => {
        console.log("Received:", event.data);
        try {
            const data = JSON.parse(event.data);

            // update color and counter
            if (data.type === "status") {
                updateColor(data.color);
                updateCounter(data.counter);
            }

            // only update counter
            if (data.type === "counter_update") {
                updateCounter(data.counter);
            }

            // only update color
            if (data.type === "color_update") {
                updateColor(data.color);
            }

            if (data.counter !== undefined && data.color !== undefined) {
                updateColor(data.color);
                updateCounter(data.counter);
            }

            // update single GPIO status
            if (data.type === "gpio_update") {
                updateGPIO(data.id, data.mode, data.level);
            }

            // update all GPIO statuses (first connected)
            if (data.type === "gpio_status") {
                if (data.gpios && Array.isArray(data.gpios)) {
                    data.gpios.forEach(gpio => {
                        updateGPIO(gpio.id, gpio.mode, gpio.level);
                    });
                }
            }

            if (data.type === "gpio_confirm") {
                console.log(`GPIO id=${data.id} operation confirmed:`, data.message);
                if (data.success === false) {
                    const pin = gpioStates[data.id]?.pin || data.id;
                    alert(`GPIO ${pin} operation failed: ${data.message}`);
                    // Request latest GPIO status to rollback UI
                    ws.send(JSON.stringify({ type: "get_gpio_status" }));
                }
            }
        } catch (e) {
            console.warn("Received non-JSON:", event.data);
        }
    };

    ws.onclose = () => {
        console.log("WebSocket closed, reconnecting in 1s...");
        setTimeout(connectWS, 1000);
    };

    ws.onerror = (err) => {
        console.error("WebSocket error:", err);
    };
}

// update LED color display and sync color picker
function updateColor(color) {
    const led = document.getElementById('led');
    led.style.backgroundColor = color;
    led.style.borderColor = color;
    led.style.boxShadow = `0 0 20px ${color}, 0 0 40px ${color}`;
    // sync color picker value
    document.getElementById('colorPicker').value = color;
}

// update counter display with animation
function updateCounter(counter) {
    document.getElementById('counter').innerText = counter;
    if (counter !== last_counter) {
        last_counter = counter;

        const counterEl = document.getElementById('counter');
        counterEl.innerText = counter;
        // trigger flash animation
        counterEl.classList.remove('flash');
        void counterEl.offsetWidth;
        counterEl.classList.add('flash');

        // get button element
        const hwButtonEl = document.querySelector('.button-icon');
        if (hwButtonEl) {
            hwButtonEl.classList.remove('bounce');
            void hwButtonEl.offsetWidth;
            hwButtonEl.classList.add('bounce');
        }
    }
}

// reset counter value to zero
function resetCounter() {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: "reset_counter" }));
        console.log("Sent reset_counter request");
    } else {
        console.warn("WS not ready, cannot send reset_counter");
    }
}


/**
 * @brief Event listener for color picker changes
 *
 * This function sets up an event listener on the color picker element that
 * sends a WebSocket message to update the LED color when the user selects
 * a new color. It checks if the WebSocket connection is open before sending
 * the command, and logs a warning if the connection is not ready.
 *
 * The function sends a JSON message with type "set_color" containing the
 * selected color value in hex format to the WebSocket server.
 */
document.getElementById('colorPicker').addEventListener('change', function () {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const c = this.value;
        ws.send(JSON.stringify({ type: "set_color", color: c }));
    } else {
        console.warn("WS not ready, cannot send set_color");
    }
});

/**
 * @brief Maps physical pin numbers to GPIO IDs
 *
 * This function creates a lookup table to find the corresponding ID for a given physical pin number
 */
function getGpioIdByPin(pinNumber) {
    const entry = gpioMap.find(entry => entry.pin === pinNumber);
    return entry ? entry.id : null;
}

/**
 * @brief Adds click event listeners to all GPIO circles after DOM loads
 */
function initGPIO() {
    // Initialize GPIO states with default values
    gpioMap.forEach(gpio => {
        gpioStates[gpio.id] = { mode: 'input', level: 0, pin: gpio.pin };
    });

    // Add click event to all GPIO circles
    const gpioCircles = document.querySelectorAll('.gpio-circle');
    gpioCircles.forEach(circle => {
        circle.addEventListener('click', function () {
            const pinNumber = parseInt(this.getAttribute('data-id'));
            const id = getGpioIdByPin(pinNumber);
            if (id !== null) {
                showGPIOOptions(id);
            } else {
                console.warn(`Could not find GPIO ID for pin ${pinNumber}`);
            }
        });
    });
}

/**
 * @brief Displays the GPIO options modal with actions appropriate to the current GPIO mode
 *
 * This function shows a modal dialog with different buttons depending on whether the GPIO
 * is currently in input or output mode. In input mode, it allows changing to output mode.
 * In output mode, it provides options to change back to input mode or set high/low output levels.
 *
 * @param {number} id - The GPIO identifier to show options for
 */
function showGPIOOptions(id) {
    const modal = document.getElementById('gpioModal');
    const options = document.getElementById('modalOptions');
    const title = document.getElementById('modalTitle');
    const state = gpioStates[id];
    const pinNumber = gpioStates[id].pin;

    title.textContent = `GPIO ${pinNumber} Control`;

    options.innerHTML = '';

    if (state.mode === 'input') {
        const btn = document.createElement('button');
        btn.className = 'gpio-option-btn input-mode';
        btn.textContent = 'Change to Output Mode';
        btn.onclick = () => {
            sendGPIOCommand(id, 'set_mode', 'output');
            modal.style.display = 'none';
        };
        options.appendChild(btn);
    } else {
        const btn1 = document.createElement('button');
        btn1.className = 'gpio-option-btn input-mode';
        btn1.textContent = 'Change to Input Mode';
        btn1.onclick = () => {
            sendGPIOCommand(id, 'set_mode', 'input');
            modal.style.display = 'none';
        };

        const btn2 = document.createElement('button');
        btn2.className = 'gpio-option-btn high';
        btn2.textContent = 'Set High Level';
        btn2.onclick = () => {
            sendGPIOCommand(id, 'set_level', 'high');
            modal.style.display = 'none';
        };

        const btn3 = document.createElement('button');
        btn3.className = 'gpio-option-btn low';
        btn3.textContent = 'Set Low Level';
        btn3.onclick = () => {
            sendGPIOCommand(id, 'set_level', 'low');
            modal.style.display = 'none';
        };

        options.appendChild(btn1);
        options.appendChild(btn2);
        options.appendChild(btn3);
    }

    // Add cancel button
    const cancelBtn = document.createElement('button');
    cancelBtn.className = 'gpio-option-btn cancel';
    cancelBtn.textContent = 'Cancel';
    cancelBtn.onclick = () => {
        modal.style.display = 'none';
    };
    options.appendChild(cancelBtn);

    modal.style.display = 'block';
}

/**
 * @brief Sends a GPIO control command to the backend via WebSocket
 *
 * This function sends a JSON message to the ESP32 device over WebSocket to control a GPIO pin.
 * It supports setting the mode (input/output) or setting the level (high/low) of the pin.
 * After sending the command, it immediately updates the local display (optimistic update).
 *
 * @param {number} id - The GPIO identifier to control
 * @param {string} action - The action to perform ('set_mode' or 'set_level')
 * @param {string} value - The value for the action ('input', 'output', 'high', or 'low')
 */
function sendGPIOCommand(id, action, value) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const payload = {
            type: "gpio_control",
            id: id,
            action: action,
            value: value
        };

        console.log("Sending GPIO command:", payload);
        ws.send(JSON.stringify(payload));

        // Optimistic update: update UI immediately
        if (action === 'set_mode') {
            gpioStates[id].mode = value;
            updateGPIOModeDisplay(id);
        } else if (action === 'set_level') {
            gpioStates[id].level = value === 'high' ? 1 : 0;
            updateGPIOLevelDisplay(id);
        }
    } else {
        console.warn("WebSocket not ready, cannot send GPIO command");
        alert("Connection lost, please refresh the page");
    }
}

/**
 * @brief Updates the mode display of a GPIO pin
 *
 * @param {number} id - The GPIO identifier to update in the display
 */
function updateGPIOModeDisplay(id) {
    const state = gpioStates[id];
    const pinNumber = state.pin;

    // Update the mode display element - look for elements with id like "mode-8", "mode-9", etc.
    const modeElement = document.getElementById(`mode-${pinNumber}`);

    if (modeElement) {
        const modeText = state.mode === 'input' ? '(INPUT)' : '(OUTPUT)';
        modeElement.textContent = modeText;
    }
}

/**
 * @brief Updates the level display of a GPIO pin
 *
 * @param {number} id - The GPIO identifier to update in the display
 */
function updateGPIOLevelDisplay(id) {
    const state = gpioStates[id];
    const pinNumber = state.pin;
    const circle = document.querySelector(`.gpio-circle[data-id="${pinNumber}"]`);

    if (circle) {
        // Remove existing classes
        circle.classList.remove('high', 'low');

        if (state.level === 1) {
            circle.classList.add('high');
        } else {
            circle.classList.add('low');
        }
    }
}

/**
 * @brief Updates the visual representation of a GPIO pin in the UI
 *
 * This function updates the display of a GPIO pin based on its current state,
 * including the mode indicator text and the color of the pin circle which reflects
 * the current level (green for high, gray for low).
 *
 * @param {number} id - The GPIO identifier to update in the display
 */
function updateGPIODisplay(id) {
    updateGPIOModeDisplay(id);
    updateGPIOLevelDisplay(id);
}

/**
 * @brief Updates the internal state and UI display of a GPIO pin based on server feedback
 *
 * This function is called when the client receives a GPIO status update from the server.
 * It updates the internal state stored in gpioStates and refreshes the UI to reflect
 * the new mode and level of the specified GPIO pin.
 *
 * @param {number} id - The GPIO identifier that was updated
 * @param {string} mode - The new mode of the GPIO ('input' or 'output')
 * @param {number} level - The new level of the GPIO (0 or 1)
 */
function updateGPIO(id, mode, level) {
    if (gpioStates[id]) {
        gpioStates[id].mode = mode;
        gpioStates[id].level = level;

        updateGPIODisplay(id);

        console.log(`GPIO id=${id} (pin=${gpioStates[id].pin}) updated: mode=${mode}, level=${level}`);
    }
}

// Close modal when clicking outside content area
document.getElementById('gpioModal').addEventListener('click', function (e) {
    if (e.target === this) {
        this.style.display = 'none';
    }
});

// Initialize GPIO functionality when DOM is loaded, then connect WebSocket
document.addEventListener('DOMContentLoaded', () => {
    initGPIO();
    connectWS();
});
