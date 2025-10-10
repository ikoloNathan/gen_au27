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

const slotId = ['S1', 'S2', 'S3', 'S4', 'CPU'];
const settings_id = ['sum', 'config', 'log', 'load'];
const settings_names = ['SUM', 'CONFIG', 'LOG', 'UPGRADE'];
const settings_links = ['./setting/summary.htm', './setting/settings.htm', './setting/log.htm', './setting/upload.htm'];
const status_summary = ["Operating", "Bootloader", "Debug", "Error"];
const status_flags = ["OK", "INITIALISATION", "MAINTENANCE", "FAULT"];
type = ['TX', 'TX', 'RX', 'RX', 'CPU'];

class Body {
    constructor(select) {

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
        this.show_modules = this.show_modules.bind(this);
        this.show_settings = this.show_settings.bind(this);
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
                                var rcm = "{MOD005SIS," + item.slot + "}";
                                Document.clear_select("id_inner_hex_small_", slots_names);
                    			Document.clear_select("id_inner_hex_medium_", settings_id);
                                Document.getElementById("id_inner_hex_small_" + item.slot).classList.add('selected');
                    			Document.post("../selected_slot.json", rcm);
                    			//var status = "{MOD00" + (i + 1) + "ST?,0}";
                    			//Document.post('../rcm.txt', status);
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
                    //setInterval(this.update_sidebar, 2000);
                })
                .catch((error) => console.log("Error fetching data:", error));
        });
    }

    show_settings() {
        this.Document.Document.addEventListener('DOMContentLoaded', () => {
            let slots_names = [];
            fetch("../sidebar.json", {
                    method: 'GET',
                    cache: 'no-cache', // Specify 'no-cache' to prevent caching
                })
                .then((response) => response.json())
                .then((jsonArray) => {
                    for (let i = 0, j = 0; i < jsonArray.length; i++) {
                        if (jsonArray[i].present === 1)
                        {
                            slots_names[j++] = jsonArray[i].slot;
                        }
                    }
                });
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
                    Document.clear_select("id_inner_hex_small_", slots_names);
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
	
    show_main_summary() {
        var table = [];
        var table_inner = [];

        var row = [];
        this.Document.Document.addEventListener('DOMContentLoaded', () => {
            fetch("../sidebar.json", {
                    method: 'GET',
                    cache: 'no-cache', // Specify 'no-cache' to prevent caching
                })
                .then((response) => response.json())
                .then((jsonArray) => {
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
                            this.module_summary[i] = this.Document.createElement("div", "id_summary_" + item.slot, "class_module_summary Operating", this.main);
			                this.module_summary[i].setAttribute("width", "100%");
			                this.module_summary[i].classList.add("Operating");
			                table[i] = this.Document.createElement("table", null, null, this.main);
			                table[i].setAttribute("width", "100%");
			                table[i].setAttribute("height", "100%");
			                row[i] = table[i].insertRow(-1);
			                this.main_summary_cells[i] = new Array(3);
			                this.main_summary_cells[i][0] = row[i].insertCell(0);
			                this.main_summary_cells[i][0].setAttribute("width", "400px");
			
			                table_inner[i] = new Array(3);
                            table_inner[i][0] = this.Document.createElement("div", "id_slot_name_" + item.slot, "class_slot_name Operating", this.main);
                			table_inner[i][0].classList.add("Operating");
                			table_inner[i][0].setAttribute("style", "line-height: 40px");

                			table_inner[i][1] = this.Document.createElement("div", null, "class_slot_name", this.main);
                            table_inner[i][1].innerHTML = item.slot;
                			table_inner[i][1].setAttribute("style", "line-height: 40px");
                            table_inner[i][2] = this.Document.createElement("div", "id_status_" + item.slot, "class_slot_name", this.main);
                			table_inner[i][2].setAttribute("style", "line-height: 40px");

			                this.main_summary_cells[i][0].appendChild(table_inner[i][0]);
			                this.main_summary_cells[i][0].appendChild(table_inner[i][1]);
			                this.main_summary_cells[i][0].appendChild(table_inner[i][2]);
			
			                this.main_summary_cells[i][1] = row[i].insertCell(1);
			                table_inner[i][0] = this.Document.createElement("div", null, "class_part_number", this.main);
                            table_inner[i][1] = this.Document.createElement("div", "id_part_number_" + item.slot, "class_part_number", this.main);
                            table_inner[i][2] = this.Document.createElement("div", "id_model_number_" + item.slot, "class_part_number", this.main);
			
			                this.main_summary_cells[i][1].className = "class_model_numbers Operating";
                            this.main_summary_cells[i][1].id = "id_model_numbers_" + item.slot;
			                this.main_summary_cells[i][1].appendChild(table_inner[i][0]);
			                this.main_summary_cells[i][1].appendChild(table_inner[i][1]);
			                this.main_summary_cells[i][1].appendChild(table_inner[i][2]);
			                this.module_summary[i].appendChild(table[i]);
			            }
			        });
					this.Document.createElement("div", "web_ver", null, this.main);
					document.getElementById("web_ver").style.cssText = "margin-top:5px; width:750px; color:#ccc; text-align:center; font-size: 12px;";
					show_version();
                });
        });
    }

    update_sidebar() {
        fetch("sidebar.json", {
                method: 'GET',
                cache: 'no-cache', // Specify 'no-cache' to prevent caching
            })
            .then((response) => response.json())
            .then((jsonArray) => {
				var sum_alarm = 0;
                jsonArray.forEach((item) => {
					if (item.present)
					{
						document.getElementById("id_outer_hex_small_" + item.slot).className = "class_outer_hexagon_small " + status_summary[item.summary];
						if (item.summary)
							sum_alarm = 1;
					}
					else
					{
						document.getElementById("id_outer_hex_small_" + item.slot).className = "class_outer_hexagon_small disabled";
					}
                });
				if (sum_alarm)
				{
					document.getElementById("id_header").className = "header Error";
					document.getElementById("id_outer_hex_medium_sum").className = "class_outer_hexagon_medium Error";
				}
				else
				{
					document.getElementById("id_header").className = "header";
					document.getElementById("id_outer_hex_medium_sum").className = "class_outer_hexagon_medium Operating";
				}
            })
            .catch((error) => console.log("Error fetching data:", error));
    }
    update_summary() {
        fetch("../summary.json", {
                method: 'GET',
                cache: 'no-cache', // Specify 'no-cache' to prevent caching
            })
            .then((response) => response.json())
            .then((jsonArray) => {
                jsonArray.forEach((item) => {
                    if (item.present === 1)
                    {
                        try {
		                    document.getElementById("id_slot_name_" + item.slot).innerHTML = item.name;
                            document.getElementById("id_status_" + item.slot).innerHTML = "Status:\t" + status_flags[item.summary];
		                    document.getElementById("id_model_number_" + item.slot).innerHTML = item.app_id;
		                    document.getElementById("id_part_number_" + item.slot).innerHTML = "S/N:" + item.sn;
		                    document.getElementById("id_summary_" + item.slot).className = "class_module_summary " + status_summary[item.summary];
		                    document.getElementById("id_slot_name_" + item.slot).className = "class_slot_name " + status_summary[item.summary];
		                    document.getElementById("id_model_numbers_" + item.slot).className = "class_model_numbers " + status_summary[item.summary];
                        } catch (error) {}
                    }
                });
            })
            .catch((error) => console.log("Error fetching data:", error));
    }
}

function show_version()
{
	document.getElementById("web_ver").innerHTML = "w704 1v5 19/09/2025 - Copyright &copy; 2025 ETL Systems Ltd.";
}
