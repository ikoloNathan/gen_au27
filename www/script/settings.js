/* 
 * File:   home.js
 * Author: Nathan.Ikolo
 *
 * Created on January 10, 2024, 08:52 AM
 */

const dis_en_state = ["Disabled", "Enabled"];

class Settings {
    constructor(Doc, bodyId) {
        this.Document = Doc;
        this.main = this.Document.getElementById(bodyId);
		this.set_dhcp_state = this.set_dhcp_state.bind(this);
		this.validate_ip = this.validate_ip.bind(this);
		this.click_cpu_reboot = this.click_cpu_reboot.bind(this);
		this.reboot_cpu = this.reboot_cpu.bind(this);
		this.click_trap_en = this.click_trap_en.bind(this);
		this.button_valid = this.Document.getElementById("id_button_valid");
		this.input_ip = this.Document.getElementById("ipAddress");
		this.button_dhcp = this.Document.getElementById("id_button_dhcp");
		this.button_cpu_reboot = this.Document.getElementById("id_button_cpu_reboot");
        this.fetch_settings = this.fetch_settings.bind(this);
        this.fetch_settings();
		this.add_control_event_listener();
    }

	add_control_event_listener() {
		this.button_valid.addEventListener("click", this.validate_ip);
		this.input_ip.addEventListener("oninput", function() {
            var inputElement = document.getElementById('ipAddress');
            var ipAddress = inputElement.value.trim();

            // Remove any non-numeric and non-dot characters
            ipAddress = ipAddress.replace(/[^0-9.]/g, '');

            // Split the IP address into octets
            var octets = ipAddress.split('.');

            // Format the IP address with periods
            if (octets.length > 1) {
                ipAddress = octets.slice(0, 4).join('.');
            }
            // Update the input field with the formatted IP address
            inputElement.value = ipAddress;
        });
		this.button_dhcp.addEventListener("click", this.set_dhcp_state);
		this.button_cpu_reboot.addEventListener("click", this.click_cpu_reboot);
		document.getElementById('id_button_trap_en').addEventListener("click", this.click_trap_en);
	}

	click_trap_en()
	{
        var button_val = document.getElementById("id_button_trap_en");
		var checkbox_val = document.getElementsByName("tren")[0];
        if (checkbox_val.checked)
		{
			checkbox_val.checked = false;
            button_val.innerHTML = dis_en_state[0];
		}
        else
		{
			checkbox_val.checked = true;
            button_val.innerHTML = dis_en_state[1];
		}
    }

    set_dhcp_state() {
        var button_val = document.getElementById("id_button_dhcp");
        if (button_val.innerHTML == dis_en_state[1])
            button_val.innerHTML = dis_en_state[0];
        else
            button_val.innerHTML = dis_en_state[1];
    }

    validate_ip()
	{
        var ipAddress = document.getElementById('ipAddress').value.trim();
		var mask = document.getElementById('subnet_mask').value.trim();
		var gw = document.getElementById('gateway').value.trim();
		var dhcp = (document.getElementById("id_button_dhcp").innerHTML == "Disabled")?0:1;
        // Regular expression for a basic IP address validation
        var ipRegex = /^(\d{1,3}\.){3}\d{1,3}$/;

        if (ipRegex.test(ipAddress) && ipRegex.test(mask) && ipRegex.test(gw))
		{
            var rcm = "{MOD005NTS," + dhcp + "," + ipAddress + "," + mask + "," + gw + "}";
            this.Document.post("../rcm.txt", rcm);
        }
		else
		{
            alert('Invalid IP Address. Please enter a valid IPv4 address.');
        }
    }

	click_cpu_reboot()
	{
		etlConfirm("Reboot CPU?", this.reboot_cpu);
	}

	reboot_cpu(buttonReturned)
	{
		if (buttonReturned == false) {
			return;
		}
		var rcm = "{MOD005RSS}";
		this.Document.post("../rcm.txt", rcm);
	}

    fetch_settings() {
        fetch("../settings.json", {
                method: 'GET',
                cache: 'no-cache', // Specify 'no-cache' to prevent caching
            })
            .then((response) => response.json())
            .then((item) => {
                var button_val = document.getElementById("id_button_dhcp");
                var dhcp_state = parseInt(item.dhcp);
                if (button_val.innerHTML != dis_en_state[dhcp_state])
                    button_val.innerHTML = dis_en_state[dhcp_state];
				document.getElementById("ipAddress").value = item.ip;
				document.getElementById("subnet_mask").value = item.mask;
				document.getElementById("gateway").value = item.gw;
				document.getElementsByName("rcm0")[0].value = item.rcm0;
				document.getElementsByName("rcm1")[0].value = item.rcm1;
				document.getElementsByName("rcm2")[0].value = item.rcm2;
				document.getElementsByName("wcm0")[0].value = item.wcm0;
				document.getElementsByName("wcm1")[0].value = item.wcm1;
				document.getElementsByName("wcm2")[0].value = item.wcm2;
				document.getElementsByName("tren")[0].checked = (item.tren == 1?true:false);
                document.getElementById("id_button_trap_en").innerHTML = dis_en_state[item.tren];
				document.getElementsByName("trd")[0].value = item.trd;
				document.getElementsByName("trc")[0].value = item.trc;
            })
            .catch((error) => console.log("Error fetching data:", error));
    }
}
