var keyboardDisplayed;
var eraseTimeout;
var freeScreen = true;
var hmiScreen;
var LoginFocus = 'user';
var codeShift, codeLeft, codeRight, codeOk, codeEsc;

/*========================================================
========================================================*/
if (typeof "$" !== "undefined") 
{ 
	function $(id)
	{
		if(document.getElementById(id))
		{
			return document.getElementById(id);
		}
		else
		{
			return null;
		}
	}
}

/*========================================================
 ========================================================*/
function checkScreen()
{
	if(freeScreen)
	{
		clearTimeout(eraseTimeout);
		if ($("modalContainer")) 
		{
			document.getElementsByTagName("body")[0].removeChild($("modalContainer"));
		}		
		return true;
	}
	return false;
}

/*========================================================
 ========================================================*/
function detectScreen()
{
	var url = parent.document.location.href;
	if(url.indexOf('index-hmi.htm') !== -1)
	{
		hmiScreen = true;
		codeShift = "&#x2191;";
		codeLeft = "<";
		codeRight = ">";
		codeOk = "Ok";
		codeEsc = "Esc";
	}
	else
	{
		hmiScreen = false;
		/*codeShift = "&#x21e7;";
		codeLeft = "&#x25C0;";
		codeRight = "&#x25B6;";
		codeOk = "&#x2714;";
		codeEsc = "&#x2716;";*/
		codeShift = "&#x2191;";
		codeLeft = "<";
		codeRight = ">";
		codeOk = "Ok";
		codeEsc = "Esc";
		
	}
}
detectScreen();

/*========================================================
 ========================================================*/
function etlAlert(question, id)
{
    var key;
    var height = window.innerHeight;
    var width = window.innerWidth;
    var d = document;

    if (!checkScreen()) 
	{
		return;
    }
	freeScreen = false;

    d.onkeypress = keyPressed;
    d.onkeydown = keyDown;
    d.documentElement.style.overflowY = 'hidden';

    mObj = d.getElementsByTagName("body")[0].appendChild(d.createElement("div"));
    mObj.id = "modalContainer";
    mObj.setAttribute("class", "etlAlarm");
    mObj.style.display = "block";
    mObj.style.visiblity = "visible";

    buttonObj = mObj.appendChild(d.createElement("div"));
    buttonObj.outerHTML += '<table id="confirm" border=0> \
				<tr> \
				    <td id="theQuestion" style="max-width: 600px;"></td> \
				</tr> \
				<tr> \
				    <td>&nbsp;</td> \
				</tr> \
				<tr> \
				    <td><div id="keyboard"></div></td> \
				</tr> \
			    </table>';


    keysToDisplay.push("Escape");
    keysToDisplay.push("k-13");
    key = addButton("k-13", codeOk);
    key.style.color = "#0c4";
    $("k-13").callBack = function(){toggleKey(this); removeCustomAlert(id);};
    $("k-13").addEventListener("click", $("k-13").callBack);
    key = addKey("Escape", codeEsc);
    key.style.color = "#f00";
    key.style.display = "none";
    $("Escape").callBack = function(){toggleKey(this); removeCustomAlert(id);};
    $("Escape").addEventListener("click", $("Escape").callBack);


    $("theQuestion").innerHTML = question;
    $("keyboard").style.display = "inline-block";

    if ((height > 250) || (width > 1000))
    {
		mObj.classList.add("big_screen");
		if (id)
		{
			$('menu_icon').style.opacity = '0.1';
			$('intro').style.opacity = '0.1';
			$('top_bar').style.opacity = '0.1';
			$('connection_ok').style.opacity = '0.1';
		}
		else
		{
			$('content').style.opacity = '0.1';
		}
		
		//place in center
		mObj.style.left = ((width - mObj.clientWidth) / 2) + 'px';
		mObj.style.top = ((height - mObj.clientHeight) / 2) + 'px';
    }
}

var keysToDisplay = [];

/*========================================================
========================================================*/
function etlConfirm(question, callBack, id)
{
    var key;
    var height = window.innerHeight;
    var width = window.innerWidth;
    var d = document;

    if (!checkScreen()) 
	{
		return;
    }
	freeScreen = false;

    d.onkeypress = keyPressed;
    d.onkeydown = keyDown;
    d.documentElement.style.overflowY = 'hidden';


    mObj = d.getElementsByTagName("body")[0].appendChild(d.createElement("div"));
    mObj.id = "modalContainer";

    mObj.setAttribute("class", "etlAlarm");
    mObj.style.display = "block";
    mObj.style.visiblity = "visible";


    buttonObj = mObj.appendChild(d.createElement("div"));
    buttonObj.outerHTML += '<table id="confirm" border=0> \
				<tr> \
				    <td id="theQuestion" style="max-width: 600px;"></td> \
				</tr> \
				<tr> \
				    <td>&nbsp;</td> \
				</tr> \
				<tr> \
				    <td><div id="keyboard"></div></td> \
				</tr> \
			    </table>';


    key = addButton("k-13", codeOk);
    key.style.color = "#0c4";
    key = addButton("Escape", codeEsc);
    key.style.color = "#f00";
    keysToDisplay.push("Escape");
    keysToDisplay.push("k-13");

    if(typeof callBack === "function")
    {
		$("k-13").callBack = function (){toggleKey(this); removeCustomAlert(id); callBack(true);};
		$("Escape").callBack = function (){toggleKey(this); removeCustomAlert(id); callBack(false);};
    }	
    else
    {
		$("k-13").callBack = function (){toggleKey(this); removeCustomAlert(id);};
		$("Escape").callBack = function (){toggleKey(this); removeCustomAlert(id);};
    }

    $("k-13").addEventListener("click", $("k-13").callBack);
    $("Escape").addEventListener("click", $("Escape").callBack);
    
    $("theQuestion").innerHTML = question;
    $("keyboard").style.display = "inline-block";

    if ((height > 250) || (width > 1000))
    {
		mObj.classList.add("big_screen");
		if (id == 'body')
		{
			$('menu_icon').style.opacity = '1';
			$('intro').style.opacity = '1';
			$('top_bar').style.opacity = '1';
			$('connection_ok').style.opacity = '1';
		}
		else
		{
			$('content').style.opacity = '1';
		}
		//place in center
		mObj.style.left = ((width - mObj.clientWidth) / 2) + 'px';
		mObj.style.top = ((height - mObj.clientHeight) / 2) + 'px';
    }
}

/*========================================================
========================================================*/
var keyboardValueEntered,
	keyboardUserEntered,
	keyboardPassEntered;

function getEtlPromptValueEntered()
{
	return keyboardValueEntered;
}

/*========================================================
========================================================*/
function etlPrompt(question, preAnswer, type, callBack) 
{
    var key;
    var height = window.innerHeight;
    var width = window.innerWidth;
    var d = document;

    if (!checkScreen()) 
	{
		return;
    }
	freeScreen = false;

    d.onkeypress = keyPressed;
    d.onkeydown = keyDown;
	d.onkeyup = keyUp;
    d.documentElement.style.overflowY = 'hidden';
    keysToDisplay = [];

    mObj = d.getElementsByTagName("body")[0].appendChild(d.createElement("div"));
    mObj.id = "modalContainer";

    mObj.setAttribute("class", "etlPrompt"); 
    //mObj.style.display = "block";    
    mObj.style.visiblity="visible";


    buttonObj = mObj.appendChild(d.createElement("div"));
	if(hmiScreen)
	{
		//alert("ok");
		buttonObj.outerHTML += '<table border=0> \
					<tr> \
						<td width="135px" id="theQuestion"></td> \
						<td width="15px">&nbsp;</td> \
						<td id="keyboard" rowspan=3></td> \
					</tr> \
					<tr> \
						<td id="theAnswer"><input autocomplete="off" type="text" spellcheck="false" id="theAnswerInput" name="theAnswer" value="" size="17" maxlength="0" readonly/></td> \
						<td>&nbsp;</td> \
					</tr> \
					<tr> \
						<td>&nbsp;</td> \
						<td>&nbsp;</td> \
					</tr> \
					</table>';
	}	
	else
	{
		buttonObj.outerHTML += '<table border=0> \
					<tr> \
						<td width="135px" id="theQuestion"></td> \
						<td width="15px">&nbsp;</td> \
						<td id="keyboard" rowspan=3></td> \
					</tr> \
					<tr> \
						<td id="theAnswer"><input autocomplete="off" type="text" spellcheck="false" id="theAnswerInput" name="theAnswer" value="" size="17" maxlength="0"/></td> \
						<td>&nbsp;</td> \
					</tr> \
					<tr> \
						<td>&nbsp;</td> \
						<td>&nbsp;</td> \
					</tr> \
					</table>';
	}

    if (type === "digital")
	{
		key = addKey("k-55", "7");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		addKey("k-56", "8");
		addKey("k-57", "9");
		addKey("k-45", "-");
		addKey("k-43", "+");

		key = addKey("k-52", "4");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		addKey("k-53", "5");
		addKey("k-54", "6");
		addKey("k-47", "/");
		addKey("k-8", "&#x2190;");

		key = addKey("k-49", "1");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		addKey("k-50", "2");
		addKey("k-51", "3");
		addKey("k-l", codeLeft);
		addKey("k-r", codeRight);


		key = addKey("k-48", "0");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		key.style.width = "76px";
		addKey("k-46", ".");

		key = addKey("k-13", codeOk);
		key.style.color = "#0c4";
		key = addKey("Escape", codeEsc);
		key.style.color = "#f00";


		if (typeof callBack === "function")
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert();
							//callBack(address, $("theAnswerInput").value);
							keyboardValueEntered = $("theAnswerInput").value;
							callBack($("theAnswerInput").value);
						};
			$("Escape").callBack = function () {
							toggleKey(this);
							if ($("theAnswerInput").value === "")
							{
								removeCustomAlert();
								//callBack(address, null);
								keyboardValueEntered = null;
								//callBack(null);
							} else
							{
								$("theAnswerInput").value = "";
							}
						};
		} 
		else
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert();
						};
			$("Escape").callBack = function () {
							toggleKey(this);
							if ($("theAnswerInput").value === "")
							{
								removeCustomAlert();
							} 
							else
							{
								$("theAnswerInput").value = "";
							}
						};
		}

		$("k-13").addEventListener("click", $("k-13").callBack);
		$("Escape").addEventListener("click", $("Escape").callBack);
		
		keyboardDisplayed = "digital";
	} 
	else if (type === "full")
	{
		key = addKey("k-48", "0");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		key.style.width = "76px";
		addKey("k-46", ".");

		key = addKey("k-13", codeOk);
		key.style.color = "#0c4";
		key = addKey("Escape", codeEsc);
		key.style.color = "#f00";

		if (typeof callBack === "function")
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert();
							//callBack(address, $("theAnswerInput").value);
							keyboardValueEntered = $("theAnswerInput").value;
							callBack($("theAnswerInput").value);
						};
			$("Escape").callBack = function () {
							toggleKey(this);
							if ($("theAnswerInput").value === "")
							{
								removeCustomAlert();
								//callBack(address, null);
								keyboardValueEntered = null;
								//callBack(null);
							} else
							{
								$("theAnswerInput").value = "";
							}
						};
		} 
		else
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert();
						};
			$("Escape").callBack = function () {
							toggleKey(this);
							if ($("theAnswerInput").value === "")
							{
								removeCustomAlert();
							} else
							{
								$("theAnswerInput").value = "";
							}
						};
		}

		fullLowercaseDisplay();
	}

    preAnswer = (preAnswer ? decodeHtml(preAnswer): '');
	$("theQuestion").innerHTML = question;
	$("theAnswerInput").value = preAnswer;
	
	$("theAnswerInput").selectionStart = 0;
	$("theAnswerInput").selectionEnd = $("theAnswerInput").value.length;
	
	
	$("theAnswerInput").addEventListener("paste", function(e) {
	    // cancel paste
	    e.preventDefault();
	    
	    var input = $("theAnswerInput");

	    // get text representation of clipboard
	    var text = (e.originalEvent || e).clipboardData.getData('text/plain');
	    
	    replaceSelection(input);

	    var oldVal = input.value,
	    	selStart = input.selectionStart;
	    
	    input.value =  oldVal.slice(0, selStart) + text + oldVal.slice(selStart);
	    
	    input.selectionEnd = input.selectionStart = selStart + text.length; 
	});

		
	//$("keyboard").style.display = "inline-block";
	if(!hmiScreen)
	{
		$("theAnswerInput").focus();
	}

	if ((height > 250) || (width > 1000))
	{
		mObj.classList.add("big_screen");
		$('content').style.opacity = '0.1';
		//place in center
		mObj.style.left = ((width - 900) / 2) + 'px';
		mObj.style.top = ((height - 350) / 2) + 'px';
	}
}

/*========================================================
========================================================*/
function etlMultiPrompt(question, preAnswer, type, callBack) 
{
    var key, type = 'full';
    var height = window.innerHeight;
    var width = window.innerWidth;
    var d = document;

    if ($("modalContainer")) 
	{
		return;
    }

    d.onkeypress = keyPressed;
    d.onkeydown = keyDown;
	d.onkeyup = keyUp;
    d.documentElement.style.overflowY = 'hidden';
    keysToDisplay = [];

    mObj = d.getElementsByTagName("body")[0].appendChild(d.createElement("div"));
    mObj.id = "modalContainer";

    mObj.setAttribute("class", "etlPrompt"); 
    //mObj.style.display = "block";    
    mObj.style.visiblity="visible";


    buttonObj = mObj.appendChild(d.createElement("div"));
    buttonObj.outerHTML += '<table border=0> \
				<tr> \
				    <td width="235px" id="theQuestion"></td> \
				    <td width="15px">&nbsp;</td> \
				    <td id="keyboard" rowspan=3></td> \
				</tr> \
				<tr> \
				    <td id="theUser"><input autocomplete="off" type="text" spellcheck="false" id="theUserInput" name="theUser" value="" size="23" maxlength="0"/></td> \
				    <td>&nbsp;</td> \
				</tr> \
				<tr> \
				    <td id="thePassword"><input autocomplete="off" type="text" spellcheck="false" id="thePasswordInput" name="thePassword" value="" size="23" maxlength="0"/></td> \
				    <td>&nbsp;</td> \
				</tr> \
				<tr> \
				    <td>&nbsp;</td> \
				    <td>&nbsp;</td> \
				</tr> \
			    </table>';

    if (type === "full")
	{
		key = addKey("k-48", "0");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		key.style.width = "76px";
		addKey("k-46", ".");

		key = addKey("k-13", "&#x2714;");
		key.style.color = "#0c4";
		key = addKey("Escape", "&#x2716;");
		key.style.color = "#f00";

		if (typeof callBack === "function")
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert();
							//callBack(address, $("theAnswerInput").value);
							keyboardUserEntered = $("theUserInput").value;
							keyboardPassEntered = $("thePasswordInput").value;
							if (($("theUserInput").value !== "") || ($("thePasswordInput").value !== ""))
							{
								callBack($("theUserInput").value, $("thePasswordInput").value);
							}
							else
							{
								alert("Please complete form");
							}
						};
			$("Escape").callBack = function () {
							toggleKey(this);
							if (($("theUserInput").value === "") || ($("thePasswordInput").value === ""))
							{
								removeCustomAlert();
								//callBack(address, null);
								keyboardUserEntered = null;
								keyboardPassEntered = null;
								//callBack(null);
							} else
							{
								//$("theAnswerInput").value = "";
							}
						};
		} 
		else
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert();
						};
			$("Escape").callBack = function () {
							toggleKey(this);
							if ($("theAnswerInput").value === "")
							{
								removeCustomAlert();
							} else
							{
								$("theAnswerInput").value = "";
							}
						};
		}

		fullLowercaseDisplay();
	}
	else if (type === "digital")
	{
		key = addKey("k-55", "7");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		addKey("k-56", "8");
		addKey("k-57", "9");
		addKey("k-45", "-");
		addKey("k-43", "+");

		key = addKey("k-52", "4");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		addKey("k-53", "5");
		addKey("k-54", "6");
		addKey("k-47", "/");
		addKey("k-8", "&#x2190;");

		key = addKey("k-49", "1");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		addKey("k-50", "2");
		addKey("k-51", "3");
		addKey("k-l", "&#x25C0;");
		addKey("k-r", "&#x25B6;");


		key = addKey("k-48", "0");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		key.style.width = "76px";
		addKey("k-46", ".");

		key = addKey("k-13", "&#x2714;");
		key.style.color = "#0c4";
		key = addKey("Escape", "&#x2716;");
		key.style.color = "#f00";


		if (typeof callBack === "function")
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert();
							//callBack(address, $("theAnswerInput").value);
							keyboardUserEntered = $("theUserInput").value;
							keyboardPassEntered = $("thePasswordInput").value;
							if (($("theUserInput").value !== "") || ($("thePasswordInput").value !== ""))
							{
								callBack($("theUserInput").value, $("thePasswordInput").value);
							}
							else
							{
								alert("Please complete form");
							}
						};
			$("Escape").callBack = function () {
							toggleKey(this);
							if (($("theUserInput").value === "") || ($("thePasswordInput").value === ""))
							{
								removeCustomAlert();
								//callBack(address, null);
								keyboardUserEntered = null;
								keyboardPassEntered = null;
								//callBack(null);
							} else
							{
								//$("theAnswerInput").value = "";
							}
						};
		} 
		else
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert();
						};
			$("Escape").callBack = function () {
							toggleKey(this);
							if ($("theAnswerInput").value === "")
							{
								removeCustomAlert();
							} else
							{
								$("theAnswerInput").value = "";
							}
						};
		}

		$("k-13").addEventListener("click", $("k-13").callBack);
		$("Escape").addEventListener("click", $("Escape").callBack);
		
		keyboardDisplayed = "digital";
	} 


	$("theQuestion").innerHTML = question;
	//$("keyboard").style.display = "inline-block";
	$("theUserInput").focus();

	if ((height > 250) || (width > 1000))
	{
		mObj.classList.add("big_screen");
		$('content').style.opacity = '0.1';
		//place in center
		mObj.style.left = ((width - 900) / 2) + 'px';
		mObj.style.top = ((height - 350) / 2) + 'px';
	}
}

/*========================================================
========================================================*/
function etlLogin(question, callBack, id, showPass) 
{
    var key, type = 'full';
    var height = window.innerHeight;
    var width = window.innerWidth;
    var d = document;
    var passField;
	
	if (!id)
	{
		id = 'body';
	}
	
    if ($("modalContainer")) 
	{
		return;
    }

    d.onkeypress = keyPressed;
    d.onkeydown = keyDown;
	d.onkeyup = keyUp;
    d.documentElement.style.overflowY = 'hidden';
    keysToDisplay = [];

    mObj = d.getElementsByTagName("body")[0].appendChild(d.createElement("div"));
    mObj.id = "modalContainer";

    mObj.setAttribute("class", "etlPrompt"); 
    //mObj.style.display = "block";    
    mObj.style.visiblity="visible";


    buttonObj = mObj.appendChild(d.createElement("div"));
    
    if (showPass)
    {
    	passField = '<td id="thePassword"><label for="thePassword">Password:</label><input onfocus="getFocus(\'pass\')" autocomplete="off" type="text" spellcheck="false" id="thePasswordInput" name="thePassword" value="" size="23" maxlength="0"/></td>';
    }
    else
    {
    	passField = '<td id="thePassword"><label for="thePassword">Password:</label><input onfocus="getFocus(\'pass\')" class="password" autocomplete="off" type="password" spellcheck="false" id="thePasswordInput" name="thePassword" value="" size="23" maxlength="0"/></td>'
    	
    }

	 buttonObj.outerHTML += '<table border=0> \
			<tr> \
			    <td width="235px" id="theQuestion"></td> \
			    <td width="15px">&nbsp;</td> \
			    <td id="keyboard" rowspan=3></td> \
			</tr> \
			<tr> \
			    <td id="theUser"><label for="theUserInput">User:</label><input onfocus="getFocus(\'user\')" autocomplete="off" type="text" spellcheck="false" id="theUserInput" name="theUser" value="" size="23" maxlength="0"/></td> \
			    <td>&nbsp;</td> \
			</tr> \
			<tr>' + passField + '<td>&nbsp;</td> \
			</tr> \
			<tr> \
			    <td>&nbsp;</td> \
			    <td>&nbsp;</td> \
			</tr> \
		    </table>';


    if (type === "full")
	{
		key = addKey("k-48", "0");
		key.style.clear = "both";
		key.style.marginLeft = "280px";
		key.style.width = "76px";
		addKey("k-46", ".");

		key = addKey("k-13", "&#x2714;");
		key.style.color = "#0c4";
		key = addKey("Escape", "&#x2716;");
		key.style.color = "#f00";

		if (typeof callBack === "function")
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert(id);
							//callBack(address, $("theAnswerInput").value);
							keyboardUserEntered = $("theUserInput").value;
							keyboardPassEntered = $("thePasswordInput").value;
//							if (($("theUserInput").value !== "") || ($("thePasswordInput").value !== ""))
//							{
//								$("thePasswordInput").value = window.btoa(keyboardPassEntered);
								callBack($("theUserInput").value, $("thePasswordInput").value);
//							}
//							else
//							{
//								alert("Please enter Username/Password");
//							}
						};
			$("Escape").callBack = function () {
							toggleKey(this);
//							if (($("theUserInput").value === "") || ($("thePasswordInput").value === ""))
//							{
								removeCustomAlert(id);
								//callBack(address, null);
								keyboardUserEntered = null;
								keyboardPassEntered = null;
								callBack(null, null);
//							} else
//							{
//								$("theAnswerInput").value = "";
//							}
						};
		} 
		else
		{
			$("k-13").callBack = function () {
							toggleKey(this);
							removeCustomAlert(id);
						};
			$("Escape").callBack = function () {
							toggleKey(this);
							if ($("theAnswerInput").value === "")
							{
								removeCustomAlert(id);
							} else
							{
								$("theAnswerInput").value = "";
							}
						};
		}

		fullLowercaseDisplay();
	}


	$("theQuestion").innerHTML = question;
	//$("keyboard").style.display = "inline-block";
	$("theUserInput").focus();

	if ((height > 250) || (width > 1000))
	{
		mObj.classList.add("big_screen");
		if (id == 'body')
		{
			$('menu_icon').style.opacity = '0.1';
			$('intro').style.opacity = '0.1';
			$('top_bar').style.opacity = '0.1';
			$('connection_ok').style.opacity = '0.1';
		}
		else
		{
			$('content').style.opacity = '0.1';
		}
		//place in center
		mObj.style.left = ((width - 900) / 2) + 'px';
		mObj.style.top = ((height - 350) / 2) + 'px';
	}
}

/*========================================================
========================================================*/
function fullLowercaseDisplay()
{
    var okCallBack, cancelCallBack;
    var container = $("keyboard");
	var height = window.innerHeight;
	var width = window.innerWidth;
    
    okCallBack = $("k-13").callBack;
    cancelCallBack = $("Escape").callBack;
    
    while(container.getElementsByTagName('div').length)
    {
	    container.removeChild(container.childNodes[0]);
    }

	key = addKey("k-113", "q");
	if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}

    addKey("k-119", "w");
    addKey("k-101", "e");
    addKey("k-114", "r");
    addKey("k-116", "t");
    addKey("k-121", "y");
    addKey("k-117", "u");
    addKey("k-105", "i");
    addKey("k-111", "o");
    key = addKey("k-112", "p");
	if ((height < 250) || (width < 1000))
	{
		//key.style.marginRight = "60px";
	}

    key = addKey("k-97", "a");
	if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "100px";
	}
	else
	{
		key.style.clear = "both";
		key.style.marginLeft = "10px";
	}
    
    addKey("k-115", "s");
    addKey("k-100", "d");
    addKey("k-102", "f");
    addKey("k-103", "g");
    addKey("k-104", "h");
    addKey("k-106", "j");
    addKey("k-107", "k");
    addKey("k-108", "l");

    if ((height > 250) || (width > 1000))
	{
		key = addKey("k-16", "&#x21e7;");
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}
	else
	{
		key = addKey("k-16", "SHFT");
		key.style.marginRight = "7px";
		key.style.width = "40px"
	}
    
    key = addKey("k-122", "z");
	if ((height > 250) || (width > 1000))
	{
		key.style.marginLeft = "22px";
	}
	else
	{
		key.style.marginLeft = "20px";
		key.style.clear = "both";
	}
    
    addKey("k-120", "x");
    addKey("k-99", "c");
    addKey("k-118", "v");
    addKey("k-98", "b");
    addKey("k-110", "n");
    addKey("k-109", "m");
    key = addKey("k-8", "&#x2190;");
	if ((height > 250) || (width > 1000))
	{
		key.style.marginLeft = "23px";
	}

    key = addKey("k-digit", "1&");
	if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}
    
    
	if ((height > 250) || (width > 1000))
	{
		key = addKey("k-l", "&#x25C0;");
		key.style.marginLeft = "22px";
		addKey("k-r", "&#x25B6;");
	}
	else
	{
		key = addKey("k-l", "Cur L");
		key.style.marginLeft = "30px";
		key.style.width = "50px";
		key.style.clear = "both";
		key = addKey("k-r", "Cur R");
		key.style.width = "50px";
	}
	
    key = addKey("k-32", "");
    key.style.width = "179px";

    if ((height > 250) || (width > 1000))
	{
		key = addKey("k-13", "&#x2714;");
	}
	else
	{
		key = addKey("k-13", "OK");
	}
    //key.style.marginLeft = "84px";
    key.style.color = "#0c4";
    key.callBack = okCallBack;
	
	if ((height > 250) || (width > 1000))
	{
		key = addKey("Escape", "&#x2716;");
	}
	else
	{
		key = addKey("Escape", "Cancel");
		key.style.width = "50px";
	}
    
    key.style.color = "#f00";
    key.callBack = cancelCallBack;
    $("k-13").addEventListener("click", $("k-13").callBack);
    $("Escape").addEventListener("click", $("Escape").callBack);

    $("k-digit").addEventListener("click", function(){fullDigitDisplay(this);});
	
	keyboardDisplayed = "lowerCase";
}

/*========================================================
========================================================*/
function fullUppercaseDisplay()
{
    var okCallBack, cancelCallBack;
    var container = $("keyboard");
	var height = window.innerHeight;
	var width = window.innerWidth;
    
    okCallBack = $("k-13").callBack;
    cancelCallBack = $("Escape").callBack;
    
    while(container.getElementsByTagName('div').length)
    {
	    container.removeChild(container.childNodes[0]);
    }

    key = addKey("k-81", "Q");
    if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}
    addKey("k-87", "W");
    addKey("k-69", "E");
    addKey("k-82", "R");
    addKey("k-84", "T");
    addKey("k-89", "Y");
    addKey("k-85", "U");
    addKey("k-73", "I");
    addKey("k-79", "O");
    addKey("k-80", "P");

    key = addKey("k-65", "A");
    if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "100px";
	}
	else
	{
		key.style.clear = "both";
		key.style.marginLeft = "7px";
	}
    addKey("k-83", "S");
    addKey("k-68", "D");
    addKey("k-70", "F");
    addKey("k-71", "G");
    addKey("k-72", "H");
    addKey("k-74", "J");
    addKey("k-75", "K");
    addKey("k-76", "L");

    if ((height > 250) || (width > 1000))
	{
		key = addKey("k-16", "&#x21e7;");
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}
	else
	{
		key = addKey("k-16", "SHFT");
		key.style.marginRight = "10px";
		key.style.width = "40px"
	}
	
    key = addKey("k-90", "Z");
    if ((height > 250) || (width > 1000))
	{
		key.style.marginLeft = "22px";
	}
	else
	{
		key.style.marginLeft = "20px";
		key.style.clear = "both";
	}
    addKey("k-88", "X");
    addKey("k-67", "C");
    addKey("k-86", "V");
    addKey("k-66", "B");
    addKey("k-78", "N");
    addKey("k-77", "M");
    key = addKey("k-8", "&#x2190;");
    if ((height > 250) || (width > 1000))
	{
		key.style.marginLeft = "23px";
	}

    key = addKey("k-digit", "1&");
    if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}
    
	if ((height > 250) || (width > 1000))
	{
		key = addKey("k-l", "&#x25C0;");
		key.style.marginLeft = "22px";
		addKey("k-r", "&#x25B6;");
	}
	else
	{
		key = addKey("k-l", "Cur L");
		key.style.marginLeft = "30px";
		key.style.width = "50px";
		key.style.clear = "both";
		key = addKey("k-r", "Cur R");
		key.style.width = "50px";
	}
	
    key = addKey("k-32", "");
    key.style.width = "179px";

    if ((height > 250) || (width > 1000))
	{
		key = addKey("k-13", "&#x2714;");
	}
	else
	{
		key = addKey("k-13", "OK");
	}
    key.style.color = "#0c4";
    key.callBack = okCallBack;
    if ((height > 250) || (width > 1000))
	{
		key = addKey("Escape", "&#x2716;");
	}
	else
	{
		key = addKey("Escape", "Cancel");
		key.style.width = "50px";
	}
    key.style.color = "#f00";
    key.callBack = cancelCallBack;
    $("k-13").addEventListener("click", $("k-13").callBack);
    $("Escape").addEventListener("click", $("Escape").callBack);

    $("k-digit").addEventListener("click", function(){fullDigitDisplay(this);});
	
	keyboardDisplayed = "upperCase";
}

/*========================================================
========================================================*/
function fullDigitDisplay()
{
    var okCallBack, cancelCallBack;
    var container = $("keyboard");
	var height = window.innerHeight;
	var width = window.innerWidth;
	
    okCallBack = $("k-13").callBack;
    cancelCallBack = $("Escape").callBack;    
    

    while (container.getElementsByTagName('div').length)
	{
		container.removeChild(container.childNodes[0]);
	}

    key = addKey("k-35", "#");
    if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}
    addKey("k-42", "*");
    addKey("k-38", "&");
    addKey("k-95", "_");
    addKey("k-45", "-");
    addKey("k-49", "1");
    addKey("k-50", "2");
    addKey("k-51", "3");
    addKey("k-63", "?");
    key = addKey("k-33", "!");
	if ((height < 250) || (width < 1000))
	{
		key.style.marginRight = "20px";
	}

    key = addKey("k-64", "@");
    if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}
	else
	{
		key.style.clear = "both";
	}
    addKey("k-40", "(");
    addKey("k-41", ")");
    addKey("k-61", "=");
    addKey("k-43", "+");
    addKey("k-52", "4");
    addKey("k-53", "5");
    addKey("k-54", "6");
    addKey("k-59", ";");
    addKey("k-58", ":");

    key = addKey("k-34", "\"");
    if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}
	else
	{
		key.style.clear = "both";
	}
    addKey("k-39", "'");
    addKey("k-92", "\\");
    addKey("k-37", "%");
    addKey("k-47", "/");
    addKey("k-55", "7");
    addKey("k-56", "8");
    addKey("k-57", "9");
    addKey("k-44", ",");
    key = addKey("k-8", "&#x2190;");
    //key.style.marginLeft = "42px";

    key = addKey("k-abc", "abc");
    if ((height > 250) || (width > 1000))
	{
		key.style.clear = "both";
		key.style.marginLeft = "80px";
	}
	else
	{
		key.style.clear = "both";
	}
	
    if ((height > 250) || (width > 1000))
	{
		key = addKey("k-l", "&#x25C0;");
		key.style.marginLeft = "22px";
		addKey("k-r", "&#x25B6;");
	}
	else
	{
		key = addKey("k-l", "Cur L");
		key.style.width = "50px";
		//key.style.clear = "both";
		key = addKey("k-r", "Cur R");
		key.style.width = "50px";
	}	
	
    key = addKey("k-32", "");
    key.style.width = "97px";
    addKey("k-48", "0");
    addKey("k-46", ".");


    if ((height > 250) || (width > 1000))
	{
		key = addKey("k-13", "&#x2714;");
	}
	else
	{
		key = addKey("k-13", "OK");
	}
    key.style.color = "#0c4";
    key.callBack = okCallBack;
    if ((height > 250) || (width > 1000))
	{
		key = addKey("Escape", "&#x2716;");
	}
	else
	{
		key = addKey("Escape", "Esc");

	}
    key.style.color = "#f00";
    key.callBack = cancelCallBack;
	

    $("k-13").addEventListener("click", $("k-13").callBack);
    $("Escape").addEventListener("click", $("Escape").callBack);    

    $("k-abc").addEventListener("click", function(){fullLowercaseDisplay(this);});
	
	keyboardDisplayed = "digit";
}

/*========================================================
========================================================*/
function removeCustomAlert(id) 
{
  var height = window.innerHeight;
    var width = window.innerWidth;
	
    document.documentElement.style.overflowY = 'scroll';	
	
	if ((height > 250) || (width > 1000))
    {
		if (id == 'body')
		{
			$('menu_icon').style.opacity = '1';
			$('intro').style.opacity = '1';
			$('top_bar').style.opacity = '1';
			$('connection_ok').style.opacity = '1';
		}
		else
		{
			$('content').style.opacity = '1';
		}
	}
	
    $("modalContainer").classList.add("shrink");
	

	eraseTimeout = setTimeout(function(){document.getElementsByTagName("body")[0].removeChild($("modalContainer"));}, 200);
	freeScreen = true;
}

/*========================================================
========================================================*/
function addKey(id, text)
{
    var key;

    var keyboardObj = $("keyboard");
    key = keyboardObj.appendChild(document.createElement("div"));
    key.id = id;
    key.innerHTML = text;
    key.addEventListener("click", function(){keyClick(this);});

    return key;
}

function addButton(id, text)
{
    var key;

    var keyboardObj = $("keyboard");
    key = keyboardObj.appendChild(document.createElement("button"));
    key.id = id;
    key.innerHTML = text;
    key.addEventListener("click", function(){keyClick(this);});

    return key;
}


/*========================================================
========================================================*/
function keyDown(event)
{
    var keycode = event.which;

    var cell = $("theAnswerInput");
    if (cell)
    {
		if(!hmiScreen)
		{		
			cell.focus();
		}
	}
	
	if ($("modalContainer")) {
		switch(keycode)
		{
		case 16:
			if((keyboardDisplayed === "lowerCase") /*&& (event.getModifierState("CapsLock"))*/)
			{
				fullUppercaseDisplay();
				$("k-16").style.backgroundColor = '#fff';
				$("k-16").style.color = '#0ee';			
			}
		break;
		case 27:
			$("Escape").callBack();
			return;
		break; 
		case 37:
			toggleKey($("k-l"));
		break;
		case 39:
			toggleKey($("k-r"));
		break;
		case  8:
			toggleKey($("k-8"));
		break;
		}
	} 
	else {
		return;
	}

}

/*========================================================
========================================================*/
function keyUp(event)
{
    var keycode = event.which;

    var cell = $("theAnswerInput");
    if (cell)
    {
		if(!hmiScreen)
		{
			cell.focus();
		}
    }

    switch(keycode)
	{
	case 16:
		if((keyboardDisplayed === "upperCase") /*&& (!event.getModifierState("CapsLock"))*/)
		{
			fullLowercaseDisplay();
			$("k-16").style.backgroundColor = '#888';
			$("k-16").style.color = '#888';
			setTimeout(function () {
				$("k-16").style.backgroundColor = 'transparent';
				$("k-16").style.color = '#fff';
				}, 100);
		}
    break;
	/*case 20:
		if(event.getModifierState("CapsLock"))
		{
			if(keyboardDisplayed === "lowerCase")
			{
				fullUppercaseDisplay();
				$("k-16").style.backgroundColor = '#fff';
				$("k-16").style.color = '#0ee';			
			}
		}
		else
		{
			if(keyboardDisplayed === "upperCase")
			{
				fullLowercaseDisplay();
				$("k-16").style.backgroundColor = '#888';
				$("k-16").style.color = '#888';
				setTimeout(function () {
					$("k-16").style.backgroundColor = 'transparent';
					$("k-16").style.color = '#fff';
					}, 100);
			}
		}
	break;*/
	}
}

/*========================================================
========================================================*/
function keyPressed(event)
{
    var keycode = event.which;
	var cell = $("theAnswerInput");
	var checkValid = $("modalContainer")
    if (cell)
    {
		if(!hmiScreen)
		{
			cell.focus();
		}
    }

    if (keycode === 13)
    {
		if ($("modalContainer")) {
			$("k-13").callBack();
			return;
		} 
		else {
			return;
		}
    } 

	

    if (keycode !== 8)
    {
		if((keycode >= 97) && (keycode <= 122))
		{
			if((keyboardDisplayed !== "lowerCase") && (keyboardDisplayed !== "digital"))
			{
				var display = false;

				if (typeof keysToDisplay[0] !== "undefined")
				{
					for (var i = 0; i < keysToDisplay.length; i++)
					{
						if (keysToDisplay[i] === event.key)
						{
							display = true;
						}
					}
				}
				else
				{
					display = true;
				}
				
				if (display === true)
				{
					fullLowercaseDisplay();
				
					$("k-16").style.backgroundColor = '#888';
					$("k-16").style.color = '#888';
					setTimeout(function () {
						$("k-16").style.backgroundColor = 'transparent';
						$("k-16").style.color = '#fff';
						}, 100);
				}	
			}
		}
		else if((keycode >= 65) && (keycode <= 90))
		{
			if((keyboardDisplayed !== "upperCase") && (keyboardDisplayed !== "digital"))
			{
				var display = false;

				if (typeof keysToDisplay[0] !== "undefined")
				{
					for (var i = 0; i < keysToDisplay.length; i++)
					{
						if (keysToDisplay[i] === event.key)
						{
							display = true;
						}
					}
				}
				else
				{
					display = true;
				}

				if (display === true)
				{
					fullUppercaseDisplay();
					
					$("k-16").style.backgroundColor = '#fff';
					$("k-16").style.color = '#0ee';	
				}
			}
		}
		else if(((keycode >= 33) && (keycode <= 64)) || (keycode === 95))
		{
			if((keyboardDisplayed !== "digit") && (keyboardDisplayed !== "digital"))
			{
				if (typeof keysToDisplay[0] !== "undefined")
				{
					for (var i = 0; i < keysToDisplay.length; i++)
					{
						if (keysToDisplay[i] === event.key)
						{
							fullDigitDisplay();
						}
					}
				}
				else
				{
					fullDigitDisplay();
				}
				
			}
		}
		
		if($("k-" + keycode))
		{
			writeKeyStroke(keycode);
		}
    }
	
    if (LoginFocus === 'pass') {
    	var randomKey = getRandomKey();
    	if(randomKey) {toggleKey($(randomKey));}
    }
    else {
    	toggleKey($("k-" + keycode));
    }
}	

/*========================================================
========================================================*/
function keyClick(key)
{
	var keyCode;
    var cell = $("theAnswerInput")
    if(cell)
    {
		if(!hmiScreen)
		{		
			cell.focus();
		}
    }

	keyCode = key.id.split("-")[1];
	
	if(keyCode === '16')
	{
		if((keyboardDisplayed === "lowerCase") /*&& (event.getModifierState("CapsLock"))*/)
		{
			fullUppercaseDisplay();
			$("k-16").style.backgroundColor = '#fff';
			$("k-16").style.color = '#0ee';			
		}
		else if((keyboardDisplayed === "upperCase") /*&& (event.getModifierState("CapsLock"))*/)
		{
			fullLowercaseDisplay();
			$("k-16").style.backgroundColor = '#888';
			$("k-16").style.color = '#888';
			setTimeout(function () {
				$("k-16").style.backgroundColor = 'transparent';
				$("k-16").style.color = '#fff';
				}, 100);
		}
	}
	else if ((key.id !== "Escape") && (key.id !== "k-13"))
	{
		toggleKey($("k-" + keyCode));
		writeKeyStroke(keyCode);
	}
}
	
/*========================================================
========================================================*/
function toggleKey(el) 
{
	if(el === null)
	{
		return;
	}
	
    el.style.backgroundColor = '#fff';
    el.style.color = '#0ee';

    setTimeout(function () {
		el.style.backgroundColor = '#888';
		el.style.color = '#888';
		}, 200);
    setTimeout(function () {
		el.style.backgroundColor = 'transparent';
		switch(el.id)
		{
		case 'k-13': el.style.color = '#0c4'; break;
		case 'Escape': el.style.color = '#f00'; break;
		default: el.style.color = '#fff'; break;
		}
		}, 300);
}

/*========================================================
========================================================*/
function getFocus(field)
{
	LoginFocus = field;
}

function writeKeyStroke(keycode)
{
	var TextArea = document.getElementById("theAnswerInput");
	var UserArea = document.getElementById("theUserInput");	
	var PassArea = document.getElementById("thePasswordInput");
	
	if((!TextArea) && ((!UserArea) || (!PassArea)))
	{
		return;
	}
	
    var currentText, currentText2;
	var position, position2;
	if (TextArea)
	{
		replaceSelection(TextArea);
		currentText = TextArea.value;
		position = getCaretPosition(TextArea);
		LoginFocus = 'TextArea';		
	}
	else if ((UserArea) && (PassArea))
	{
		replaceSelection((LoginFocus === 'user') ? UserArea : PassArea);
		currentText = UserArea.value;
		position = getCaretPosition(UserArea);
		currentText2 = PassArea.value;
		position2 = getCaretPosition(PassArea);
	}
	else
	{
		
	}
	var newText, userCpy, passCpy;
	
	//var txt = TextArea.innerHTML;

	//if (UserArea === document.activeElement)
	if (LoginFocus === 'user')
	{
		switch(keycode)
		{
		case 'digit':
		case 'abc':
		break;
		case 'l':
			setCaretPosition(UserArea, position-1);
		break;
		case 'r':
			setCaretPosition(UserArea, position+1);
		break;
		case '8': 
			//txt = txt.slice(0, -1);
			newText = [currentText.slice(0, position-1), currentText.slice(position)].join('');
			UserArea.value = newText;
			setCaretPosition(UserArea, position-1);
		break;
		default:
			newText = [currentText.slice(0, position), String.fromCharCode(keycode), currentText.slice(position)].join('');		
			UserArea.value = newText;
			setCaretPosition(UserArea, position+1);
		break;
		};
	}
	//else if (PassArea === document.activeElement)
	else if (LoginFocus === 'pass')
	{
		switch(keycode)
		{
		case 'digit':
		case 'abc':
		break;
		case 'l':
			setCaretPosition(PassArea, position2-1);
		break;
		case 'r':
			setCaretPosition(PassArea, position2+1);
		break;
		case '8': 
			//txt = txt.slice(0, -1);
			newText = [currentText2.slice(0, position2-1), currentText2.slice(position2)].join('');
			PassArea.value = newText;
			setCaretPosition(PassArea, position2-1);
		break;
		default:
			newText = [currentText2.slice(0, position2), String.fromCharCode(keycode), currentText2.slice(position2)].join('');		
			PassArea.value = newText;
			setCaretPosition(PassArea, position2+1);
		break;
		};
	}
	else
	{
		switch(keycode)
		{
		case 'digit':
		case 'abc':
		break;
		case 'l':
			setCaretPosition(TextArea, position-1);
		break;
		case 'r':
			setCaretPosition(TextArea, position+1);
		break;
		case '8': 
			//txt = txt.slice(0, -1);
			newText = [currentText.slice(0, position-1), currentText.slice(position)].join('');
			TextArea.value = newText;
			setCaretPosition(TextArea, position-1);
		break;
		default:
			newText = [currentText.slice(0, position), String.fromCharCode(keycode), currentText.slice(position)].join('');		
			TextArea.value = newText;
			setCaretPosition(TextArea, position+1);
		break;
		};
	}
    
}

/*========================================================
========================================================*/
function getCaretPosition (elementId) 
{
    // Initialize
    var iCaretPos = 0;

    // IE Support
    if (document.selection)
	{
		// Set focus on the element
		if(!hmiScreen)
		{		
			elementId.focus();
		}

		// To get cursor position, get empty selection range
		var oSel = document.selection.createRange();

		// Move selection start to 0 position
		oSel.moveStart('character', -elementId.value.length);

		// The caret position is selection length
		iCaretPos = oSel.text.length;
	}

    // Firefox support
    else if (elementId.selectionStart || elementId.selectionStart == '0')
	    iCaretPos = elementId.selectionStart;

    // Return results
    return iCaretPos;
}

/*========================================================
========================================================*/
function setCaretPosition(ctrl, pos)
{
    if (ctrl.setSelectionRange)
	{
		if(!hmiScreen)
		{
			ctrl.focus();
		}
		ctrl.setSelectionRange(pos, pos);
	} 
	else if (ctrl.createTextRange)
	{
		var range = ctrl.createTextRange();
		range.collapse(true);
		range.moveEnd('character', pos);
		range.moveStart('character', pos);
		range.select();
	}
}

/*========================================================
========================================================*/
function replaceSelection(elem)
{
    if (elem.selectionEnd > elem.selectionStart)
	{
    	var start = elem.selectionStart;
    	elem.value = elem.value.slice(0, start) + elem.value.slice(elem.selectionEnd);
    	elem.selectionEnd = elem.selectionStart = start;    	
	} 
}

/*========================================================
========================================================*/
function getRandomKey() {
	var keyboard = $('keyboard');
	
	if (keyboard)
	{
		var keys = [], 
			divs = keyboard.querySelectorAll('div');
		
		for(var i = 0; i < divs.length; i++) {
			var elem = divs[i];
			if (divs[i].id) {keys.push(divs[i].id)};
		}
		
		if (keys.length)
		{
			var randomIndex = Math.floor(Math.random() * keys.length); 			
			return keys[randomIndex];
		}		
	}
		
	return '';
}
	
/*========================================================
========================================================*/
function etlDelayedAlert(msg, id)
{
	setTimeout(function(){etlAlert(msg, id);}, 200);
}

/*========================================================
========================================================*/
function etlValidateLoginInputs(user, pass, id)
{
	// check if the user canceled the form
	if ((typeof(user) !== 'string') || ((typeof(pass) !== 'string'))) {return false;}
	
	if ((!user) && (!pass))	{
		etlDelayedAlert('Please complete the form.', id);
		return false;
	}
	
	if (!user) {
		etlDelayedAlert('Please fill in the username.', id);
		return false;
	}
	
	if (!pass) {
		etlDelayedAlert('Please fill in the password.', id);
		return false;
	} 
	
	return true;
}

function decodeHtml(html) {
    var txt = document.createElement("textarea");
    txt.innerHTML = html;
    return txt.textContent;
}

function getFileName(path) {
	try{
		var name = path.split("/").slice(-1)[0];
		return name;
	}
	catch(err){
		return '';
	}
}

function getFileParentDirectory(path) {
	try{
		var parent = path.split("/").slice(-2)[0];
		return parent;
	}
	catch(err){
		return '';
	}
}

function redirectWhenRestrictedAccess(currentUser, lastLoggedInUser, cb) 
{
	if ((!currentUser) || (currentUser === "Access Locked")) {
		if (lastLoggedInUser) {
			// redirect only if on etl or config sections or if on a module engineer.htm
			var frameElem = $('main_page');	
			if (frameElem){
				var src = frameElem.src, parentDir = getFileParentDirectory(src);
				if ((["config","etl"].includes(parentDir)) || (getFileName(src) === 'engineer.htm') ) {
					cb();
				}
			}
		}
		return '';
	}
	else {
		return currentUser;		
	}
}
