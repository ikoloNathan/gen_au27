/* 
 * File:   main.js
 * Author: Nathan.Ikolo
 *
 * Created on December 01, 2023, 14:11 PM
 */

class Doc {
    constructor() {
            this.Document = document;
            this.xhr = new XMLHttpRequest();
        }
        /**
         *
         * @param {*} id
         * @returns
         */
    getElementById(id) {
        try {
            var element = this.Document.getElementById(id);
            if (element === null) {
                throw new Error(`Element with ID '${id}' not found.`);
            }
            return element;
        } catch (error) {
            // console.error("Error:", error.message);
            // Return null or another default value if unsuccessful
            return null;
        }
    }

    post(apiUrl, postData) {
        // Use the Fetch API to make a POST request
        fetch(apiUrl, {
                method: 'POST',
                cache: 'no-cache',
                headers: {
                    'Content-Type': 'text/plain' // Specify the content type as JSON
                        // You may also need to include other headers if required by the API
                },
                body: (postData) // Convert the data object to a JSON string
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
            })
            .catch(error => {
                // Handle errors during the fetch
                console.error('Error during fetch:', error);
            });
    }

    /**
     *
     * @param {string literal to target element id} id
     * @param {JSON object} attribute
     */
    setAttribute(id, attribute) {
        try {
            var element = this.getElementById(id); //safe access
            if (element !== null) {
                element.setAttribute(attribute.name, attribute.value);
            } else {
                throw new Error(`Element with ID '${id}' not found attribute not set.`);
            }
        } catch (error) {
            console.error("Error:", error.message);
        }
    }

    /**
     *
     * @param {*} id
     * @returns
     */
    createElement(type, id, className, body) {
        try {
            var element = this.Document.createElement(type);
            if (id !== null) {
                element.id = id;
            }
            if (className !== null) {
                element.className = className;
            }
            body.appendChild(element);
            return element;
        } catch (error) {
            console.log("Error:", error.message);
            return null;
        }
    }

    set_intervals_callbacks(callback, timer_interval) {
        return setInterval(callback, timer_interval);
    }

    clear_intervals_callbacks(callback_id) {
        return clearInterval(callback_id);
    }
    clear_select(prefix, element_id) {
        for (let j = 0; j < element_id.length; j++) {
            var button = this.Document.getElementById(prefix + element_id[j]);
            var list = Array.from(button.classList);
            for (let k = 0; k < list.length; k++) {
                if (list[k] === 'selected') {
                    button.classList.remove('selected');
                }
            }
        }
    }
}

/*
    IDs : main:0, iframe:1, server: 2
    payload: {...}
*/

const settings_id = ['sum', 'config', 'log', 'load'];
const settings_names = ['CONTROL', 'CONFIG', 'LOG', 'UPGRADE'];
const settings_links = ['./setting/summary.htm', './setting/settings.htm', './setting/log.htm', './setting/upload.htm'];
type = ['TX', 'TX', 'RX', 'RX', 'CPU'];

class Body {
    constructor(select) {
        this.worker = null;
        this.port = null;
        this.slots = [];
        this.sidebar_dividers_outer = [];
        this.sidebar_dividers_inner = [];
        this.sidebar_table_slot_cells = []; // dynamic cell array
        this.sidebar_settings = []
        this.sidebar_settings_ctrl = []
        this.sidebar_settings_div_outer = [];
        this.sidebar_settings_div_inner = [];
        this.module_summary = [];
        this.Document = new Doc();
        this.module_ctrl = this.Document.getElementById(select.modules);
        this.settings = this.Document.getElementById(select.settings);
        this.main = this.Document.getElementById(select.display_panel);
        this.main_summary_cells = [];
        this.show_settings = this.load.bind(this);
        this.show_modules = this.show_modules.bind(this);
        this.send_message = this.send_message.bind(this);
        this.test_parent_post = this.test_parent_post.bind(this);
        this.on_message_worker = this.on_message_worker.bind(this);
        this.broker = this.broker.bind(this);
    }
    async show_modules() {
        this.Document.Document.addEventListener('DOMContentLoaded', () => {
            /**
             * Create dynamic table to insert into sidebar to manage and align hexagon slot elements
             */
            fetch("sidebar.json", {
                    method: 'GET',
                    cache: 'no-cache', // Specify 'no-cache' to prevent caching
                })
                .then((response) => response.json())
                .then((jsonArray) => {
            		this.sidebar_table_slot = this.Document.createElement("table", null, null, this.module_ctrl); // table
            		this.sidebar_table_slot_row = this.sidebar_table_slot.insertRow(-1); // single row 
            		this.sidebar_table_slot.setAttribute("padding-top", "30px");
                    let slots_names = [];
                    for (let i = 0, j = 0; i < jsonArray.length; i++) {
                        if (jsonArray[i].present === 1)
                        {
                            slots_names[j++] = jsonArray[i].slot;
                        }
                    }

                    jsonArray.forEach((item, i) => {
                        if (item.present === 1)
                        {
                            this.sidebar_dividers_outer[i] = this.Document.createElement("div", "id_outer_hex_small_" + item.slot, "class_outer_hexagon_small Operating", this.module_ctrl);
                            this.sidebar_dividers_inner[i] = this.Document.createElement("div", "id_inner_hex_small_" + item.slot, "class_inner_hexagon_small enabled", this.module_ctrl);
                			this.sidebar_table_slot_cells[i] = this.sidebar_table_slot_row.insertCell(i);
                            this.slots[i] = this.Document.createElement("button", "id_slot_button_" + item.slot, "class_slot_ctrl enabled", this.module_ctrl);
                            this.slots[i].innerHTML = item.slot;
                			this.slots[i].addEventListener("click", function() {
                    			var Document = new Doc();
                                Document.clear_select("id_inner_hex_small_", slots_names);
                    			Document.clear_select("id_inner_hex_medium_", settings_id);
                                Document.getElementById("id_inner_hex_small_" + item.slot).classList.add('selected');
                                Document.getElementById("main_page").setAttribute("page", "module_" + item.slot);
                                Document.getElementById("main_page").setAttribute("src", "./module/home_" + item.type + ".htm?slot_id=" + item.slot);

                			});

                        } else {
                            this.sidebar_dividers_outer[i] = this.Document.createElement("div", "id_outer_hex_small_" + item.slot, "class_outer_hexagon_small disabled", this.module_ctrl);
                            this.sidebar_dividers_inner[i] = this.Document.createElement("div", "id_inner_hex_small_" + item.slot, "class_inner_hexagon_small disabled", this.module_ctrl);
                            this.sidebar_table_slot_cells[i] = this.sidebar_table_slot_row.insertCell(i);
                            this.slots[i] = this.Document.createElement("button", "id_slot_button_" + type[item.slot], "class_slot_ctrl disabled", this.module_ctrl);
                            this.slots[i].innerHTML = item.slot;
                        }
						this.sidebar_dividers_inner[i].appendChild(this.slots[i]);
						this.sidebar_dividers_outer[i].appendChild(this.sidebar_dividers_inner[i]);
						this.sidebar_table_slot_cells[i].appendChild(this.sidebar_dividers_outer[i]);
                    });
                })
                .catch((error) => console.log("Error fetching data:", error));
        });
    }

    load() {
        this.Document.Document.addEventListener('DOMContentLoaded', () => {
            window.addEventListener("message", (event) => {
            if (event.origin === window.origin) {
                this.broker(event.data);
            }
            });
            if ('SharedWorker' in window) {
                try {
                    this.worker = new SharedWorker('worker.js', { name: 'worker'});
                    this.port = this.worker.port;
                    this.port.start();
                    this.port.onmessage = this.on_message_worker;
                } catch (err) {
                    console.error('Failed to start SharedWorker:', err);
                }
            }
            this.sidebar_setting_table = this.Document.createElement("table", null, null, this.settings);
            this.sidebar_setting_cell_upper = new Array(2);
			this.sidebar_setting_cell_lower = new Array(2);
            this.sidebar_settings_rows = new Array(2);
            this.sidebar_settings_rows[0] = this.sidebar_setting_table.insertRow();
            this.sidebar_settings_rows[1] = this.sidebar_setting_table.insertRow();
            this.sidebar_setting_cell_upper[0] = this.sidebar_settings_rows[0].insertCell(0);
            this.sidebar_setting_cell_upper[1] = this.sidebar_settings_rows[0].insertCell(1);
            this.sidebar_setting_cell_lower[0] = this.sidebar_settings_rows[1].insertCell(0);
			this.sidebar_setting_cell_lower[1] = this.sidebar_settings_rows[1].insertCell(1);
            this.sidebar_settings_rows[0].className = "class_setting_upper";
            this.sidebar_settings_rows[1].className = "class_setting_lower";

            for (let i = 0; i < settings_id.length; i++) {
                this.sidebar_settings_div_outer[i] = this.Document.createElement("div", "id_outer_hex_medium_" + settings_id[i], "class_outer_hexagon_medium Operating", this.settings);
                this.sidebar_settings_div_inner[i] = this.Document.createElement("div", "id_inner_hex_medium_" + settings_id[i], "class_inner_hexagon_medium", this.settings);
                this.sidebar_settings_ctrl[i] = this.Document.createElement("button", "id_setting_" + settings_id[i], "class_setting_ctrl", this.settings);
                this.sidebar_settings_ctrl[i].innerHTML = settings_names[i];
                this.sidebar_settings_ctrl[i].addEventListener("click", function() {
                    var Document = new Doc();
                    Document.clear_select("id_inner_hex_medium_", settings_id);
                    Document.getElementById("id_inner_hex_medium_" + settings_id[i]).classList.add('selected');
                    Document.getElementById("main_page").setAttribute("page", "setting_" + settings_id[i]);
                    Document.getElementById("main_page").setAttribute("src", settings_links[i]);

                });
                this.sidebar_settings_div_inner[i].appendChild(this.sidebar_settings_ctrl[i]);
                this.sidebar_settings_div_outer[i].appendChild(this.sidebar_settings_div_inner[i]);
            }
            this.sidebar_setting_cell_upper[0].appendChild(this.sidebar_settings_div_outer[0]);
            this.sidebar_setting_cell_upper[1].appendChild(this.sidebar_settings_div_outer[1]);
            this.sidebar_setting_cell_lower[0].appendChild(this.sidebar_settings_div_outer[2]);
			this.sidebar_setting_cell_lower[1].appendChild(this.sidebar_settings_div_outer[3]);
        });
    }
    test_parent_post(){
        console.log("posting from parent");
        const iframe = document.getElementById("main_page");
        iframe.contentWindow.postMessage({
        type: "publish",
        topic: "chat/messages",
        payload: { message: "Hello from parent!" }
        }, window.origin);
    }
    
    /**
     * on message event listener to the worker event from the server
     * @param {*} e 
     */
    on_message_worker(e){
        const { type,status, payload } = e.data;
        switch(type){
            case "ws-status":
                switch(status){
                    case "open":
                        {   //handle websocket connection opened
                            const statusEl = document.getElementById('status');
                            statusEl.textContent = "connected";
                        }
                        break;
                    case "closed":
                    case "disconnected":
                    case "closing":
                        {   //handle closed connection
                            const statusEl = document.getElementById('status');
                            statusEl.textContent = "closed";
                        }
                        break;
                }

                break;
            case "tab-broadcast":
            case "server-message":
            case "message-main":
                {
                    this.broker(payload);
                }
                break;
        }
    }

    broker(payload){
        const {type,dest,data} = payload;
        if(type === "ping"){
            console.log("server sent: ",payload);
        }else{
            switch(dest){
                case 0: // addressed to the main page
                    break;
                case 1: // sending to the iframe
                {
                    const iframe = document.getElementById("main_page");
                    iframe.contentWindow.postMessage(payload, window.origin);
                }
                    break;
                    default:// send this message to server
                    this.send_message({type:"client-message",payload:payload});
                    break;

            }
        }
    }

    send_message(msg) {
        if (this.port) {
            this.port.postMessage(msg);
        }
    }

}

function post(event){
     window.parent.postMessage(event, window.origin);
}

function register_event_listner(fn){
    window.addEventListener("message", fn);
}

function show_version()
{
	document.getElementById("web_ver").innerHTML = "w704 1v5 19/09/2025 - Copyright &copy; 2025 ETL Systems Ltd.";
}
