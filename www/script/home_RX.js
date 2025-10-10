/* 
 * File:   home.js
 * Author: Nathan.Ikolo
 *
 * Created on December 05, 2023, 12:04 PM
 */


const gain_mode_select = new Map();
gain_mode_select.set("Manual", "OMS,001M");
gain_mode_select.set("AGC", "OMS,001A");
gain_mode_select.set("Fixed", "OMS,001F");

fetch_control_id = "";
var temp_value;

class Module {
    constructor(Doc, bodyId) {
        this.Document = Doc;
        this.main = this.Document.getElementById(bodyId);
        this.fetch_control = this.fetch_control.bind(this);
        this.add_control_event_listener = this.add_control_event_listener.bind(this);
        this.write_attenuation = this.write_attenuation.bind(this);
        this.write_gain_mode = this.write_gain_mode.bind(this);
        this.write_op = this.write_op.bind(this);
        this.fetch_info = this.fetch_info.bind(this);
		this.onclickRFindet = this.onclickRFindet.bind(this);
		this.onclickRFoutdet = this.onclickRFoutdet.bind(this);
		this.doRFindetlo = this.doRFindetlo.bind(this);
		this.doRFindethi = this.doRFindethi.bind(this);
		this.doRFoutdetlo = this.doRFoutdetlo.bind(this);
		this.doRFoutdethi = this.doRFoutdethi.bind(this);
		this.doRFdethi = this.doRFdethi.bind(this);
		this.onclickGain = this.onclickGain.bind(this);
		this.onclickAGCPwr = this.onclickAGCPwr.bind(this); 
        //this.update_status = this.update_status.bind(this);
        this.fetch_module_monitoring = this.fetch_module_monitoring.bind(this);

        var slot_id_map = new Map();
        slot_id_map.set("S1", "001");
        slot_id_map.set("S2", "002");
        slot_id_map.set("S3", "003");
        slot_id_map.set("S4", "004");
        slot_id_map.set("CPU", "005");
        this.status_flags = ["OK", "INITIALISATION", "MAINTENANCE", "FAULT"];
        this.page_name = "";
        this.agc = new Array(3);
        this.lnb_voltage = new Array(3);
        this.lnb_tone = new Array(2);

        this.agc[0] = this.Document.getElementById("manual_mode");
        this.agc[1] = this.Document.getElementById("agc_mode");
        this.agc[2] = this.Document.getElementById("fixed_mode");
        this.gain_set = this.Document.getElementById("GainSet");
        this.OutputPowerSet = this.Document.getElementById("OutputPowerSet");
		this.InputPowerLimits = this.Document.getElementById("InputPowerLimits");
		this.OutputPowerLimits = this.Document.getElementById("OutputPowerLimits");

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
        this.agc[0].addEventListener('click', this.write_gain_mode);
        this.agc[1].addEventListener('click', this.write_gain_mode);
        this.agc[2].addEventListener('click', this.write_gain_mode);
        //this.gain_set.addEventListener('keypress', this.write_attenuation);
        //this.gain_set.addEventListener('keydown', this.write_attenuation);
        this.gain_set.addEventListener('click', this.onclickGain);
        this.OutputPowerSet.addEventListener('click', this.onclickAGCPwr);
		this.InputPowerLimits.addEventListener('click', this.onclickRFindet);
		this.OutputPowerLimits.addEventListener('click', this.onclickRFoutdet);
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

	onclickAGCPwr()
	{
		etlPrompt('Enter RF Output Power in dBm:', null, 'digital', this.write_op);
	}

    write_op(value) 
	{
		if (value == "")
			return;

        var command = "RPS,001";
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

    write_gain_mode(event) {
        const element = event.target;
        const keys = Array.from(gain_mode_select.keys());
        var value = element.textContent;
        for (let i = 0; i < keys.length; i++) {
            if (this.agc[i].classList.contains("selected")) {
                this.agc[i].classList.remove("selected");
            }
            if (value == keys[i]) {
                this.agc[i].classList.add("selected");
        }
    }

        var rcm = "{MOD" + this.slot_id + gain_mode_select.get(value) + "}";
        this.postrcm(rcm);
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

	onclickRFoutdet()
	{
		etlPrompt('Enter RF Output lower limit in dBm:', null, 'digital', this.doRFoutdetlo);
	}

	doRFoutdetlo(value)
	{
		if (value != "")
		{
			temp_value = value;
		
			etlPrompt('Enter RF Output higher limit in dBm:', null, 'digital', this.doRFoutdethi);
		}
	}

	doRFoutdethi(value)
	{
		this.doRFdethi(value, 'O');
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

    fetch_control() {
        fetch("../module_control.json", {
                method: 'GET',
                cache: 'no-cache', // Specify 'no-cache' to prevent caching
            })
            .then((response) => response.json())
            .then((item) => {
				var slot_index = parseInt(this.slot_id) - 1;
                this.Document.getElementById("GainSet").value = parseFloat(item[slot_index].gain).toFixed(2) + "dB";
                for (let i = 0; i < 3; i++) {
                    if (this.agc[i].classList.contains("selected")) {
                        this.agc[i].classList.remove("selected");
                        }
                    }
                this.agc[item[slot_index].agc].classList.add("selected");
				switch (item[slot_index].agc)
				{
					case 0: // manual
					this.gain_set.disabled = false;
                    this.gain_set.readOnly = false;
					this.gain_set.classList.remove("disabled");
					this.OutputPowerSet.classList.add("disabled");
					break;
					case 1: // AGC
					this.gain_set.disabled = true;
                    this.gain_set.readOnly = true;
					this.gain_set.classList.add("disabled");
					this.OutputPowerSet.classList.remove("disabled");
					break;
					case 2: // Fixed
					this.gain_set.disabled = true;
                    this.gain_set.readOnly = true;
					this.gain_set.classList.add("disabled");
					this.OutputPowerSet.classList.add("disabled");
					break;
				}
				this.Document.getElementById("InputPowerLimits").innerHTML = parseInt(item[slot_index].in_lim_lo)/100 + "|" + parseInt(item[slot_index].in_lim_hi)/100 + "dBm";
				this.Document.getElementById("OutputPowerLimits").innerHTML = parseInt(item[slot_index].out_lim_lo)/100 + "|" + parseInt(item[slot_index].out_lim_hi)/100 + "dBm";
				this.OutputPowerSet.value = parseFloat(item[slot_index].agc_pwr)/100 + "dBm";
            })
            .catch((error) => console.log("Error fetching data:", error));
    }

    fetch_module_monitoring() {
        fetch("../module_monitoring.json", {
                method: 'GET',
                cache: 'no-cache',
            })
            .then((response) => response.json())
            .then((item) => {
				var slot_index = parseInt(this.slot_id) - 1;
				var cell;
				if (item[slot_index].v5v_e)
				{
					document.getElementById("vbus").className = "value monit alarm";
					document.getElementById("vbus").innerHTML = parseFloat(item[slot_index].Vbus).toFixed(1) + "V";
				}
				else
				{
					document.getElementById("vbus").className = "value monit";
					document.getElementById("vbus").innerHTML = "OK";
				}
                
				if (item[slot_index].i5v_e)
				{
					document.getElementById("ibus").className = "value monit alarm";
					document.getElementById("ibus").innerHTML = item[slot_index].Ibus + "mV";
				}
				else
				{
					document.getElementById("ibus").className = "value monit";
					document.getElementById("ibus").innerHTML = "OK";
				}
				if (item[slot_index].rfop_e)
				{
					document.getElementById("OutputPower").className = "value monit alarm";
				}
				else
				{
					document.getElementById("OutputPower").className = "value monit";
				}
                document.getElementById("OutputPower").innerHTML = parseFloat(item[slot_index].RFop).toFixed(1) + "dBm";
				if (item[slot_index].temp_e)
				{
					document.getElementById("temp").className = "value monit alarm";
					document.getElementById("temp").innerHTML = parseFloat(item[slot_index].Temp).toFixed(1) + "°C";
				}
				else
				{
					document.getElementById("temp").className = "value monit";
					document.getElementById("temp").innerHTML = "OK";
				}
				if (item[slot_index].rfip_e)
				{
					document.getElementById("OpticalInputRead").className = "value monit alarm";
				}
				else
				{
					document.getElementById("OpticalInputRead").className = "value monit";
				}
                document.getElementById("OpticalInputRead").innerHTML = parseFloat(item[slot_index].RFip).toFixed(1) + "dBm";
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
            })
            .catch((error) => console.log("Error fetching data:", error));
    }
}