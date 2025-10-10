const fetchInterval = 500; // 500 milliseconds
var lastLoggedInUser = "";
var menu_shown;
var default_margin;
var left_position;
var previous_screen_width;
var _status = [];
var main_page_showing = null;
var main_page_showing = null;
var current_module = "none",
  oddRow = false;
const getLanguage = () =>
  navigator.userLanguage ||
  (navigator.languages &&
    navigator.languages.length &&
    navigator.languages[0]) ||
  navigator.language ||
  navigator.browserLanguage ||
  navigator.systemLanguage ||
  "en";

var indexCopy = "";

//init page
function initPage() {
  menu_shown = false;
  previous_screen_width = window.innerWidth;

  //force the size of elements

  default_margin = document.getElementById("top_bar").offsetLeft;
  left_position = document.getElementById("main_page").offsetLeft;

  //width of main frame
  document.getElementById("main_page").style.width =
    window.innerWidth - (default_margin + left_position) + "px";
  //height of element
  document.getElementById("menu_page").style.height =
    window.innerHeight -
    (document.getElementById("menu_top").offsetTop +
      document.getElementById("menu_page").offsetTop +
      default_margin) +
    "px";
  document.getElementById("main_page").style.height =
    window.innerHeight -
    (document.getElementById("main_top").offsetTop + default_margin) +
    "px";

  //check if we can display menu if enough space
  if (window.innerWidth > 600) {
    //300 for menu, 300 for main page
    changeMenuIcon(document.getElementById("menu_icon"));
  }

  //load default page if none call the website
  if (typeof Storage !== "undefined") {
    var Page = sessionStorage.getItem("PageForced");
    if (Page === "" || Page === null) {
      document.getElementById("main_page").src = "./summary.htm";
    } else {
      document.getElementById("main_page").src = Page;
      //put back default webpage
      sessionStorage.setItem("PageForced", "./summary.htm");
    }
  } else {
    document.getElementById("main_page").src = "./summary.htm";
  }

  placeLogo();
}

//place logo
function placeLogo() {
  if (
    window.innerHeight - document.getElementById("logo_position").offsetTop <
    310
  ) {
    //hide the logo
    document.getElementById("logo").style.display = "none";
    document.getElementById("logo_position").style.height = "0px";
  } else {
    document.getElementById("logo").style.display = "block";
    document.getElementById("logo_position").style.height =
      window.innerHeight -
      (document.getElementById("logo_position").offsetTop + 310) +
      "px";
  }
}
function manageHex_json(index) {
  var smallSection = $("hex_small_section");
  var i, j;
  var indexSplit;
  var newIndex = [];

  var indexHexDisplayed = 0;

  var module_status = index.module_status;
  if (Array.isArray(module_status)) {
    for (i = 0; i < module_status.length; i++) {
      if ($("mod_m" + i + 1)) {
        /*if($('mod_m'+j).classList.contains("last_line"))
				{
					$('mod_m'+j).classList.remove("last_line");
				}*/

        $("mod_m" + i + 1).style.cssFloat = "left";
        $("mod_m" + i + 1).style.marginLeft = "0px";
        $("mod_m" + i + 1 + "_text").style.marginLeft = "0px";
      }
    }
  }
}
//restyle hexagons with staggered pattern
function manageHex(indexStr) {
  var smallSection = $("hex_small_section");
  var i, j;
  var indexSplit;
  var newIndex = [];

  var indexHexDisplayed = 0;
  if (smallSection) {
    //put back from scratch
    indexSplit = indexStr.split("|");

    //only take module status
    for (i = 7; i < indexSplit.length; i++) {
      newIndex.push(indexSplit[i]);
    }

    for (i = 1; i <= 12; i++) {
      // remove all styling, to avoid styles persisting after change
      /*if (smallSection.childNodes[i])
			{
				smallSection.childNodes[i].style.removeProperty('float');
				smallSection.childNodes[i].style.removeProperty('margin-left');
			}*/

      switch (i) {
        // case 36:
        // case 37:
        // case 38:
        // case 39:
        //   j = i + 2;
        //   break;
        // case 40:
        //   j = 36;
        //   break;
        // case 41:
        //   j = 37;
        //   break;
        default:
          j = i;
          break;
      }

      if ($("mod_m" + j)) {
        /*if($('mod_m'+j).classList.contains("last_line"))
				{
					$('mod_m'+j).classList.remove("last_line");
				}*/

        $("mod_m" + j).style.cssFloat = "left";
        $("mod_m" + j).style.marginLeft = "0px";
        $("mod_m" + j + "_text").style.marginLeft = "0px";
      }

      switch (newIndex[j]) {
        default:
          indexHexDisplayed++;
          switch (indexHexDisplayed) {
            case 1:
            case 10:
            case 19:
            case 28:
            case 37:
            // case 41:
            //   $("mod_m" + j).style.marginLeft = "28px";
            //   break;

            case 4:
            case 9:
            case 13:
            case 18:
            case 22:
            case 27:
            case 31:
            case 36:
              // case 40:
              //   //$('mod_m'+j).style.marginLeft = '3px';
              //   $("mod_m" + j).style.cssFloat = "none";

              //   $("mod_m" + j + "_text").style.width = "56px";
              //   var t = indexHexDisplayed;
              //   if (t == 9 || t == 18 || t == 27 || t == 36) {
              //     $("mod_m" + j + "_text").style.marginLeft = "224px";
              //   } else {
              //     $("mod_m" + j + "_text").style.marginLeft = "195px";
              //   }

              //$('mod_m'+j).style.removeProperty('float');
              //$('mod_m'+j).classList.add("last_line");
              break;

            default:
              //$('mod_m'+j).style.marginLeft = '0px';
              break;
          }

          //marginInit += 10;
          //smallSection.style.marginBottom = marginInit + "px";

          break;
        case "U":
          //do nothing because not displayed
          break;
      }
    }
  }
}

//menu page and icon
function changeMenuIcon(x) {
  x.classList.toggle("change");
  document.getElementById("menu_top").classList.toggle("show_menu");
  if (window.innerWidth > 600) {
    document.getElementById("main_top").classList.toggle("move_main");

    if (menu_shown) {
      //hidding menu, increase main page width
      document.getElementById("main_page").style.width =
        window.innerWidth - (default_margin + left_position) + "px";
    } else {
      //showing menu, decrease main page width
      document.getElementById("main_page").style.width =
        window.innerWidth - (default_margin + left_position + 300) + "px";
    }
  } else {
    document.getElementById("main_top").classList.toggle("hide_main");
  }
  if (menu_shown) {
    menu_shown = false;
  } else {
    menu_shown = true;
  }
}

//menu part
/*function toggleMenu(menu_index)
{
	document.getElementById("svg_hex"+menu_index+"_id").classList.toggle("rotate_hex");	
	document.getElementById("menu_part"+menu_index+"_title_id").classList.toggle("move_hexagon_medium");	
	document.getElementById("menu_part"+menu_index+"_content_id").classList.toggle("show_hide_menu_content");	
}*/

//select page
function pageSelected(page) {
  var CellSelected, CellText, index, status, i;
  var HexSmall = $("hex_small_section");
  var SelectedId;
  /*var TempSplit;
	//check it is not an absent module
	TempSplit = page.split('/');
	
	if(document.getElementById("mod_"+TempSplit[1]))
	{
		if(document.getElementById("mod_"+TempSplit[1]).className.search("absent") !== -1)
		{
			//alert("Module Absent");
			return;
		}
	}*/
  //page should exist
  if (window.innerWidth <= 600) {
    //auto hide menu
    changeMenuIcon(document.getElementById("menu_icon"));
  }

  main_page_showing = null;
  //set the selected menu
  switch (page) {
    case "summary.htm":
      CellSelected = $("sum_title");
      current_module = "none";

      //default text
      // change button style
      $("help_title_text").removeAttribute("style");
      $("log_title_text").removeAttribute("style");
      // change button text
      $("help_title_text").innerHTML = "HELP";
      $("log_title_text").innerHTML = "LOG";
      break;
    case "config.htm":
      CellSelected = $("config_title");
      page = "./config/config.htm";
      current_module = "none";

      //default text
      // change button style
      $("help_title_text").removeAttribute("style");
      $("log_title_text").removeAttribute("style");
      // change button text
      $("help_title_text").innerHTML = "HELP";
      $("log_title_text").innerHTML = "LOG";
      break;
    case "log.htm":
      CellSelected = $("log_title");
      for (i = 0; i < HexSmall.childNodes.length; i++) {
        if (HexSmall.childNodes[i].classList.contains("selected")) {
          current_module = HexSmall.childNodes[i].id;
        }
      }
      /*if (current_module !== "none")
		{
			index = parseInt(current_module.match(/[0-9]+/), 10);
			page = "/mod_" + ("000" + index).slice(-3) + "/log.htm";
		}*/
      break;
    case "upgrade.htm":
      CellSelected = $("upgrade_title");
      page = "./config/upgrade.htm";
      current_module = "none";

      //default text
      // change button style
      $("help_title_text").removeAttribute("style");
      $("log_title_text").removeAttribute("style");
      // change button text
      $("help_title_text").innerHTML = "HELP";
      $("log_title_text").innerHTML = "LOG";
      break;
    case "help.htm":
      CellSelected = $("help_title");
      for (i = 0; i < HexSmall.childNodes.length; i++) {
        if (HexSmall.childNodes[i].classList.contains("selected")) {
          current_module = HexSmall.childNodes[i].id;
        }
      }
      if (current_module !== "none") {
        index = parseInt(current_module.match(/[0-9]+/), 10);
        page = "/mod_" + ("000" + index).slice(-3) + "/help.htm";
      }
      break;
    case "video.htm":
      CellSelected = null;

      //default text
      // change button style
      $("help_title_text").removeAttribute("style");
      $("log_title_text").removeAttribute("style");
      // change button text
      $("help_title_text").innerHTML = "HELP";
      $("log_title_text").innerHTML = "LOG";

      break;
    default:
      index = page.indexOf("mod_");
      if (index !== -1) {
        index = parseInt(page.match(/[0-9]+/), 10);
        CellSelected = $("mod_m" + index);
        CellText = $("mod_m" + index + "_text");
        status = _status[index];
        main_page_showing = index;
        if (status !== "A" && status !== "B" && status !== "C") {
          // change help style
          $("help_title_text").style.top = "-73px";
          $("log_title_text").style.top = "-73px";
          // change help button text
          $("help_title_text").innerHTML =
            "HELP<br>(" + CellText.innerHTML + ")";
          $("log_title_text").innerHTML = "LOG<br>(" + CellText.innerHTML + ")";
        }
      }
      switch (status) {
        case "A":
          page = "./module_absent.htm"; //that page should not be called
          return;
          break;
        case "B":
          page = "./module_bootloader.htm";
          break;
        case "C":
          page = "./module_communication.htm";
          break;
      }
      break;
  }

  //if we reach here it means link has changed
  var cusid_ele = document.getElementsByClassName("hexagon_medium_element");
  for (var i = 0; i < cusid_ele.length; ++i) {
    if (cusid_ele[i].classList.contains("selected")) {
      cusid_ele[i].classList.remove("selected");
    }
  }
  cusid_ele = document.getElementsByClassName("hexagon_small_element");
  for (var i = 0; i < cusid_ele.length; ++i) {
    if (cusid_ele[i].classList.contains("selected")) {
      cusid_ele[i].classList.remove("selected");
    }
  }

  //set the selected one
  if (CellSelected) {
    CellSelected.classList.add("selected");
  }
  document.getElementById("main_page").src = page;
  //document.getElementById("main_top").classList.toggle("full_hide_main");
}

function select_module(module_slot) {
  var HexSmall = $("hex_small_section");
  CellSelected = $(module_slot);
  main_page_showing = null;
  fetch(module_slot + ".json",{
    method: 'GET',
    cache: 'no-cache', // Specify 'no-cache' to prevent caching
  })
    .then((response) => response.json())
    .then((module) => {
      document.getElementById("main_page").src = module.path;
      index = parseInt(module_slot.split('_m'), 10)[1];
      main_page_showing = index;
      //if we reach here it means link has changed
      var cusid_ele = document.getElementsByClassName("hexagon_medium_element");
      for (var i = 0; i < cusid_ele.length; ++i) {
        if (cusid_ele[i].classList.contains("selected")) {
          cusid_ele[i].classList.remove("selected");
        }
      }
      cusid_ele = document.getElementsByClassName("hexagon_small_element");
      for (var i = 0; i < cusid_ele.length; ++i) {
        if (cusid_ele[i].classList.contains("selected")) {
          cusid_ele[i].classList.remove("selected");
        }
      }
      //set the selected one
      if (CellSelected) {
        CellSelected.classList.add("selected");
      }
    })
    .catch((error) => {
      console.error("Error fetching data:", error);
    });
}
//resizedw
var doit;
function resizedw() {
  // Haven't resized in 100ms!
  //alert('resize now');

  //if the new window is lower than 600 and menu is shown because previous was greater than 600
  if (menu_shown && window.innerWidth <= 600 && previous_screen_width > 600) {
    //close the menu
    document.getElementById("menu_icon").classList.toggle("change");
    document.getElementById("menu_top").classList.toggle("show_menu");
    document.getElementById("main_top").classList.toggle("move_main");
    menu_shown = false;
  }

  //if the new window is greater than 600 and menu is shown with previous lower than 600
  if (menu_shown && window.innerWidth > 600 && previous_screen_width <= 600) {
    //leave the menu open
    //remove opacity of main page
    document.getElementById("main_top").classList.toggle("hide_main");
    //and move the main page
    document.getElementById("main_top").classList.toggle("move_main");
  }

  //width of main frame

  //if(window.innerWidth > 600)
  {
    //document.getElementById("main_top").classList.toggle("move_main");

    /*if(menu_shown)
		{
			//hidding menu, increase main page width
			document.getElementById("main_page").style.width = (window.innerWidth - (default_margin+left_position)) + 'px';
		}
		else
		{
			//showing menu, decrease main page width
			document.getElementById("main_page").style.width = (window.innerWidth - (default_margin+left_position+300)) + 'px';
		}*/

    if (menu_shown && window.innerWidth > 600) {
      //hidding menu, increase main page width
      document.getElementById("main_page").style.width =
        window.innerWidth - (default_margin + left_position + 300) + "px";
    } else {
      //showing menu, decrease main page width
      document.getElementById("main_page").style.width =
        window.innerWidth - (default_margin + left_position) + "px";
    }
  }
  /*else
	{
		document.getElementById("main_top").classList.toggle("hide_main");
	}*/

  /*if(menu_shown)
	{
		//hidding menu, increase main page width
		document.getElementById("main_page").style.width = (window.innerWidth - (default_margin+left_position+300)) + 'px';
	}
	else
	{
		//showing menu, decrease main page width
		document.getElementById("main_page").style.width = (window.innerWidth - (default_margin+left_position)) + 'px';
	}*/
  //height of element
  document.getElementById("menu_page").style.height =
    window.innerHeight -
    (document.getElementById("menu_top").offsetTop +
      document.getElementById("menu_page").offsetTop +
      default_margin) +
    "px";
  document.getElementById("main_page").style.height =
    window.innerHeight -
    (document.getElementById("main_top").offsetTop + default_margin) +
    "px";

  placeLogo();

  //update previous data
  previous_screen_width = window.innerWidth;
}

//resizeWindow
function resizeWindow() {
  clearTimeout(doit);
  doit = setTimeout(resizedw, 300);
  {
  }
}

//reload
function reload() {
  {
  }
}

function $(id) {
  if (document.getElementById(id)) {
    return document.getElementById(id);
  } else {
    return null;
  }
}

var indexCommErr = 0;

function fecth_index() {
  var i = 0;
  fetch("index.json",{
    method: 'GET',
    cache: 'no-cache', // Specify 'no-cache' to prevent caching
  }) // Replace with your API endpoint
    .then((response) => response.json())
    .then((system) => {
      // Process the fetched JSON data
      //chassis name
      $("chassis_name").innerHTML = system.name;
      //chassis type
      $("chassis_type").innerHTML = system.type;
      switch (system.alarm) {
        case 2:
          $("top_bar").setAttribute("class", "horizontal_line warning");
          $("menu_page").setAttribute("class", "menu_page warning");
          $("bar1").setAttribute("class", "bar1 warning");
          $("bar2").setAttribute("class", "bar2 warning");
          $("bar3").setAttribute("class", "bar3 warning");
          break;
        case 1:
          $("top_bar").setAttribute("class", "horizontal_line alarm");
          $("menu_page").setAttribute("class", "menu_page alarm");
          $("bar1").setAttribute("class", "bar1 alarm");
          $("bar2").setAttribute("class", "bar2 alarm");
          $("bar3").setAttribute("class", "bar3 alarm");
          break;
        default:
          $("top_bar").setAttribute("class", "horizontal_line");
          $("menu_page").setAttribute("class", "menu_page");
          $("bar1").setAttribute("class", "bar1");
          $("bar2").setAttribute("class", "bar2");
          $("bar3").setAttribute("class", "bar3");
          break;
      }
      //delete any alarm
      elementClassList = $("sum_title").classList;
      if (elementClassList.contains("warning")) {
        elementClassList.remove("warning");
      }
      if (elementClassList.contains("alarm")) {
        elementClassList.remove("alarm");
      }
      elementClassList = $("config_title").classList;
      if (elementClassList.contains("warning")) {
        elementClassList.remove("warning");
      }
      if (elementClassList.contains("alarm")) {
        elementClassList.remove("alarm");
      }
      switch (system.status) {
        case 0:
          $("sum_title").classList.add("warning");
          break;
        case 1:
          $("sum_title").classList.add("alarm");
          break;
      }
      switch (system.config) {
        case 0:
          $("config_title").classList.add("warning");
          break;
        case 1:
          $("config_title").classList.add("alarm");
          break;
      }
      var module = system.module_status;
      var i = 1;
      if (Array.isArray(module)) {
        module.forEach((state) => {
          setModuleHexMenu("mod_m" + i++, state);
          //this will automatically display appropriate page in case of communication problem with the module while displaying it
          if (main_page_showing === i) {
            page = $("main_page").src;
            //alert(page);

            if ((page.indexOf("module_absent") === -1) & (State === "A")) {
              //$("main_page").src = "./module_absent.htm";
              pageSelected("summary.htm");
            } else if (
              (page.indexOf("module_bootloader") === -1) &
              (state === "B")
            ) {
              //$("main_page").src = "./module_bootloader.htm";
              pageSelected("summary.htm");
            } else if (
              (page.indexOf("module_communication") === -1) &
              (state === "C")
            ) {
              //$("main_page").src = "./module_communication.htm";
              pageSelected("summary.htm");
            }
          }
          manageHex_json(system);
          placeLogo();
          if (system.user.length > 0) {
            $("login_button").innerHTML = system.user;
          } else {
            $("login_button").innerHTML = "Login";
          }
          lastLoggedInUser = redirectWhenRestrictedAccess(
            system.user,
            lastLoggedInUser,
            function () {
              pageSelected("summary.htm");
            }
          );
          $("index_time_value").innerHTML = system.time;
          $("index_date_value").innerHTML = system.date;
        });
      }
    })
    .catch((error) => {
      indexCommErr++;
      if (indexCommErr > 5) {
        $("connection_lost").style.display = "block";
        $("connection_ok").style.display = "none";
        $("top_bar").setAttribute("class", "horizontal_line alarm");
        $("bar1").setAttribute("class", "bar1 alarm");
        $("bar2").setAttribute("class", "bar2 alarm");
        $("bar3").setAttribute("class", "bar3 alarm");
        console.log("Error fetching data:", error);
      }
    });
}

function setModuleHexMenu(id, status) {
  var Cell = $(id);
  if (Cell) {
    if (Cell.classList.contains("selected")) {
      Cell.setAttribute("class", "hexagon_small_element");
      Cell.classList.add("selected");
    } else {
      Cell.setAttribute("class", "hexagon_small_element");
    }
    switch (status) {
      case "U": //not fitted
        Cell.classList.add("unfitted");
        //Cell.setAttribute("class", "hexagon_small_element unfitted");
        break;
      case "A": //absent
        //Cell.setAttribute("class", "hexagon_small_element absent");
        Cell.classList.add("absent");
        break;
      case "W": //warning
        Cell.classList.add("warning");
        //	Cell.setAttribute("class", "hexagon_small_element alarm");
        break;
      case "B": //bootloader
      case "C": //comm failure
      case "1": //alarm
      case "2": //alarm
      case "3": //alarm
      case "4": //alarm
        Cell.classList.add("alarm");
        //Cell.setAttribute("class", "hexagon_small_element alarm");

        /*if (Cell.classList.contains("alarm")) 
			{
				//Cell.classList.remove("alarm");
			} 
			else
			{
				Cell.classList.add("alarm");
			}
			
			if (Cell.classList.contains("absent")) 
			{
				Cell.classList.remove("absent");
			} 
			else
			{
				//Cell.classList.add("alarm");
			}*/
        break;
      case "0": //ok
        //Cell.setAttribute("class", "hexagon_small_element");
        /*if (Cell.classList.contains("alarm")) 
			{
				Cell.classList.remove("alarm");
			} 
			else
			{
				//Cell.classList.add("alarm");
			}
			if (Cell.classList.contains("absent")) 
			{
				Cell.classList.remove("absent");
			} 
			else
			{
				//Cell.classList.add("alarm");
			}*/
        break;
    }
  }
}

// Login
function checkLogin() {
  var loginBtn = $("login_button");
  if (loginBtn.innerHTML == "Login") {
    etlLogin("Please Login Here", doLogin);
  } else if (loginBtn.innerHTML == "Access Locked") {
    etlAlert(
      "The current IP address is locked. Please contact the administrators to unlock it.",
      "body"
    );
  } else {
    etlConfirm(
      'Logout from "' + $("login_button").innerHTML + '"?',
      doLogout,
      "body"
    );
  }
}

function seePermissions() {
  etlAlert(tooltip, "body");
}

function doLogin(user, pass) {
  if (!etlValidateLoginInputs(user, pass, "body")) {
    return;
  }

  var data, url;

  data =
    "{CPUXXXLI," + encodeURIComponent(user) + "," + window.btoa(pass) + ",1}";
  url = "command.xml?rcm=" + data;

  newAjaxRequest(url, "login", false, loginSuccess, loginFailure, null);
}

function doLogout(buttonReturned) {
  var data, url;

  if (buttonReturned == false) {
    return;
  }

  data = "{CPUXXXLO}";
  url = "command.xml?rcm=" + data;

  newAjaxRequest(url, "login", false, loginSuccess, loginFailure, null);
}

function loginSuccess(xd) {
  var temp = gXMLv(xd, "data");
  var tempSplit = temp.split(",");

  if (tempSplit[0] === "{CPUXXXLI") {
    if (tempSplit[2] == "O}") {
      setTimeout(function () {
        etlAlert("Login successful.", "body");
      }, 200);
      //etlAlert('Login successful.');
      // redirect to summary page
      pageSelected("summary.htm");
    } else {
      setTimeout(function () {
        etlAlert("Either the username or password is incorrect.", "body");
      }, 200);
      // redirect to summary page
      pageSelected("summary.htm");
      //etlAlert('Either the username or password is incorrect.');
    }
  } else if (tempSplit[0] === "{CPUXXXLO") {
    if (tempSplit[1] == "O}") {
      setTimeout(function () {
        etlAlert("Logout successful.", "body");
      }, 200);
      //etlAlert('Logout successful.');
      // redirect to summary page
      pageSelected("summary.htm");
    }
  } else {
    //etlAlert('Could not determine reply.');
  }
}

function loginFailure() {
  //etlAlert('Login failed, cannot contact unit.');
}

// kickoff status comms first..
setInterval(fecth_index, fetchInterval);
