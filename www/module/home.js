var home_command_in_progress = 0;



function initModuleAjax()
{
	newAjaxRequest('home.xml', null, true, updateHome, updateHomeFail, null);
	newAjaxRequest('init.xml', null, false, initSuccess, initFail, null);
}



var homeCommErr = 0;

function updateHomeFail()
{
	if(homeCommErr < 10)
	{
		homeCommErr++;
	}
	if(homeCommErr > 9)
	{
		//TODO see if do something showing comm lost with unit
		//otherwise, main page will force diplay of comm lost with this module
	}
}


function updateHome(xd)
{
	var i, index, temp, packetSplit, packet, mnemonic, tempSplit, cell;
	var infoSplit;

	temp = gXMLv(xd, 'data');
	if(temp === null)
	{
		//bad format file received, ignore the rest
		return;
	}
	
	
	packetSplit = temp.split(';');
	//remove the {MODXXXST?, on packetSplit[0]
	packetSplit[0] = packetSplit[0].slice(11);
	
	//remove any caract after '}' on last packetSplit
	packetSplit[packetSplit.length-1] = (packetSplit[packetSplit.length-1].split('}'))[0];
	
	
	for(i=0; i<packetSplit.length; i++)
	{
		packet = packetSplit[i];
		//get the mnemonic of packet
		mnemonic = packet.substring(0,2);
		
		//process the packet
		switch(mnemonic)
		{
		case "SA":	//summary alarm
			cell = $('module_status');
			switch(packet.charAt(5))
			{
			case 'O':
				cell.style.color= '#fff';
				cell.style.fontWeight = 'normal';		
				cell.innerHTML = "OK";
			break;
			case 'W':
				cell.style.color= '#f80';
				cell.style.fontWeight = 'normal';		
				cell.innerHTML = "Warning";
			break;
			case 'F':
				cell.style.color= '#f00';
				cell.style.fontWeight = 'normal';		
				cell.innerHTML = "Fault";
			break;
			default:
				cell.style.color= '#888';
				cell.style.fontWeight = 'normal';		
				cell.innerHTML = "Unknown";
			break;
			}
		break;
	
		case "TC":	//temperature
			cell = $('temperature');
			switch(packet.charAt(5))
			{
			case 'O':
    		cell.style.color= '#fff';
    		cell.style.fontWeight = 'normal';	
    		cell.innerHTML = "OK";	

			break;
			case 'L':
			case 'H':
    		cell.style.color= '#f00';
    		cell.style.fontWeight = 'normal';
    		//cell.innerHTML = "Fault";	
    		cell.innerHTML = (parseInt(packet.substring(6, 10))/10).toString() + "°C";	

			break;
			default:
    		cell.style.color= '#888';
    		cell.style.fontWeight = 'normal';
    		cell.innerHTML = "Unknown";	
			break;
			}	
		break;
	
		case "HM"://humidity
			cell = $('humidity');
			switch(packet.charAt(5))
			{
			case 'O':
				cell.style.color= '#fff';
				cell.style.fontWeight = 'normal';	
				cell.innerHTML = "OK";		
			break;
			case 'L':
			case 'H':
				cell.style.color= '#f00';
				cell.style.fontWeight = 'normal';	
				//cell.innerHTML = "Fault";	
				cell.innerHTML = (parseInt(packet.substring(6, 9))).toString() + "%";	
			break;
			default:
				cell.style.color= '#888';
				cell.style.fontWeight = 'normal';	
				cell.innerHTML = "Unknown";		
			break;
			}
		break;

		case "HO":	//hours of operation
			cell = $('hours_operation');
			cell.innerHTML = (parseInt(packet.substring(6, 12))).toString();	
			switch(packet.charAt(5))
			{
			case 'O':
			default:
				cell.style.color = '#fff';	
				$('acknowledge').style.display = 'none';
			break;
			case 'W':
				cell.style.color = '#f80';
				$('acknowledge').style.display = 'inline-table';		
			break;
			}
		break;
    
		case "WY":	//warranty
			cell = $('module_warranty');
			switch(packet.charAt(5))
			{
			case 'O':
			default:
				cell.style.color = '#fff';	
				$('warranty_info').style.display = 'none';
			break;
			case 'W':
				cell.style.color = '#f80';
				$('warranty_info').style.display = 'inline-table';
			break;
			case 'F':
				cell.style.color = '#f00';
				$('warranty_info').style.display = 'inline-table';
			break;
			}
			switch(packet.charAt(6))
			{
			case 'D':
			default:
				$('acknowledge_warranty').style.display = 'none';
			break;
			case 'E':
				$('acknowledge_warranty').style.display = 'inline-table';
			break;
			}
		break;

		case "PI":  //product info
			infoSplit = packet.split('|');
			//serial number
			$('serial_number').innerHTML = infoSplit[0].substring(5, infoSplit[0].length);
		
			//warranty date
			$('module_warranty').innerHTML = infoSplit[1];

			//build date
			if (infoSplit[2].charAt(infoSplit[2].length-1) == '}')
			{
				$('module_build').innerHTML = infoSplit[2].substring(0, infoSplit[2].length-1);
			}
			else
			{
				$('module_build').innerHTML = infoSplit[2];
			}
		break;
		}
	}
	

	$('warranty_email').href = "mailto:info@etlsystems.com?subject=ETL Systems - Warranty Extension For: " 
							+ $('model_number').innerHTML + " - " + $('serial_number').innerHTML + 
							"&body=Dear ETL Info, %0D%0A%0D%0AI would like to receive a quotation to extend warranty for CPU " +
							$('model_number').innerHTML + " serial " + $('serial_number').innerHTML +
							"%0D%0A%0D%0AThe software version is: " + $('module_top_software').innerHTML +
							" with the release build: " + $('module_build').innerHTML + 
							" and the warranty expiry date is: " + $('module_warranty').innerHTML +". %0D%0A%0D%0AKind Regards,";
}


function initFail()
{
	//retry til we get it
	newAjaxRequest('init.xml', null, false, initSuccess, initFail, null);
}


function initSuccess(xd)
{
	var i, index, temp, tempSplit, cell;

	temp = gXMLv(xd, 'data');
	if(temp === null)
	{
		//bad format file received, retry and ignore the rest
		initFail();
		return;
	}
	
	tempSplit = temp.split(',');

	$('model_number').innerHTML = tempSplit[0];
	$('serial_number').innerHTML = tempSplit[1];
	$('module_top_software').innerHTML = tempSplit[2];
	$('module_warranty').innerHTML = tempSplit[3];
	$('module_build').innerHTML = tempSplit[4];
}
function doExtendHoursOperation(cell)
{

	if(home_command_in_progress)
		{return;}	
	
	etlConfirm("Extend hours of operation warning timer by 6 months?", launchExtendHoursOperation);


}

function launchExtendHoursOperation(buttonReturned)
{
    if(buttonReturned === false)
    {
		return;
    }	
	
	var url = 'command.cgi?rcm={MODXXXHES,0011}';
	newAjaxRequest( url, true, false, resultExtendHoursOperationSucess, resultExtendHoursOperationFail, null);
	home_command_in_progress = 1;
}


function resultExtendHoursOperationSucess(xd)
{
	var temp, tempSplit;

	home_command_in_progress = 0;
	
	temp = gXMLv(xd, 'result');
	if(temp === null)
	{
		//bad format file received, ignore the rest
		return;
	}
	
	//{MODXXXHES,0011,O}
	tempSplit = temp.split('|');

	//monitoring
	switch(tempSplit[0].charAt(16))
	{
	case 'O':
		etlAlert("Hours of operation warning timer extended by 6 months.");
	break;
	case 'E':
	case 'F':
	default:
		etlAlert("Error extending hours of operation warning timer.");
	break;
	}
}

function resultExtendHoursOperationFail(xd)
{
	home_command_in_progress = 0;
	
	etlAlert("Error, the unit did not respond.");
}



function doExtendWarranty(cell)
{

	if(home_command_in_progress)
		{return;}	
	
	etlConfirm("Acknowledge reaching end of warranty?", launchExtendWarranty);


}

function launchExtendWarranty(buttonReturned)
{
    if(buttonReturned === false)
    {
		return;
    }	
	
	var url = 'command.cgi?rcm={MODXXXWYS,0011}';
	newAjaxRequest( url, true, false, resultExtendWarrantySucess, resultExtendWarrantyFail, null);
	home_command_in_progress = 1;
}


function resultExtendWarrantySucess(xd)
{
	var temp, tempSplit;

	home_command_in_progress = 0;
	
	temp = gXMLv(xd, 'result');
	if(temp === null)
	{
		//bad format file received, ignore the rest
		return;
	}
	
	//{MODXXXWYS,0011,O}
	tempSplit = temp.split('|');

	//monitoring
	switch(tempSplit[0].charAt(16))
	{
	case 'O':
		etlAlert("Reaching end of warranty acknowledged.");
	break;
	case 'E':
	case 'F':
	default:
		etlAlert("Error acknowledging reaching end of warranty.");
	break;
	}
}

function resultExtendWarrantyFail(xd)
{
	home_command_in_progress = 0;
	
	etlAlert("Error, the unit did not respond.");
}
