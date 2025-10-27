/**
 * @file main.js
 * @brief UI helpers and application controller: builds the sidebar, wires system-state buttons,
 *        brokers messages between a SharedWorker, the window, and an iframe.
 * @author
 *   Nathan.Ikolo
 * @date 2023-12-01
 * @version 1.0
 *
 * @details
 * This script centralizes DOM helpers (Doc) and the main UI/controller (Body).
 * It renders a hex-grid sidebar for modules/settings from JSON, initializes a SharedWorker
 * for cross-tab coordination, and forwards messages to the main iframe.
 *
 * @par Doxygen notes
 * - Doxygen can parse JSDoc with: `OPTIMIZE_OUTPUT_FOR_C = NO`
 * - Map JavaScript: `EXTENSION_MAPPING = js=JavaScript`
 * - Useful tags used here: @class, @constructor, @param, @returns, @throws, @event, @listens, @fires, @example, @typedef, @enum, @warning, @note
 *
 * @see worker.js       (message producer/consumer via SharedWorker)
 * @see sidebar.json    (module presence and metadata consumed by show_modules)
 */

/**
 * @class Doc
 * @classdesc Lightweight DOM / network utility wrapper to reduce repetitive boilerplate.
 * @example
 * const d = new Doc();
 * const btn = d.getElementById("myBtn");
 * d.setAttribute("myDiv", { name: "data-role", value: "panel" });
 */
class Doc {
    /**
     * @constructor
     * @note Stores `document` and a plain `XMLHttpRequest` instance for possible legacy use.
     */
    constructor() {
        /** @type {Document} */
        this.Document = document;
        /** @type {XMLHttpRequest} */
        this.xhr = new XMLHttpRequest();
    }

    /**
     * Safe wrapper around `document.getElementById`.
     * @param {string} id - Element ID to look up.
     * @returns {HTMLElement|null} The element if present; otherwise `null`.
     * @throws {Error} When the element ID is not found (caught internally and returns `null`).
     * @note Prefer this over direct access when an element may be optional.
     */
    getElementById(id) {
        try {
            const element = this.Document.getElementById(id);
            if (element === null) {
                throw new Error(`Element with ID '${id}' not found.`);
            }
            return element;
        } catch (_error) {
            // Intentionally swallow to keep caller-side branching simple.
            return null;
        }
    }

    /**
     * POST helper using Fetch API.
     * @param {string} apiUrl - Target endpoint URL.
     * @param {*} postData - Request body (already serialized if needed).
     * @returns {Promise<void>} Resolves when the request finishes.
     * @fires fetch:error When the network request fails (logged to console).
     *
     * @example
     * const doc = new Doc();
     * await doc.post('/api/log', JSON.stringify({ msg: 'hello'}));
     *
     * @warning Sends Content-Type: text/plain; ensure server expects this or adjust as needed.
     */
    post(apiUrl, postData) {
        // Use the Fetch API to make a POST request
        return fetch(apiUrl, {
                method: 'POST',
                cache: 'no-cache',
                headers: {
                    'Content-Type': 'text/plain'
                },
                body: (postData)
            })
            .then(response => {
                if (!response.ok) {
                    // Bubble to catch; useful for callers that await and wrap in try/catch
                    throw new Error('Network response was not ok');
                }
            })
            .catch(error => {
                // Emit as console error to keep UI resilient; callers may still await this.
                console.error('Error during fetch:', error);
            });
    }

    /**
     * Set an attribute on a specific element by ID.
     * @param {string} id - Target element ID.
     * @param {{name:string, value:string}} attribute - Attribute name and value.
     * @returns {void}
     */
    setAttribute(id, attribute) {
        try {
            const element = this.getElementById(id); // safe access
            if (element !== null) {
                element.setAttribute(attribute.name, attribute.value);
            } else {
                throw new Error(`Element with ID '${id}' not found; attribute not set.`);
            }
        } catch (error) {
            console.error("Error:", error.message);
        }
    }

    /**
     * Create a DOM element and append to a parent node.
     * @param {keyof HTMLElementTagNameMap} type - Element tag name (e.g., `"div"`, `"table"`).
     * @param {?string} id - Optional element id.
     * @param {?string} className - Optional class string.
     * @param {HTMLElement} body - Parent to append the element to (must exist).
     * @returns {HTMLElement|null} The created element or `null` on failure.
     * @warning `body` must be non-null; ensure DOMContentLoaded has fired.
     */
    createElement(type, id, className, body) {
        try {
            const element = this.Document.createElement(type);
            if (id !== null) element.id = id;
            if (className !== null) element.className = className;
            body.appendChild(element);
            return element;
        } catch (error) {
            console.log("Error:", error.message);
            return null;
        }
    }

    /**
     * Wraps `setInterval`.
     * @param {Function} callback - Callback to invoke.
     * @param {number} timer_interval - Interval in ms.
     * @returns {number} Interval ID.
     */
    set_intervals_callbacks(callback, timer_interval) {
        return setInterval(callback, timer_interval);
    }

    /**
     * Wraps `clearInterval`.
     * @param {number} callback_id - Interval ID returned by `setInterval`.
     * @returns {void}
     */
    clear_intervals_callbacks(callback_id) {
        return clearInterval(callback_id);
    }

    /**
     * Remove the `selected` class from a list of elements whose IDs share a prefix.
     * @param {string} prefix - Common prefix of element IDs (e.g., "id_inner_hex_small_").
     * @param {Array<string|number>} element_id - Suffixes composing full IDs.
     * @returns {void}
     * @example
     * d.clear_select("id_inner_hex_small_", ["A1","A2","A3"]);
     */
    clear_select(prefix, element_id) {
        for (let j = 0; j < element_id.length; j++) {
            const button = this.Document.getElementById(prefix + element_id[j]);
            if (!button) continue; // defensive: skip missing elements
            const list = Array.from(button.classList);
            for (let k = 0; k < list.length; k++) {
                if (list[k] === 'selected') {
                    button.classList.remove('selected');
                }
            }
        }
    }
}

/*
    IDs used by message broker:
      main:0, iframe:1, server:2
    payload: { type:string, dest:number, ... }
*/

/**
 * @enum {string}
 * @readonly
 * @description Readable identifiers for system state buttons. Index order matters for mapping handlers.
 */
const status_button_li = ['TRANSMIT','STANDBY','OFF'];

/** Settings panel identifiers (used to build hex grid). */
const settings_id    = ['sum', 'config', 'log', 'load'];
/** Display labels for each setting tile. */
const settings_names = ['CONTROL', 'CONFIG', 'LOG', 'UPGRADE'];
/** Target pages for each setting tile (loaded into the main iframe). */
const settings_links = ['./setting/summary.htm', './setting/settings.htm', './setting/log.htm', './setting/upload.htm'];

/**
 * NOTE: `type` is intentionally global (no var/let/const) to preserve original behavior.
 * @global
 * @const {!Array<string>} Module type labels by slot index.
 * @warning Globals complicate testing; consider refactoring to encapsulate in Body if possible.
 */
type = ['TX', 'TX', 'RX', 'RX', 'CPU'];

/**
 * @typedef {Object} SidebarSelectors
 * @property {string} modules - Element id for the modules container (sidebar left, upper).
 * @property {string} settings - Element id for the settings container (sidebar left, lower).
 * @property {string} display_panel - Element id for the main iframe/display.
 */

/**
 * Emitted by the SharedWorker to notify websocket status changes.
 * @event ws-status
 * @type {object}
 * @property {"open"|"closed"|"disconnected"|"closing"} status - Connection state.
 */

/**
 * Forwarded or received messages that should be routed by {@link Body#broker}.
 * @event routed-message
 * @type {object}
 * @property {string} type - Message type (e.g., "ping", "publish", etc.).
 * @property {number} dest - Destination id (0 main, 1 iframe, 2 server).
 * @property {*} [payload] - Arbitrary message-specific data.
 */

/**
 * @class Body
 * @classdesc Main UI/controller: builds sidebar, wires event handlers, and brokers messages
 *            between the SharedWorker, window, and the main iframe.
 * @example
 * // Typical bootstrap:
 * const ui = new Body({ modules: "sidebar_modules", settings: "sidebar_settings", display_panel: "main_page" });
 * ui.show_modules();
 * ui.load();
 */
class Body {
    /**
     * @constructor
     * @param {SidebarSelectors} select - Target element ids for major UI regions.
     */
    constructor(select) {
        /** @type {SharedWorker|null} */
        this.worker = null;
        /** @type {MessagePort|null} */
        this.port = null;

        /** @type {!Array<HTMLElement>} */
        this.slots = [];
        /** @type {!Array<HTMLButtonElement>} */
        this.status_buttons = [];

        /** @type {!Array<HTMLElement>} */
        this.sidebar_dividers_outer = [];
        /** @type {!Array<HTMLElement>} */
        this.sidebar_dividers_inner = [];
        /** @type {!Array<HTMLTableCellElement>} */
        this.sidebar_table_slot_cells = []; // dynamic cells to align hexagon slot buttons
        /** @type {!Array<HTMLElement>} */
        this.sidebar_settings = [];
        /** @type {!Array<HTMLButtonElement>} */
        this.sidebar_settings_ctrl = [];
        /** @type {!Array<HTMLElement>} */
        this.sidebar_settings_div_outer = [];
        /** @type {!Array<HTMLElement>} */
        this.sidebar_settings_div_inner = [];
        /** @type {!Array<HTMLElement>} */
        this.module_summary = [];

        /** @type {Doc} */
        this.Document = new Doc();

        /** @type {HTMLElement|null} */
        this.module_ctrl = this.Document.getElementById(select.modules);
        /** @type {HTMLElement|null} */
        this.settings = this.Document.getElementById(select.settings);
        /** @type {HTMLIFrameElement|null} */
        this.main = this.Document.getElementById(select.display_panel);
        /** @type {!Array<HTMLElement>} */
        this.main_summary_cells = [];

        // Bind methods for event handlers / callbacks (keep `this` stable)
        this.show_settings = this.load.bind(this);
        this.show_modules = this.show_modules.bind(this);
        this.send_message = this.send_message.bind(this);
        this.test_parent_post = this.test_parent_post.bind(this);
        this.on_message_worker = this.on_message_worker.bind(this);
        this.broker = this.broker.bind(this);
        this.status_button_off_fn = this.status_button_off_fn.bind(this);
        this.status_button_standby_fn = this.status_button_standby_fn.bind(this);
        this.status_button_transmit_fn = this.status_button_transmit_fn.bind(this);
        this.transmit_button_state = this.transmit_button_state.bind(this);
        /** @private */
        this.status_buttons_fn = [this.status_button_transmit_fn, this.status_button_standby_fn, this.status_button_off_fn];
    }

    /**
     * Build the module sidebar on DOMContentLoaded; fetches slot definitions from `sidebar.json`
     * and renders hexagon buttons + a single-row table to align them.
     * @async
     * @returns {Promise<void>}
     * @listens window#DOMContentLoaded
     * @see sidebar.json
     */
    async show_modules() {
        // Defer DOM operations until content is ready
        this.Document.Document.addEventListener('DOMContentLoaded', () => {
            // Create dynamic table inserted into sidebar to align hexagon slot elements
            fetch("sidebar.json", {
                    method: 'GET',
                    cache: 'no-cache', // prevent caching while developing / hot-reloading
                })
                .then((response) => response.json())
                .then((jsonArray) => {
                    /** @type {HTMLTableElement} */
                    this.sidebar_table_slot = this.Document.createElement("table", null, null, this.module_ctrl); // table
                    /** @type {HTMLTableRowElement} */
                    this.sidebar_table_slot_row = this.sidebar_table_slot.insertRow(-1); // single row
                    this.sidebar_table_slot.setAttribute("padding-top", "30px");

                    // Collect only present slots to streamline selection-clearing later
                    /** @type {!Array<string>} */
                    const slots_names = [];
                    for (let i = 0, j = 0; i < jsonArray.length; i++) {
                        if (jsonArray[i].present === 1) {
                            slots_names[j++] = jsonArray[i].slot;
                        }
                    }

                    // Render each slot tile: enabled if present, otherwise disabled
                    jsonArray.forEach((item, i) => {
                        if (item.present === 1) {
                            this.sidebar_dividers_outer[i] = this.Document.createElement(
                                "div", "id_outer_hex_small_" + item.slot, "class_outer_hexagon_small Operating", this.module_ctrl
                            );
                            this.sidebar_dividers_inner[i] = this.Document.createElement(
                                "div", "id_inner_hex_small_" + item.slot, "class_inner_hexagon_small enabled", this.module_ctrl
                            );
                            this.sidebar_table_slot_cells[i] = this.sidebar_table_slot_row.insertCell(i);
                            this.slots[i] = this.Document.createElement(
                                "button", "id_slot_button_" + item.slot, "class_slot_ctrl enabled", this.module_ctrl
                            );
                            this.slots[i].innerHTML = item.slot;

                            // On click: clear previous selections, mark new, and navigate iframe
                            this.slots[i].addEventListener("click", function () {
                                const Document = new Doc();
                                Document.clear_select("id_inner_hex_small_", slots_names);
                                Document.clear_select("id_inner_hex_medium_", settings_id);
                                Document.getElementById("id_inner_hex_small_" + item.slot).classList.add('selected');
                                Document.getElementById("main_page").setAttribute("page", "module_" + item.slot);
                                Document.getElementById("main_page").setAttribute(
                                    "src", "./module/home_" + item.type + ".htm?slot_id=" + item.slot
                                );
                            });

                        } else {
                            // Not present: render a disabled-looking tile
                            this.sidebar_dividers_outer[i] = this.Document.createElement(
                                "div", "id_outer_hex_small_" + item.slot, "class_outer_hexagon_small disabled", this.module_ctrl
                            );
                            this.sidebar_dividers_inner[i] = this.Document.createElement(
                                "div", "id_inner_hex_small_" + item.slot, "class_inner_hexagon_small disabled", this.module_ctrl
                            );
                            this.sidebar_table_slot_cells[i] = this.sidebar_table_slot_row.insertCell(i);
                            this.slots[i] = this.Document.createElement(
                                "button", "id_slot_button_" + type[item.slot], "class_slot_ctrl disabled", this.module_ctrl
                            );
                            this.slots[i].innerHTML = item.slot;
                        }
                        // Nest structure: inner hex -> outer hex -> table cell
                        this.sidebar_dividers_inner[i].appendChild(this.slots[i]);
                        this.sidebar_dividers_outer[i].appendChild(this.sidebar_dividers_inner[i]);
                        this.sidebar_table_slot_cells[i].appendChild(this.sidebar_dividers_outer[i]);
                    });
                })
                .catch((error) => console.log("Error fetching data:", error));
        });
    }

    /**
     * Initialize event listeners, SharedWorker, status buttons, and settings grid on DOMContentLoaded.
     * @returns {void}
     * @listens window#DOMContentLoaded
     * @fires ws-status
     */
    load() {
        this.Document.Document.addEventListener('DOMContentLoaded', () => {
            // Route only same-origin messages for safety
            window.addEventListener("message", (event) => {
                if (event.origin === window.origin) {
                    this.broker(event.data);
                }
            });

            // SharedWorker bootstrap (if supported)
            if ('SharedWorker' in window) {
                try {
                    /** @type {SharedWorker} */
                    this.worker = new SharedWorker('worker.js', { name: 'worker' });
                    /** @type {MessagePort} */
                    this.port = this.worker.port;
                    this.port.start();
                    // Handle messages arriving from worker
                    this.port.onmessage = this.on_message_worker;
                } catch (err) {
                    console.error('Failed to start SharedWorker:', err);
                }
            }

            // System state buttons (OFF/STANDBY/TRANSMIT)
            this.status_buttons = new Array(status_button_li.length);
            status_button_li.forEach((button, i) => {
                /** @type {HTMLButtonElement|null} */
                this.status_buttons[i] = this.Document.getElementById(button);
                if (this.status_buttons[i]) {
                    const Document = new Doc();
                    Document.clear_select("", status_button_li);
                    this.status_buttons[i].textContent = button;
                    this.status_buttons[i].classList.add('selected');
                    this.status_buttons[i].addEventListener('click', this.status_buttons_fn[i]);
                }
            });

            // Settings hex grid (2x2)
            /** @type {HTMLTableElement} */
            this.sidebar_setting_table = this.Document.createElement("table", null, null, this.settings);
            /** @type {!Array<HTMLTableCellElement>} */
            this.sidebar_setting_cell_upper = new Array(2);
            /** @type {!Array<HTMLTableCellElement>} */
            this.sidebar_setting_cell_lower = new Array(2);
            /** @type {!Array<HTMLTableRowElement>} */
            this.sidebar_settings_rows = new Array(2);
            this.sidebar_settings_rows[0] = this.sidebar_setting_table.insertRow();
            this.sidebar_settings_rows[1] = this.sidebar_setting_table.insertRow();
            this.sidebar_setting_cell_upper[0] = this.sidebar_settings_rows[0].insertCell(0);
            this.sidebar_setting_cell_upper[1] = this.sidebar_settings_rows[0].insertCell(1);
            this.sidebar_setting_cell_lower[0] = this.sidebar_settings_rows[1].insertCell(0);
            this.sidebar_setting_cell_lower[1] = this.sidebar_settings_rows[1].insertCell(1);
            this.sidebar_settings_rows[0].className = "class_setting_upper";
            this.sidebar_settings_rows[1].className = "class_setting_lower";

            // Create 4 setting tiles and wire navigation into iframe
            for (let i = 0; i < settings_id.length; i++) {
                this.sidebar_settings_div_outer[i] = this.Document.createElement(
                    "div", "id_outer_hex_medium_" + settings_id[i], "class_outer_hexagon_medium Operating", this.settings
                );
                this.sidebar_settings_div_inner[i] = this.Document.createElement(
                    "div", "id_inner_hex_medium_" + settings_id[i], "class_inner_hexagon_medium", this.settings
                );
                this.sidebar_settings_ctrl[i] = this.Document.createElement(
                    "button", "id_setting_" + settings_id[i], "class_setting_ctrl", this.settings
                );
                this.sidebar_settings_ctrl[i].innerHTML = settings_names[i];
                this.sidebar_settings_ctrl[i].addEventListener("click", function () {
                    const Document = new Doc();
                    Document.clear_select("id_inner_hex_medium_", settings_id);
                    Document.getElementById("id_inner_hex_medium_" + settings_id[i]).classList.add('selected');
                    Document.getElementById("main_page").setAttribute("page", "setting_" + settings_id[i]);
                    Document.getElementById("main_page").setAttribute("src", settings_links[i]);
                });
                this.sidebar_settings_div_inner[i].appendChild(this.sidebar_settings_ctrl[i]);
                this.sidebar_settings_div_outer[i].appendChild(this.sidebar_settings_div_inner[i]);
            }
            // Place tiles into the 2x2 table (row-major)
            this.sidebar_setting_cell_upper[0].appendChild(this.sidebar_settings_div_outer[0]);
            this.sidebar_setting_cell_upper[1].appendChild(this.sidebar_settings_div_outer[1]);
            this.sidebar_setting_cell_lower[0].appendChild(this.sidebar_settings_div_outer[2]);
            this.sidebar_setting_cell_lower[1].appendChild(this.sidebar_settings_div_outer[3]);
        });
    }

    /**
     * Debug helper to post a message from parent to iframe.
     * @returns {void}
     * @example
     * ui.test_parent_post(); // sends a "publish" to iframe topic "chat/messages"
     */
    test_parent_post() {
        console.log("posting from parent");
        const iframe = document.getElementById("main_page");
        iframe.contentWindow.postMessage({
            type: "publish",
            topic: "chat/messages",
            payload: { message: "Hello from parent!" }
        }, window.origin);
    }

    /** @returns {void} */
    status_button_off_fn() {
        let button = this.Document.getElementById(status_button_li[2]);
        this.Document.clear_select("",status_button_li);
        button.classList.add('selected');
        let d = this.Document.getElementById(status_button_li[0]);
        d.classList.add('disabled');
        d.removeEventListener('click',this.status_buttons_fn[0]);
        this.broker({type:"broadcast",dest:2,payload:"off"});
        console.log('status button off clicked');
        // TODO: Wire to backend command once available (e.g., { cmd: "system.off" })
    }

    /** @returns {void} */
    status_button_standby_fn() {
        let button = this.Document.getElementById(status_button_li[1]);
        this.Document.clear_select("",status_button_li);
        button.classList.add('selected');
        let d = this.Document.getElementById(status_button_li[0]);
        d.classList.remove('disabled');
        d.addEventListener('click',this.status_buttons_fn[0]);
        this.broker({type:"broadcast",dest:2,payload:"on"});
        console.log('status button standby clicked');
        // TODO: Wire to backend command once available (e.g., { cmd: "system.standby" })
    }

    /** @returns {void} */
    status_button_transmit_fn() {
        let button = this.Document.getElementById(status_button_li[0]);
        this.Document.clear_select("",status_button_li);
        button.classList.add('selected');
        console.log('status button transmit clicked');
        // TODO: Wire to backend command once available (e.g., { cmd: "system.transmit" })
    }

    transmit_button_state(state){
        let btn = this.Document.getElementById(status_button_li[0]);
        if(state === 'on'){
            if(btn.classList.contains('disabled')){
                btn.classList.remove('disabled');
                btn.addEventListener('click',this.status_buttons_fn[0]);
            }
        }else{
            btn.classList.add('disabled');
            btn.removeEventListener('click',this.status_buttons_fn[0]);
        }
    }

    /**
     * Handle messages from the SharedWorker port.
     * @param {MessageEvent} e - Worker message event.
     * @returns {void}
     * @listens SharedWorker#message
     * @fires routed-message
     */
    on_message_worker(e) {
        const { type, status, payload } = e.data;
        switch (type) {
            case "ws-status":
                // Update a simple UI status indicator (if present)
                switch (status) {
                    case "open": {
                        const statusEl = document.getElementById('status');
                        if (statusEl) statusEl.textContent = "connected";
                        break;
                    }
                    case "closed":
                    case "disconnected":
                    case "closing": {
                        const statusEl = document.getElementById('status');
                        if (statusEl) statusEl.textContent = "closed";
                        break;
                    }
                }
                break;
            case "tab-broadcast":
                this.transmit_button_state(payload.payload);
                break;
            case "server-message":
            case "message-main": {
                // Forward messages to broker for routing to iframe/server/main
                console.log(payload);
                this.broker(payload);
                break;
            }
        }
    }

    /**
     * Message router for incoming payloads from worker/window.
     * @param {{type:string, dest:number}} payload - Generic payload with destination.
     * @returns {void}
     * @example
     * ui.broker({ type: "ping", dest: 0 });
     * @note
     * dest legend: 0 -> main page handler (noop placeholder), 1 -> iframe, 2 -> broadcast to tabs, other -> server via worker.
     */
    broker(payload) {
        const { type, dest } = payload;
        if (type === "ping") {
            console.log("server sent: ", payload);
        } else {
            switch (dest) {
                case 0: // addressed to the main page (no-op hook; extend here if needed)
                    break;
                case 1: { // sending to the iframe
                    const iframe = document.getElementById("main_page");
                    iframe.contentWindow.postMessage(payload, window.origin);
                    break;
                }
                case 2:
                    this.send_message({type:"broadcast",payload:payload});
                    break;
                default: // send this message to server via worker port (if present)
                    console.log(payload);
                    this.send_message({ type: "client-message", payload: payload });
                    break;
            }
        }
    }

    /**
     * Send a message to the SharedWorker (if present).
     * @param {*} msg - Arbitrary message object to post.
     * @returns {void}
     * @warning No-op if `this.port` is null (e.g., unsupported browser or worker init failed).
     */
    send_message(msg) {
        if (this.port) {
            this.port.postMessage(msg);
        }
    }
}

/**
 * Post a message from an iframe up to its parent window.
 * @param {*} event - Any serializable payload.
 * @returns {void}
 * @example
 * // Inside iframe:
 * post({ type: "publish", topic: "ui/ready" });
 */
function post(event) {
    window.parent.postMessage(event, window.origin);
}

/**
 * Register a window `"message"` event listener.
 * @param {(evt: MessageEvent) => void} fn - Callback invoked on message events.
 * @returns {void}
 * @example
 * register_event_listner((e) => console.log("message:", e.data));
 */
function register_event_listner(fn) {
    window.addEventListener("message", fn);
}

/**
 * Update the visible web version string in the footer/header.
 * @returns {void}
 * @note Adjust the string here during releases to keep UI in sync with backend build.
 */
function show_version() {
    document.getElementById("web_ver").innerHTML =
        "w704 1v5 19/09/2025 - Copyright &copy; 2025 ETL Systems Ltd.";
}
