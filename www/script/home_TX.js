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

const vlnb_select = new Map();
vlnb_select.set("Off", "LVS,001000");
vlnb_select.set("13V", "LVS,001133");
vlnb_select.set("188V", "LVS,001188");
vlnb_select.set("18V", "LVS,001183");

const vlnb_options = ["LNBoff", "LNB13v", "LNB13v", "LNB18v", "LNB18v"];

const lnb_tone_select = new Map();
lnb_tone_select.set("Off", "LTS,001D");
lnb_tone_select.set("On", "LTS,001E");

fetch_control_id = "";
var temp_value;

class Module {
    constructor(Doc, bodyId) {
        this.Document = Doc;
        this.main = this.Document.getElementById(bodyId);

        this.fetch_control = this.fetch_control.bind(this);
        this.add_control_event_listener = this.add_control_event_listener.bind(this);
        this.write_attenuation = this.write_attenuation.bind(this);
        this.write_lnb_voltage = this.write_lnb_voltage.bind(this);
        this.write_lnb_tone = this.write_lnb_tone.bind(this);
        this.write_gain_mode = this.write_gain_mode.bind(this);
        this.write_10MHz_state = this.write_10MHz_state.bind(this);
        this.fetch_info = this.fetch_info.bind(this);
		this.onclickRFindet = this.onclickRFindet.bind(this);
		this.doRFindetlo = this.doRFindetlo.bind(this);
		this.doRFindethi = this.doRFindethi.bind(this);
		this.onclickRFoutdet = this.onclickRFoutdet.bind(this);
		this.doRFoutdetlo = this.doRFoutdetlo.bind(this);
		this.doRFoutdethi = this.doRFoutdethi.bind(this);
		this.onclickLnbCurLim = this.onclickLnbCurLim.bind(this);
		this.doLnbCurLimlo = this.doLnbCurLimlo.bind(this);
		this.doLnbCurLimhi = this.doLnbCurLimhi.bind(this);
		this.onclickGain = this.onclickGain.bind(this);
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
        this.lnb_voltage = new Array(4);
        this.lnb_tone = new Array(2);

        this.agc[0] = this.Document.getElementById("manual_mode");
        this.agc[1] = this.Document.getElementById("agc_mode");
        this.agc[2] = this.Document.getElementById("fixed_mode");
        this.gain_set = this.Document.getElementById("GainSet");
        this.set_10MHz = this.Document.getElementById("_10MHz_status1");
        this.lnb_voltage[0] = this.Document.getElementById("LNBoff");
        this.lnb_voltage[1] = this.Document.getElementById("LNB13v");
        this.lnb_voltage[2] = this.Document.getElementById("LNB18v");
        this.lnb_voltage[3] = this.Document.getElementById("LNB18v");
        this.lnb_tone[0] = this.Document.getElementById("LNBToneOff");
        this.lnb_tone[1] = this.Document.getElementById("LNBToneOn");
		this.InputPowerLimits = this.Document.getElementById("InputPowerLimits");
		this.OutputPowerLimits = this.Document.getElementById("OutputPowerLimits");
		this.LNBcurrentLimits = this.Document.getElementById("LNBcurrentLimits");
        this.fetch_control();
        this.fetch_module_monitoring();
        this.fetch_info();
        const urlParams = new URLSearchParams(window.location.search);
        this.slot_id = slot_id_map.get(urlParams.get('slot_id'));


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
        this.set_10MHz.addEventListener('click', this.write_10MHz_state);
        this.lnb_voltage[0].addEventListener('click', this.write_lnb_voltage);
        this.lnb_voltage[1].addEventListener('click', this.write_lnb_voltage);
        this.lnb_voltage[2].addEventListener('click', this.write_lnb_voltage);
        this.lnb_voltage[3].addEventListener('click', this.write_lnb_voltage);
        this.lnb_tone[0].addEventListener('click', this.write_lnb_tone);
        this.lnb_tone[1].addEventListener('click', this.write_lnb_tone);
		this.InputPowerLimits.addEventListener('click', this.onclickRFindet);
		this.OutputPowerLimits.addEventListener('click', this.onclickRFoutdet);
		this.LNBcurrentLimits.addEventListener('click', this.onclickLnbCurLim);
    }

	postrcm(rcm)
	{
		this.Document.post("../rcm.txt", rcm);
		setTimeout(this.fetch_control, 500);
		this.restartInterval();
	}

    write_10MHz_state(event) {
        const path_modulos = ["001,", "001,", "002,", "002"];
        var rcm = "";
        if (event.target.textContent === "Off") {
            rcm = "{MOD005PES," + path_modulos[Number(this.slot_id) - 1] + "001}";
            event.target.innerHTML = "On";
            this.set_10MHz.classList.add("selected");
        } else {
            rcm = "{MOD005PES," + path_modulos[Number(this.slot_id) - 1] + "000}";
            event.target.innerHTML = "Off";
            if (this.set_10MHz.classList.contains("selected")) {
                this.set_10MHz.classList.remove("selected");
            }
        }
        this.postrcm(rcm);
    }

    write_lnb_voltage(event) {
        const element = event.target;
        var value = element.textContent;
        const keys = Array.from(vlnb_select.keys());
        for (let i = 0; i < keys.length; i++) {
            if (this.lnb_voltage[i].classList.contains("selected")) {
                this.lnb_voltage[i].classList.remove("selected");
            }
            if (value == keys[i]) {
                this.lnb_voltage[i].classList.add("selected");
            }
        }
        var rcm = "{MOD" + this.slot_id + vlnb_select.get(value) + "}";
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

    write_lnb_tone(event) {
        const element = event.target;
        var value = element.textContent;
        const keys = Array.from(lnb_tone_select.keys());
        for (let i = 0; i < keys.length; i++) {
            if (this.lnb_tone[i].classList.contains("selected")) {
                this.lnb_tone[i].classList.remove("selected");
            }
            if (value == keys[i]) {
                this.lnb_tone[i].classList.add("selected");
            }
        }
        var rcm = "{MOD" + this.slot_id + lnb_tone_select.get(value) + "}";
        this.postrcm(rcm);
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
	
	onclickRFindet()
	{
		etlPrompt('Enter RF Input lower limit in dBm:', null, 'digital', this.doRFindetlo);
	}

	doRFindetlo(value)
	{
		if (value != "")
		{
			temp_value = value;
		
			etlPrompt('Enter RF Input higher limit in dBm:', null, 'digital', this.doRFindethi);
		}
	}

	doRFindethi(value)
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

		url = "{MOD"+this.slot_id+"ILS,001" + syml + ("0000" + parseInt(Math.abs(limitlo * 100))).slice(-4) + symh + ("0000" + parseInt(Math.abs(limithi * 100))).slice(-4) + "}";
		this.postrcm(url);
	}

	onclickRFoutdet()
	{
		etlPrompt('Enter Laser RF lower limit in dBm:', null, 'digital', this.doRFoutdetlo);
	}

	doRFoutdetlo(value)
	{
		if (value != "")
		{
			temp_value = value;
		
			etlPrompt('Enter Laser RF higher limit in dBm:', null, 'digital', this.doRFoutdethi);
		}
	}

	doRFoutdethi(value)
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

		url = "{MOD"+this.slot_id+"OLS,001" + syml + ("0000" + parseInt(Math.abs(limitlo * 100))).slice(-4) + symh + ("0000" + parseInt(Math.abs(limithi * 100))).slice(-4) + "}";
		this.postrcm(url);
	}

	onclickLnbCurLim()
	{
		etlPrompt('Enter LNB Amp lower limit in mA:', null, 'digital', this.doLnbCurLimlo);
	}

	doLnbCurLimlo(value)
	{
		if (value != "")
		{
			temp_value = value;
		
			etlPrompt('Enter LNB Amp higher limit in mA:', null, 'digital', this.doLnbCurLimhi);
		}
	}

	doLnbCurLimhi(value)
	{
		var value_hundredthsdB, url, inout, syml = '+', symh = '+';
		var limitlo, limithi;
		
		if (value == "")
			return;

		limitlo = Number(temp_value);
		limithi = Number(value);

		if (limitlo < 0)
		{
			limitlo = 0;
		}
		else if (limitlo > 9900)
		{
			limitlo = 9900;
		}

		if (limithi < 0)
		{
			limithi = 0;
		}
		else if (limithi > 9900)
		{
			limithi = 9900;
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

		url = "{MOD"+this.slot_id+"LLS,001" + syml + ("0000" + parseInt(Math.abs(limitlo))).slice(-4) + symh + ("0000" + parseInt(Math.abs(limithi))).slice(-4) + "}";
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
                // if (item._10MHz_state[Number(this.slot_id) - 1]) {
                this.set_10MHz.innerHTML = "On";
                // if (!this.set_10MHz.classList.contains("selected")) {
                //     this.set_10MHz.classList.add("selected");
                // }
                // } else {
                //     this.set_10MHz.innerHTML = "Off";
                //     if (!this.set_10MHz.classList.contains("selected")) {
                //         this.set_10MHz.classList.remove("selected");
                //     }
                // }
                for (let i = 0; i < 3; i++) {
                    if (this.lnb_voltage[i].classList.contains("selected")) {
                        this.lnb_voltage[i].classList.remove("selected");
                    }
                }
                this.lnb_voltage[item[slot_index].lnb_voltage].classList.add("selected");
                for (let i = 0; i < 2; i++) {
                    if (this.lnb_tone[i].classList.contains("selected")) {
                        this.lnb_tone[i].classList.remove("selected");
                    }
                        }
                this.lnb_tone[item[slot_index].lnb_tone].classList.add("selected");
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
					break;
					case 1: // AGC
					case 2: // Fixed
					this.gain_set.disabled = true;
                    this.gain_set.readOnly = true;
					this.gain_set.classList.add("disabled");
					break;
				}
				this.Document.getElementById("InputPowerLimits").innerHTML = parseInt(item[slot_index].in_lim_lo)/100 + "|" + parseInt(item[slot_index].in_lim_hi)/100 + "dBm";
				this.Document.getElementById("OutputPowerLimits").innerHTML = parseInt(item[slot_index].out_lim_lo)/100 + "|" + parseInt(item[slot_index].out_lim_hi)/100 + "dBm";
				this.Document.getElementById("LNBcurrentLimits").innerHTML = item[slot_index].lnb_lim_lo + "|" + item[slot_index].lnb_lim_hi + "mA";
            })
            .catch((error) => console.log("Error fetching data:", error));
    }

    fetch_module_monitoring() {
        fetch("../module_monitoring.json", {
                method: 'GET',
                cache: 'no-cache', // Specify 'no-cache' to prevent caching
            })
            .then((response) => response.json())
            .then((item) => {
				var cell;
				var slot_index = parseInt(this.slot_id) - 1;
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
				//document.getElementById("vlnb").innerHTML = parseFloat(item.V_lnb).toFixed(2);	
				if (item[slot_index].rfip_e)
				{
					document.getElementById("RFInputRead").className = "value monit alarm";
				}
				else
				{
					document.getElementById("RFInputRead").className = "value monit";
				}
				document.getElementById("RFInputRead").innerHTML = parseFloat(item[slot_index].RFip).toFixed(1) + "dBm";
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
				if (item[slot_index].rfop_e)
				{
					document.getElementById("RFOutputPower").className = "value monit alarm";
				}
				else
				{
					document.getElementById("RFOutputPower").className = "value monit";
				}
                document.getElementById("RFOutputPower").innerHTML = parseFloat(item[slot_index].RFop).toFixed(1) + "dBm";
				document.getElementById("laserPowerRead").innerHTML = parseFloat(item[slot_index].pd).toFixed(1) + "dBm";
				if (item[slot_index].ilnb_e)
				{
					document.getElementById("ilnb").className = "value monit alarm";
				}
				else
				{
					document.getElementById("ilnb").className = "value monit";
				}
                document.getElementById("ilnb").innerHTML = parseInt(item[slot_index].Ilnb) + "mA";
				
				if (item[slot_index].vlnb_e)
				{
					document.getElementById("vlnb").className = "value monit alarm";
				}
				else
				{
					document.getElementById("vlnb").className = "value monit";
				}
				document.getElementById("vlnb").innerHTML = Math. round(parseFloat(item[slot_index].Vlnb)) + "V";
                
				if (item[slot_index].i5v_e)
				{
					document.getElementById("ibus").className = "value monit alarm";
					document.getElementById("ibus").innerHTML = item[slot_index].Ibus + "mA";
				}
				else
				{
					document.getElementById("ibus").className = "value monit";
					document.getElementById("ibus").innerHTML = "OK";
				}
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
				if ((item[slot_index].model_number == "SRY-1T1R-9510TX")
					||(item[slot_index].model_number == "SRY-0T4R-9505TX")
					||(item[slot_index].model_number.includes("SRY-2T2R-9512TX"))
					)
				{
					// SRY_IDU
					document.getElementById("div_LNB_volt").style.display = "none";
					document.getElementById("div_LNB_tone").style.display = "none";
					document.getElementById("div_LNB_lim").style.display = "none";
					document.getElementById("div_LNB_cur").style.display = "none";
				}
            })
            .catch((error) => console.log("Error fetching data:", error));
    }
}