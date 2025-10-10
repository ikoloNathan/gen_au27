/* 
 * File:   home.js
 * Author: Nathan.Ikolo
 *
 * Created on December 05, 2023, 12:04 PM
 */
const fiber_option = ["External", "Fibre"];
const module_10MHz_option = ["Disabled", "Enabled"];
const button_values = [fiber_option, module_10MHz_option, module_10MHz_option, module_10MHz_option];
fetch_control_id = "";
var temp_value;

class Module {
    constructor(Doc, bodyId) {
        this.Document = Doc;
        this.main = this.Document.getElementById(bodyId);
		this.fetch_control = this.fetch_control.bind(this);
		this.add_control_event_listener = this.add_control_event_listener.bind(this);
		this.write_attenuation = this.write_attenuation.bind(this);
		this.write_10MHz_input_path = this.write_10MHz_input_path.bind(this);
		this.write_10MHz_ref_out = this.write_10MHz_ref_out.bind(this);
		this.write_10MHz_tx_path = this.write_10MHz_tx_path.bind(this);
		this.write_10MHz_rx_path = this.write_10MHz_rx_path.bind(this);
		this.fetch_info = this.fetch_info.bind(this);
        this.fetch_module_monitoring = this.fetch_module_monitoring.bind(this);
		this.onclickGain = this.onclickGain.bind(this);
		this.set_cell_monit = this.set_cell_monit.bind(this);
		this.onclickRFindet = this.onclickRFindet.bind(this);
		this.doRFindetlo = this.doRFindetlo.bind(this);
		this.doRFindethi = this.doRFindethi.bind(this);
		this.doRFdethi = this.doRFdethi.bind(this);
		
        var slot_id_map = new Map();
        slot_id_map.set("S1", "001");
        slot_id_map.set("S2", "002");
        slot_id_map.set("S3", "003");
        slot_id_map.set("S4", "004");
        slot_id_map.set("CPU", "005");
		this.status_flags = ["OK", "INITIALISATION", "MAINTENANCE", "FAULT"];
        this.page_name = "";
		
		this.gain_set = this.Document.getElementById("GainSet");
		this.id_10MHz_input_path_div = this.Document.getElementById("id_10MHz_input_path_div");
		this.id_10MHz_ref_out_div = this.Document.getElementById("id_10MHz_ref_out_div");
		this.id_10MHz_tx_path_div = this.Document.getElementById("id_10MHz_tx_path_div");
		this.id_10MHz_rx_path_div = this.Document.getElementById("id_10MHz_rx_path_div");
		this.InputPowerLimits = this.Document.getElementById("InputPowerLimits");

		const urlParams = new URLSearchParams(window.location.search);
        this.slot_id = slot_id_map.get(urlParams.get('slot_id'));
		this.fetch_control();
        this.fetch_module_monitoring();
        this.fetch_info();
		this.startInterval();
		this.add_control_event_listener();
    }

	startInterval() {
        fetch_control_id = setInterval(this.fetch_control, 2000);
    }

    restartInterval() {
        clearInterval(fetch_control_id); // Clear the existing interval
        this.startInterval(); // Start a new interval
    }

	add_control_event_listener() {
		//this.gain_set.addEventListener('keypress', this.write_attenuation);
        //this.gain_set.addEventListener('keydown', this.write_attenuation);
        this.gain_set.addEventListener('click', this.onclickGain);
		this.id_10MHz_input_path_div.addEventListener("click", this.write_10MHz_input_path);
		this.id_10MHz_ref_out_div.addEventListener("click", this.write_10MHz_ref_out);
		this.id_10MHz_tx_path_div.addEventListener("click", this.write_10MHz_tx_path);
		this.id_10MHz_rx_path_div.addEventListener("click", this.write_10MHz_rx_path);
		this.InputPowerLimits.addEventListener('click', this.onclickRFindet);
    }

	postrcm(rcm)
	{
		this.Document.post("../rcm.txt", rcm);
		setTimeout(this.fetch_control, 500);
		this.restartInterval();
	}

	onclickGain()
	{
		etlPrompt('Enter gain in dB:', null, 'digital', this.write_attenuation);
	}

	onclickRFindet()
	{
		etlPrompt('Enter Optical In lower limit in dBm:', null, 'digital', this.doRFindetlo);
	}

	doRFindetlo(value)
	{
		if (value != "")
		{
			temp_value = value;
		
			etlPrompt('Enter Optical In higher limit in dBm:', null, 'digital', this.doRFindethi);
		}
	}

	doRFindethi(value)
	{
		this.doRFdethi(value, 'I');
	}

	doRFdethi(value, inout_char)
	{
		var value_hundredthsdB, url, inout, syml = '+', symh = '+';
		var limitlo, limithi;
		
		if (value == "")
			return;

		limitlo = Number(temp_value);
		limithi = Number(value);

		if (limitlo < -99)
		{
			limitlo = -99;
		}
		else if (limitlo > 99)
		{
			limitlo = 99;
		}

		if (limithi < -99)
		{
			limithi = -99;
		}
		else if (limithi > 99)
		{
			limithi = 99;
		}

		if (limitlo > limithi)
		{
			etlAlert("Error, invalid lower/higher limits");
			return;
		}

		if (limitlo < 0)
		{
			syml = '-';
		}
		
		if (limithi < 0)
		{
			symh = '-';
		}

		url = "{MOD"+this.slot_id+inout_char+"LS,001" + syml + ("0000" + parseInt(Math.abs(limitlo * 100))).slice(-4) + symh + ("0000" + parseInt(Math.abs(limithi * 100))).slice(-4) + "}";
		this.postrcm(url);
	}

    write_attenuation(value) 
	{
		if (value == "")
			return;
		
        var command = "GAS,001";
        var value1 = parseInt(parseFloat(value) * 100);
		if (value1 < -9999)
		{
			value1 = -9999;
		}
		else if (value1 > 9999)
		{
			value1 = 9999;
		}
		var value_str = (Math.abs(value1) < 10) ? ("000") + Math.abs(value1) : (Math.abs(value1) < 100) ? ("00") + Math.abs(value1) : (Math.abs(value1) < 1000) ? ("0") + Math.abs(value1) : Math.abs(value1);
        var sign = value1 > 0 ? "+" : "-";
        var rcm = "{MOD" + this.slot_id + command + sign + value_str + "}";
        this.postrcm(rcm);
    }

    write_10MHz_input_path() {
        var command = "PES,000";
        var button = this.Document.getElementById("id_10MHz_input_path_div");
        for (let i = 0; i < fiber_option.length; i++) {
            if (button.innerHTML == fiber_option[i]) {
                button.innerHTML = fiber_option[i ^ 1];
                var value = (i ^ 1)?'E':'D';
                var rcm = "{MOD" + this.slot_id + command + value + "}";
                this.postrcm(rcm);
                break;
            }
        }
    }

    write_10MHz_ref_out() {
        var command = "PES,003";
        var button = this.Document.getElementById("id_10MHz_ref_out_div");
        for (let i = 0; i < module_10MHz_option.length; i++) {
            if (button.innerHTML == module_10MHz_option[i]) {
                button.innerHTML = module_10MHz_option[i ^ 1];
				var value = (i ^ 1)?'E':'D';
                var rcm = "{MOD" + this.slot_id + command + value + "}";
                this.postrcm(rcm);
                break;
            }
        }
    }

    write_10MHz_tx_path() {
        var command = "PES,001";
        var button = this.Document.getElementById("id_10MHz_tx_path_div");
        for (let i = 0; i < module_10MHz_option.length; i++) {
            if (button.innerHTML == module_10MHz_option[i]) {
                button.innerHTML = module_10MHz_option[i ^ 1];
                var value = (i ^ 1)?'E':'D';
                var rcm = "{MOD" + this.slot_id + command + value + "}";
                this.postrcm(rcm);
                break;
            }
        }

    }

    write_10MHz_rx_path() {
        var command = "PES,002";
        var button = this.Document.getElementById("id_10MHz_rx_path_div");
        for (let i = 0; i < module_10MHz_option.length; i++) {
            if (button.innerHTML == module_10MHz_option[i]) {
                button.innerHTML = module_10MHz_option[i ^ 1];
                var value = (i ^ 1)?'E':'D';
                var rcm = "{MOD" + this.slot_id + command + value + "}";
                this.postrcm(rcm);
                break;
            }
        }
    }

    fetch_control() {
        fetch("../module_control.json", {
                method: 'GET',
                cache: 'no-cache', // Specify 'no-cache' to prevent caching
            })
            .then((response) => response.json())
            .then((item) => {
				var slot_index = parseInt(this.slot_id) - 1;
                this.Document.getElementById("GainSet").value = parseFloat(item[slot_index].gain).toFixed(2) + "dB";
                var button = [this.Document.getElementById("id_10MHz_input_path_div"), this.Document.getElementById("id_10MHz_tx_path_div"), this.Document.getElementById("id_10MHz_rx_path_div"), this.Document.getElementById("id_10MHz_ref_out_div")];
                var en_10MHz = parseInt(item[slot_index].en_10MHz);
                for (let i = 0; i < 4; i++) {
                    if (((en_10MHz >> i) & 1) === 1) {
                        //if (button[i].classList.contains(button_values[i][0])) {
                        //    button[i].classList.remove(button_values[i][0]);
                        //}
                        //button[i].classList.add(button_values[i][1]);
                        button[i].innerHTML = button_values[i][1];
                    } else {
                        //if (button[i].classList.contains(button_values[i][1])) {
                        //    button[i].classList.remove(button_values[i][1]);
                        //}
                        //button[i].classList.add(button_values[i][0]);
                        button[i].innerHTML = button_values[i][0];
                    }
                }
				if (this.Document.getElementById("id_10MHz_input_path_div").innerHTML == fiber_option[0])
				{
					this.Document.getElementById("id_10MHz_ref_out_div").className = "value disabled";
					this.Document.getElementById("id_10MHz_ref_out_div").removeEventListener("click", this.write_10MHz_ref_out);
				}
				else
				{
					this.Document.getElementById("id_10MHz_ref_out_div").className = "value key";
					this.Document.getElementById("id_10MHz_ref_out_div").addEventListener("click", this.write_10MHz_ref_out);
				}
				this.Document.getElementById("InputPowerLimits").innerHTML = parseInt(item[slot_index].in_lim_lo)/100 + "|" + parseInt(item[slot_index].in_lim_hi)/100 + "dBm";
            })
            .catch((error) => console.log("Error fetching data:", error));
    }

	set_cell_monit(cell, err, inner, showOK)
	{
		cell.innerHTML = inner;
		if (err)
		{
			cell.className = "value monit alarm";
		}
		else
		{
			cell.className = "value monit";
			if (showOK)
				cell.innerHTML = "OK";
		}
	}

    fetch_module_monitoring() {
        fetch("../module_monitoring.json", {
                method: 'GET',
                cache: 'no-cache', // Specify 'no-cache' to prevent caching
            })
            .then((response) => response.json())
            .then((item) => {
				var slot_index = parseInt(this.slot_id) - 1;
				var cell;
				
				this.set_cell_monit(document.getElementById("id_monitoring_vbus"),
					item[slot_index].v12v_e,
					parseFloat(item[slot_index].Vbus).toFixed(1) + "V",
					1);
				
				this.set_cell_monit(document.getElementById("id_monitoring_vbus2"),
					item[slot_index].v12v2_e,
					parseFloat(item[slot_index].Vbus2).toFixed(1) + "V",
					1);
				
				this.set_cell_monit(document.getElementById("id_monitoring_ibus"),
					item[slot_index].v5v_e,
					(parseInt(item[slot_index].Ibus) / 1000).toFixed(1) + "V",
					1);

				this.set_cell_monit(document.getElementById("id_monitoring_ip"),
					item[slot_index].i5v_e,
					item[slot_index].RFip + "mA",
					1);

				this.set_cell_monit(document.getElementById("id_monitoring_temp"),
					item[slot_index].temp_e,
					parseFloat(item[slot_index].Temp).toFixed(1) + "°C",
					1);

				this.set_cell_monit(document.getElementById("id_monitoring_op"),
					item[slot_index].i3v3_e,
					item[slot_index].RFop + "mA",
					1);

				this.set_cell_monit(document.getElementById("id_monitoring_optical"),
					item[slot_index].rfip_e,
					parseFloat(item[slot_index].pd).toFixed(1) + "dBm",
					0);
				
				cell = document.getElementById("module_top_status");
				cell.innerHTML = this.status_flags[item[slot_index].status];
				if (item[slot_index].status)
					cell.style.color = '#f00';
				else
					cell.style.color = '#fff';
            })
            .catch((error) => console.log("Error fetching data:", error));
    }

    fetch_info() {
        fetch("../module_info.json", {
                method: 'GET',
                cache: 'no-cache', // Specify 'no-cache' to prevent caching
            })
            .then((response) => response.json())
            .then((item) => {
				var slot_index = parseInt(this.slot_id) - 1;
                document.getElementById("address").innerHTML = "Slot " + parseInt(this.slot_id);
                document.getElementById("release_date").innerHTML = item[slot_index].rel_date;
				document.getElementById("warranty_date").innerHTML = item[slot_index].warranty_date;
				document.getElementById("hours_operation").innerHTML = item[slot_index].op_hrs;
                document.getElementById("id_info_model_number").innerHTML = item[slot_index].model_number;
                document.getElementById("id_info_serial_number").innerHTML = "S/N: " + item[slot_index].sn;
                document.getElementById("id_info_build_number").innerHTML = "SW: " + item[slot_index].app_id + " " + item[slot_index].app_version;
				if ((item[slot_index].model_number == "SRY-1T1R-9510")
					|| (item[slot_index].model_number == "SRY-0T4R-9505")
					|| (item[slot_index].model_number == "SRY-4T0R-9016")
					|| (item[slot_index].model_number == "SRY-0T4R-9017")
					|| (item[slot_index].model_number == "SRY-1T1R-9011")
					|| (item[slot_index].model_number == "SRY-2T2R-9512")
					)
				{
					// SRY_IDU
					// SRY_ODU_NO_10MHZ
					// SRY_ODU_10MHZ_TX
					this.id_10MHz_input_path_div.removeEventListener("click", this.write_10MHz_input_path);
					this.id_10MHz_input_path_div.className = "value disabled";
					this.id_10MHz_ref_out_div.removeEventListener("click", this.write_10MHz_ref_out);
					this.id_10MHz_ref_out_div.className = "value disabled";
					
					if ((item[slot_index].model_number == "SRY-1T1R-9510")
						|| (item[slot_index].model_number == "SRY-0T4R-9505")
						|| (item[slot_index].model_number == "SRY-2T2R-9512")
						)
					{
						// SRY_IDU
						document.getElementById("id_lbl_vbus2").style.display = '';
						document.getElementById("id_monitoring_vbus2").style.display = '';
						
						document.getElementById("lbl_mon_optical").innerHTML = "10MHz Optical Out:"
					}
					else if (item[slot_index].model_number == "SRY-1T1R-9011")
					{
						// SRY_ODU_10MHZ_TX
						document.getElementById("lbl_mon_optical").innerHTML = "10MHz Optical Out:"
					}
					else
					{
						// SRY_ODU_NO_10MHZ
						document.getElementById("div_mon_optical").style.display = "none";
					}
				}
				else
				{
					// SRY_ODU
					document.getElementById("lbl_mon_optical").innerHTML = "10MHz Optical In:"
					document.getElementById("div_ctrl_optical").style.display = "";
				}
				
            })
            .catch((error) => console.log("Error fetching data:", error));
    }
}