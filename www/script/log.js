function initLog()
{
	document.getElementById("logContainer").innerHTML = "";
	
    fetch("../loghead.json", {
            method: 'GET',
            cache: 'no-cache', // Specify 'no-cache' to prevent caching
        })
        .then((response) => response.json())
        .then((item) => {
				if (item.level < 0xff)
					DisplayAndGetNextLog(item);
				else
					displayNoLog();
            })
            .catch((error) => console.log("Error fetching data:", error));
}

function DisplayAndGetNextLog(item)
{
	if (item.level < 0xff)
	{
		addLogLine(item);

		fetch("../lognext.json", {
				method: 'GET',
				cache: 'no-cache', // Specify 'no-cache' to prevent caching
			})
			.then((response) => response.json())
			.then((item) => {
				DisplayAndGetNextLog(item);
			})
			.catch((error) => console.log("Error fetching data:", error));
	}
}

function addLogLine(item)
{
	var colour, cell, i, tempSplit2;
	
	switch(item.level)
	{
		case 0:	//invalid
			colour = "#E76DFF";
			break;
		case 1:	//normal
		   colour = "#FFFFFF";
			break;
		case 2:	//alarm
			colour = "#FF0000";
			break;
		case 3:	//warning
			colour = "#F07800";
			break;
		case 4:	//ok
			colour = "#0FF000";
			break;
		default:
			colour = "#A6A6A6";
			break;
    }	
	
	var table = document.getElementById("logContainer");
	var row = table.insertRow(0);

	cell = row.insertCell(0);
	cell.style.width = "20px";
	cell = row.insertCell(0);
	tempSplit2 = item.log.split(":: ");
	if(tempSplit2.length === 1)
	{
		cell.innerHTML = tempSplit2[0];
	}
	else
	{
		cell.innerHTML = tempSplit2[1];
	}
	cell.style.color = colour;
	cell = row.insertCell(0);
	cell.style.width = "20px";
	cell = row.insertCell(0);
	if(tempSplit2.length === 1)
	{
		cell.innerHTML = "&nbsp;";
	}
	else
	{
		cell.innerHTML = tempSplit2[0]
	}
	cell.style.color = colour;

	cell = row.insertCell(0);
	cell.style.width = "20px";
	cell = row.insertCell(0);
	cell.innerHTML = Math.trunc(item.time/86400)+"day(s)"+Math.trunc((item.time%86400)/3600)+":"+("0"+Math.trunc((item.time%3600)/60)).slice (-2)+":"+("0"+(item.time%60)).slice (-2);
	cell.style.color = colour;
}

function doFormatLog()
{
    var module_num;
	var url;
	
	setTimeout(function () {
		etlConfirm("Are you sure you want to clear the log?", clearLog);
	}, 200);

	function clearLog(buttonReturned)
	{
		if (buttonReturned == false) {
			return;
		}

		var rcm = "{MOD005FLS}";
		
		fetch("../rcm.txt", {
                method: 'POST',
                cache: 'no-cache',
                headers: {
                    'Content-Type': 'text/plain' // Specify the content type as JSON
                        // You may also need to include other headers if required by the API
                },
                body: (rcm) // Convert the data object to a JSON string
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
				initLog();
            })
            .catch(error => {
                // Handle errors during the fetch
                console.error('Error during fetch:', error);
            });
	}
}

function doDownloadLog()
{
	var log = "";
	var i;

	for (i = 0; i < $('logContainer').rows.length; i++)
	{
		if($('logContainer').rows[i].cells[0].innerHTML !== "&nbsp;")
		{
			log += $('logContainer').rows[i].cells[0].innerHTML;
		}
		else
		{
			log += "\t";
		}
		log += "\t";
		if($('logContainer').rows[i].cells[2].innerHTML !== "&nbsp;")
		{
			log += $('logContainer').rows[i].cells[2].innerHTML;
		}
		else
		{
			log += "\t";
		}
		log += "\t";
		if($('logContainer').rows[i].cells[4].innerHTML !== "&nbsp;")
		{
			log += $('logContainer').rows[i].cells[4].innerHTML;
		}
		else
		{
			log += "\t";
		}
        
		log += "\n";
	}

	var downloadButton = document.createElement("a");
	var file = new Blob([log], {type: 'text/plain'});

	downloadButton.href = URL.createObjectURL(file);
	downloadButton.download = "log.txt";
	document.body.appendChild(downloadButton);
	downloadButton.click();
	document.body.removeChild(downloadButton);
}

function displayNoLog()
{
	document.getElementById("logContainer").innerHTML = "<p style=\"color:#FFFFFF;\">No log events to display.</p>";
}
