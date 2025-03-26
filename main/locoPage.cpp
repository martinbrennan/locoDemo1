/**********************************************************
This file contains the entire Loco web page
it contains html, css and javascript
Its purpose is to show how to create a single page web app
that works with loco through endpoints defined in
the module web.c
***********************************************************/


const char* mainPage = R"***(
<!DOCTYPE html>
<head>
  <title> Brennan Loco </title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<html>
<!----------------------------CSS---------------------------->
<style>
:root {
 --bgColor: #002b36;
 --lightRow: #003948;
 --cardColor: #003f4f;
 --selectedColor: #001f28;
 --buttonColor: #334d55;
 --hoverColor: #5c4718;
}
body {
  font-family: sans-serif;
  color: #FFF;
  background-color: var(--bgColor);
}
.container {
  max-width: 500px;
  margin: 0 auto;
}
.blockStyle {
  display: block;
  width: 100%;
  margin: 10px auto;
  padding: 14px 28px;
  border: none;
  box-sizing: border-box; 
}
.blockButton {
  display: block;
  width: 100%;
  margin: 10px auto;
  padding: 14px 28px;
  border: none;
  color: white;
  background-color: var(--buttonColor);
  cursor: pointer;
  text-align: center;
}

.blockButton:hover {
  background-color: var(--hoverColor);
}
.buttonContainer {
  display: flex;
  justify-content: space-between;
  padding: 10px;
}
.equalButton {
  flex: 1;
  margin: 0 5px;
  padding: 10px;
  border: none;
  color: white;
  background-color: var(--buttonColor);
  cursor: pointer;
  text-align: center;
}
.equalButton:hover {
  background-color: var(--hoverColor);
}

.title {
  color: #FFF;
  font-size: 12px;
}
p {
  margin: 8px 0;
}
.card {
  margin: 20px;
  background-color: var(--cardColor);  
  padding: 8px 16px;
}
.triangle-up {
	width: 0;
	height: 0;
	border-left: 8px solid transparent;
	border-right: 8px solid transparent;
	border-bottom: 14px solid #CCC;
  cursor: pointer;  
}
.triangle-down {
	width: 0;
	height: 0;
	border-left: 8px solid transparent;
	border-right: 8px solid transparent;
	border-top: 14px solid #CCC;
  cursor: pointer;  
}

.mylink {
  cursor: pointer;
}  

.cPanel {
  max-height: 0;
  transition: max-height 400ms ease 0ms;
  overflow-y: hidden;
}

.cPanel.is_show {
  max-height: 300px;
  transition: max-height 400ms ease 0ms;  
}

.programming {
  background-color: DarkRed;  
}
.programming:hover {
  background-color: DarkRed !important;  
}

.settingBox {
  margin-top: 10px;
  margin-bottom: 10px;  
}
.desc {
  color: #CCC;
  font-size: 12px;
}

.searchResults {
	max-height: 200px;
	overflow-y: scroll;
}	

@keyframes flash {
 0% {opacity: 1;}
 50% {opacity: 0;}
 100% {opacity: 1;}
}   

.flash-slowly {
	animation: flash 1s linear infinite;
}

/* Dark overlay */
#overlay {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background-color: rgba(0, 0, 0, 0.7); /* Semi-transparent background */
    display: none; /* Initially hidden */
    z-index: 10; /* Behind the popup but above other content */
}

/* Popup window */
#popup {
    position: fixed;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    padding: 20px;
    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.5);
    display: none; /* Initially hidden */
    z-index: 20; /* Above the overlay */
    width: 400px;
}	

h2 {
	font-size: 14px;
	margin: 8px 0;
}

/*
tr {
	margin: 5px 0;
}
*/

table {
/*	border-spacing: 0 5px;	*/
	width: 100%;
}

.iconCol {
	width: 8%;
}	

#loading {
    position: fixed;
    top: 235px;
    left: 50%;
    display: none; /* Initially hidden */
    z-index: 20; /* Above the overlay */	
}

.pulse {
  width: 50px;
  height: 50px;
  background-color: #3498db;
  border-radius: 50%;
  animation: pulse 1.5s infinite;
}

@keyframes pulse {
  0% { transform: scale(1); }
  50% { transform: scale(1.5); opacity: 0.5; }
  100% { transform: scale(1); }
}

.spinner {
  border: 6px solid #808080; 
  border-top: 6px solid #404040;
  border-bottom: 6px solid #404040;
  border-radius: 50%;
  width: 30px;
  height: 30px;
  animation: spin 2s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

 /* Style the tab */
.tab {
  overflow: hidden;
  border: 1px solid #ccc;
  background-color: var(--cardColor);
  color: #FFF;
}

/* Style the buttons that are used to open the tab content */
.tab button {
  background-color: inherit;
  float: left;
  border: none;
  outline: none;
  cursor: pointer;
  padding: 14px 16px;
  transition: 0.3s;
  color: #fff;
}

/* Change background color of buttons on hover */
.tab button:hover {
  background-color: var(--selectedColor); 
}

/* Create an active/current tablink class */
.tab button.active {
  background-color: var(--selectedColor);  
}

/* Style the tab content */
.tabcontent {
  display: none;
  padding: 6px 12px;
  border: 1px solid #ccc;
  border-top: none;
  animation: fadeEffect 1s; /* Fading effect takes 1 second */
} 


/* Go from zero to full opacity */
@keyframes fadeEffect {
  from {opacity: 0;}
  to {opacity: 1;}
}

#art {
width: 80%;
height: auto;
display: block;
margin-left: auto;
margin-right: auto;
}

#station {
display: block;
text-align: center;
}

.colorContainer {
	display: flex;
	flex-direction: column;
	gap: 10px;
}

.colorItem {
	display: flex;
	align-items: centre;
	gap: 10px;
}

input[type='color'] {
	width: 40px;
	height: 40px;
	border: none;
	padding: 0;
	cursor: pointer;
}

#spotifyMetadata {
width: 80%;
display: block;
margin-left: auto;
margin-right: auto;	
}	
.muted {
color: #888;
font-size: 13.3px;
font-family: Tahoma, sans-serif;
}

.scroll {
overflow-y: scroll;
/* border: 1px solid #CCC; */
}


.myTable tr:nth-child(even){
  background-color: var(--bgColor);
}

.myTable tr:nth-child(odd){
  background-color: var(--lightRow);
}

.myTable td {
padding: 5px;
}

::webkit-scrollbar {
display: none;
}

* {
scrollbar-width: none;
-ms-overflow-style: none;
}


</style>
<body>
<div class="container">
 <!-- Tab links -->
<div class="tab">
  <button class="tablinks" onclick="openTab(0)">Radio</button>
  <button class="tablinks" onclick="openTab(1)">Settings</button>
  <button class="tablinks" onclick="openTab(2)">Now Playing</button>
  <button class="tablinks" onclick="openTab(3)">Spotify</button>
</div>

<!-- Tab content -->

<div id="radioTab" class="tabcontent">

  
<div class="card">
    <p onclick="toggleFavourites()">
    <span id="favourites" class="title" >Favourites</span>
    <span id="favouritesButton" class="triangle-down" style="float: right;"></span>
    </p>
    <div id="favouritesPanel" class="cPanel">

	<table id="favouritesTable">   
	</table>
    </div>

</div> 

<div class="card">
    <p onclick="toggleDirect()">
    <span id="direct" class="title" >Direct Radio URL entry</span>
    <span id="directButton" class="triangle-down" style="float: right;"></span>
    </p>
    <div id="directPanel" class="cPanel">

    <input class="blockStyle" type="text" name="directUrl" id="directUrl" placeholder="Station URL">    
    <input class="blockStyle" type="text" name="directName" id="directName" placeholder="Station Name">    
	<button class="blockButton" onclick="playDirect()">Go</button>  
    </div>

</div> 


<div class="card">
    <p onclick="toggleSearch()">
    <span id="search" class="title" >Search Airable</span>
    <span id="searchButton" class="triangle-down" style="float: right;"></span>
    </p>
    <div id="searchPanel" class="cPanel">

    <input class="blockStyle" type="text" name="vTunerSearch" id="vTunerSearch" onkeypress="searchKeypress(event)" placeholder="Search for a station">    

	<div class="searchResults">
	<table id="searchResults" >
	</table>
	</div>
    </div>

</div> 

<div class="card">
    <p onclick="toggleBrowse()">
    <span id="browse" class="title" >Browse Airable</span>
    <span id="browseButton" class="triangle-down" style="float: right;"></span>
    </p>
    <p id="breadcrumbs" class="muted"></p>
    <div id="browsePanel" class="cPanel">

	<div class="buttonContainer" >
		<button class="equalButton" onclick="vTunerGoBack ()">Back</button>
		<button class="equalButton" onclick="vTunerInit ()">Top</button>
	</div>

	<div class="searchResults">
	<table>
		<tbody id="vTunerTable"></tbody>
	</table>
	</div>
    </div>

</div> 

</div>

<div class="tabcontent">
<div class="card">
    <p>
    <span id="network" class="title" >Current Network</span>
    <span id="networkButton" class="triangle-down" onclick="toggleNetwork()" style="float: right;"></span>
    </p>
    <div id="networkPanel" class="cPanel">
    <button class="blockButton" onclick="wifiScan()">Wifi Scan</button> 
    <span style="display: none" id="wifiSpinner">Scanning...</span>
    <select class="blockStyle" id="ssids" name="ssids"></select>
    <input class="blockStyle" type="text" name="password" id="passid" placeholder="WifiPassword">
    <button class="blockButton" onclick="submitSsid ()">Send</button>
    </div>
</div> 
<div class="card">
    <p>
      <span id="version" class="title">Version:</span>
      <span id="versionButton" class="triangle-down" onclick="toggleVersion()" style="float: right;"></span>      
    </p>
    <div id="versionPanel" class="cPanel">
      <button id="updateButton" class="blockButton" onclick="update()">Update</button>
    </div>  
</div>
<div class="card buttonContainer" >
    <!--<button class="equalButton" onclick="command('login')">Login</button>-->
<!--    <button class="equalButton" onclick="recolor()">Colour</button>-->
    <button class="equalButton" onclick="command('reconnect')">Reconnect</button>
    <button class="equalButton" onclick="command('blob')">Blob</button>
    <button class="equalButton" onclick="command('disconnect')">Disconnect</button>                   
<!--    <button class="equalButton" onclick="command('state')">State</button>-->
</div>

<div class="card">
 <div class="colorContainer">
  <div class="colorItem">
   <input type="color" id="bgPicker"/>  
   <label>Background Colour</label>
  </div> 
  
  <div class="colorItem">
   <input type="color" id="cardPicker"/>  
   <label>Card Background Colour</label>
  </div> 
  
  <div class="colorItem">
   <input type="color" id="selPicker"/>  
   <label>Selected Colour</label>
  </div> 

  <div class="colorItem">
   <input type="color" id="buttonPicker"/>  
   <label>Button Colour</label>
  </div> 

 </div>
</div>	
		
</div>

<div id="nowPlaying" class="tabcontent">
						<img id="art" src ="">
<div id="spotifyMetadata">						
      <p class="muted">Track:</p>
      <p id="track">Track:</p>
      <p class="muted">Album:</p>
      <p id="album">Album:</p>
      <p class="muted">Artist:</p>      
      <p id="artist">Artist:</p>   
</div>      
<div id="radioMetadata">						
      <p id="station">Station</p>    
</div>  
<div class="card buttonContainer" >
<!--    <button class="equalButton" onclick="command('stop')">Stop</button>-->
    <button class="equalButton" onclick="command('prev')">Prev</button>  
    <button id="playStop" class="equalButton" onclick="command('pause')">Pause</button>
    <button class="equalButton" onclick="command('next')">Next</button>                  
<!--    <button class="equalButton" onclick="command('shuffle')">Shuffle</button> -->
</div> 
</div> 


<div id="spotifyTab" class="tabcontent">

  
<div class="card">
    <p onclick="togglePlaylists()">
    <span id="playlists" class="title" >Playlists</span>
    <span id="playlistsButton" class="triangle-down" style="float: right;"></span>
    </p>
    <div id="playlistsPanel" class="cPanel scroll">

	<table id="playlistsTable" class="myTable">   
	</table>
    </div>
</div> 

</div>


    <div style="display: none;" class="title" id="myDIV"> </div>

    <!-- Dark overlay -->
    <div id="overlay"></div>

    <!-- Popup window -->
    <div id="popup" class="card">
        <h2 id="popupTitle">Add Station to Preset</h2>
        <table><tbody id="presetsList">
        </tbody></table>
	<div class="buttonContainer" >
        <button onclick="newPreset ()" class="equalButton">New Preset</button>
        <button id="closePopup" onclick="closePopup()" class="equalButton">Close</button>
    </div>    
    </div>

    <!-- Loading Gif -->
    <div id="loading" class="spinner"></div>


</div>
  <script>

  var cw = 420;
  var ch = 100;
  var stations;
  var vTunerLinks;

    //-------------------------------------------------------

    
    function command(cmd)
    {
      var req = new XMLHttpRequest();
      req.onreadystatechange = function()
      {
        if(this.readyState == 4 && this.status == 200)
        {
          var x = document.getElementById("myDIV");
          x.innerHTML =
          this.responseText;
        }
      };
      req.open("GET", cmd, true);
      req.send();
    } 

function createElementFromHTML (string){
  var div = document.createElement('div');
  div.innerHTML = string;
  return div.firstChild;
}

    function getSsids()
    {
      var req = new XMLHttpRequest();
      req.onreadystatechange = function()
      {
        if(this.readyState == 4 && this.status == 200)
        {
//          console.log (this.responseText);
          ssids = JSON.parse (this.responseText).names;
          console.log ("Got "+ssids.length+" ssids");
          var sel = document.getElementById("ssids");
          removeChildren (sel);
          for (n=0;n<ssids.length;n++){
            var option = createElementFromHTML ('<option value="'+ssids[n]+'">'+ssids[n]+'</option>');
            sel.appendChild (option);
          };  
        };
      };
      req.open("GET","ssids", true);
      req.send();
    } 

var wifiTimer;

function wifiUpdate (){
    var s = document.getElementById("wifiSpinner");
    if (wifiTimer){
      s.innerHTML = "Scanning..."+wifiTimer;
      wifiTimer--;
      setTimeout (wifiUpdate,1000);
    }  
    else {
//      getSsids();
      document.getElementById("wifiSpinner").style.display = "none";       
    }
}

function wifiScan (){
  command('scanWifi');
  document.getElementById("wifiSpinner").style.display = "inline";
  wifiTimer = 6;
  wifiUpdate ();
}


    function getZones()
    {
      var req = new XMLHttpRequest();
      req.onreadystatechange = function()
      {
        if(this.readyState == 4 && this.status == 200)
        {
          console.log (this.responseText);
          
          zones = JSON.parse (this.responseText).names;
          console.log ("Got "+zones.length+" zones");
          var sel = document.getElementById("zones");
          removeChildren (sel);
          for (n=0;n<zones.length;n++){
            var option = createElementFromHTML ('<option value="'+zones[n]+'">'+zones[n]+'</option>');
            sel.appendChild (option);
          };
            
        };
      };
      req.open("GET","zones", true);
      req.send();
    } 

function removeChildren (node){
  while (node.firstChild) node.removeChild (node.lastChild);
}


function submitSsid (){
  console.log ("submitSsid()");

  var password = document.getElementById ("passid").value;
  var ssid = document.getElementById ("ssids").value;
  var req = new XMLHttpRequest();
  var params = "ssid="+ssid+"&password="+password;
  req.open("POST", "pickSSID", true);
  req.onreadystatechange = function(){
    if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
    };
  }; 
  req.send(params);        
}

function submitZone (){
  console.log ("submitZone()");

  var zone = document.getElementById ("zones").value;
  var req = new XMLHttpRequest();
  req.open("GET", "pickZone?zone="+zone, true);
  req.onreadystatechange = function(){
    if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
    };
  }; 
  req.send();        
}

var lastStatusText;
var currentStatus;

function doStatus (){
  var req = new XMLHttpRequest();
  req.onreadystatechange = function(){
    var n;
    if(this.readyState == 4 && this.status == 200)
    {
      var statusText = this.responseText;
      if (statusText != lastStatusText){
        console.log ("Status has changed "+statusText);
        lastStatusText = statusText;        
        var s = JSON.parse (statusText);
        currentStatus = s;

        if ((ssids = s.ssids)){
          console.log ("Got "+ssids.length+" ssids");
          var sel = document.getElementById("ssids");
          removeChildren (sel);
          for (n=0;n<ssids.length;n++){
            var option = createElementFromHTML ('<option value="'+ssids[n]+'">'+ssids[n]+'</option>');
            sel.appendChild (option);
          }; 
        }

        if ((zones = s.zones)){  
          console.log ("Got "+zones.length+" zones");
          var sel = document.getElementById("zones");
          removeChildren (sel);
          for (n=0;n<zones.length;n++){
            var option = createElementFromHTML ('<option value="'+zones[n]+'">'+zones[n]+'</option>');
            sel.appendChild (option);
          };
        }

        if (s.ssid){
          var sel = document.getElementById("network");
          
          var ip = s.ip?" "+s.ip:"";

          sel.innerHTML = "Current Network : "+s.ssid+ip;          
        }

        if (s.zone){
          var sel = document.getElementById("zone");
          sel.innerHTML = "Current Zone : "+s.zone;

          document.getElementById ("zones").value = s.zone;
        }

        if (s.track){
          var sel = document.getElementById("track");
          sel.innerHTML = s.track;
        }
        
        if (s.album){
          var sel = document.getElementById("album");
          sel.innerHTML = s.album;
        }
        if (s.artist){
          var sel = document.getElementById("artist");
          sel.innerHTML = s.artist;
        }
        if (s.station){
          var sel = document.getElementById("station");
          sel.innerHTML = s.station;
        }
        if (s.art){
          var image = document.getElementById("art");
          image.src = s.art;
        }

        document.getElementById("version").innerHTML = "Version: "+s.version;
                
        if (s.update){
			var sel = document.getElementById("updateButton");
			sel.innerHTML = s.update;
			sel.classList.add("programming"); 
		}
		else {
			var sel = document.getElementById("updateButton");
			sel.innerHTML = "Update";
			sel.classList.remove("programming");			
		} 
		
		if (s.presets){
			buildPresetsTable ();						
		}  
		if (!s.searchResults){
			var sel = document.getElementById("searchResults");
			removeChildren (sel);
		}
		if (s.favourites) buildFavourites (s.favourites);
		if (s.playlists) buildPlaylists (s.playlists);
		
		if (s.source == "Radio"){
			document.getElementById("spotifyMetadata").style.display = "none";  
			document.getElementById("radioMetadata").style.display = "block";  
		}
		else if (s.track) {
			document.getElementById("spotifyMetadata").style.display = "block";  
			document.getElementById("radioMetadata").style.display = "none"; 
		}			
		else {
			document.getElementById("spotifyMetadata").style.display = "none";  
			document.getElementById("radioMetadata").style.display = "none"; 		
		}
        var playStop = document.getElementById("playStop");
		if (s.playing == "true") playStop.innerHTML = "Pause";		
		else playStop.innerHTML = "Play";
		
		if (s.breadcrumbs){
          var sel = document.getElementById("breadcrumbs");
          sel.innerHTML = s.breadcrumbs;
        }
			
					                     
      }
    }
  };
  req.open("GET", "getStatus", true);
  req.send(); 
}

var genericSettings;

function getGenericSettings (){
  var req = new XMLHttpRequest();
  req.onreadystatechange = function(){
    if(this.readyState == 4 && this.status == 200){
      genericSettings = JSON.parse (this.responseText);

      var sel = document.getElementById("settingsPanel");
      removeChildren (sel);
      var settings = Object.keys(genericSettings);
      settings.forEach ((setting) => {
        console.log (genericSettings[setting].label + " " + genericSettings[setting].value);
        var s = genericSettings[setting];

        var it = '<div class="settingBox"><input type="number" name="' + setting + '" id="' + setting + 
                  '" min="'+s.min+'" max="'+s.max+'" step="'+s.step+'" value="'+s.value+'" onchange="setGenericSettings()">' +
                  '<span class="title"> '+s.label+'</span></div>';
        var inp = createElementFromHTML (it);          
        sel.appendChild (inp);



      }); 
    }      
  };
  req.open("GET", "getGenericSettings", true);
  req.send(); 
}


document.addEventListener("DOMContentLoaded",function(event){

//  getGenericSettings ();
  setInterval (doStatus,1000);
  openTab (0);
  
  var root = document.documentElement;
  var cp1 = document.getElementById ("bgPicker");
  cp1.value = getComputedStyle(root).getPropertyValue ('--bgColor').trim();
  cp1.addEventListener ('input',function () {
  	root.style.setProperty('--bgColor',cp1.value);
  });	

  var cp2 = document.getElementById ("cardPicker");
  cp2.value = getComputedStyle(root).getPropertyValue ('--cardColor').trim();
  cp2.addEventListener ('input',function () {
  	root.style.setProperty('--cardColor',cp2.value);
  });	

  var cp3 = document.getElementById ("selPicker");
  cp3.value = getComputedStyle(root).getPropertyValue ('--selectedColor').trim();
  cp3.addEventListener ('input',function () {
  	root.style.setProperty('--selectedColor',cp3.value);
  });

  var cp4 = document.getElementById ("buttonPicker");
  cp4.value = getComputedStyle(root).getPropertyValue ('--buttonColor').trim();
  cp4.addEventListener ('input',function () { 
  	root.style.setProperty('--buttonColor',cp4.value);
  });
   
});

function pickZone (){
  console.log ("pickZone");

  var zone = document.getElementById ("zones").value;
  var req = new XMLHttpRequest();
  req.open("GET", "pickZone?zone="+zone, true);
  req.onreadystatechange = function(){
    if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
    };
  }; 
  req.send();        
}

function togglePanel (panel,button){
 panel.classList.toggle("is_show");

 if (panel.classList.contains("is_show")){
   button.classList.remove("triangle-down");
   button.classList.add("triangle-up");
   return 1;
 }
 else {
  button.classList.remove("triangle-up");
  button.classList.add("triangle-down");
  return 0;
 } 
}

function toggleVersion (){

 var panel = document.getElementById ("versionPanel");
 var button = document.getElementById ("versionButton");
 togglePanel (panel,button);
}

function toggleNetwork (){

 var panel = document.getElementById ("networkPanel");
 var button = document.getElementById ("networkButton");
 togglePanel (panel,button);

}

function toggleZone (){

 var panel = document.getElementById ("zonePanel");
 var button = document.getElementById ("zoneButton");
 togglePanel (panel,button);

}

function toggleSettings (){

 var panel = document.getElementById ("settingsPanel");
 var button = document.getElementById ("settingsButton");
 togglePanel (panel,button);
}

function toggleFavourites (){

 var panel = document.getElementById ("favouritesPanel");
 var button = document.getElementById ("favouritesButton");
 togglePanel (panel,button);
}

function togglePlaylists (){

 var panel = document.getElementById ("playlistsPanel");
 var button = document.getElementById ("playlistsButton");
 var r = togglePanel (panel,button);
 if (r) command('initPlaylists')
}


function playPlaylist (index){
 console.log ("playPlaylist ("+index+")");
  
 var req = new XMLHttpRequest();
 req.onreadystatechange = function() {
   if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
   };  
 };
 req.open("GET","playPlaylist?index="+index, true);
 req.send(); 
} 


function toggleDirect (){

 var panel = document.getElementById ("directPanel");
 var button = document.getElementById ("directButton");
 togglePanel (panel,button);
}

function toggleSearch (){

 var panel = document.getElementById ("searchPanel");
 var button = document.getElementById ("searchButton");
 togglePanel (panel,button);
}

function toggleBrowse (){

 var panel = document.getElementById ("browsePanel");
 var button = document.getElementById ("browseButton");
 var r = togglePanel (panel,button);
 if (r) vTunerInit ();
}

function setGenericSettings(){

  console.log ("setGenericSettings()");

  settings = Object.keys(genericSettings);
  settings.forEach ((setting) => {
    var v = parseInt (document.getElementById (setting).value);
    console.log (setting + " " + v); 
    genericSettings[setting].value = v;   
  });

  var j = JSON.stringify (genericSettings);
  console.log (j);

  var req = new XMLHttpRequest();
  req.open("POST", "setGenericSettings", true);
  req.onreadystatechange = function(){
    if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
    };
  }; 
  req.send(j);   

}

// TODO temporary way to set presets
// integrate with vTuner perhaps

function setPresets(){

  console.log ("setPresets()");

	var presets = currentStatus.presets;

  var j = JSON.stringify (presets);
  console.log (j);

  var req = new XMLHttpRequest();
  req.open("POST", "setPresets", true);
  req.onreadystatechange = function(){
    if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
    };
  }; 
  req.send(j);   

}

function playDirect(){

  console.log ("playDirect()");

  var station = {};
  station.url = document.getElementById("directUrl").value;
  station.name = document.getElementById("directName").value;

  var j = JSON.stringify (station);
  console.log (j);

  var req = new XMLHttpRequest();
  req.open("POST", "playDirect", true);
  req.onreadystatechange = function(){
    if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
    };
  }; 
  req.send(j);   

}


  function keypress (e){
    if ((e.which == 10)||(e.which==13)) setPresets ();
  } 



function playResult (result){
 console.log ("playResult ("+result+")");
  
 var req = new XMLHttpRequest();
 req.onreadystatechange = function() {
   if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
   };  
 };
 req.open("GET","playResult?station="+result, true);
 req.send(); 
} 

function playFavourite (result){
 console.log ("playFavourite ("+result+")");
  
 var req = new XMLHttpRequest();
 req.onreadystatechange = function() {
   if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
   };  
 };
 req.open("GET","playFavourite?station="+result, true);
 req.send(); 
} 


function playPreset (index){
 console.log ("playPreset ("+index+")");
  
 var req = new XMLHttpRequest();
 req.onreadystatechange = function() {
   if(this.readyState == 4 && this.status == 200){
      console.log (this.responseText);
   };  
 };
 req.open("GET","playPreset?station="+index, true);
 req.send(); 
} 

/*
function buildSearchResults (text){

	stations = JSON.parse (text)
    console.log ("Got "+stations.length+" stations");
    var sel = document.getElementById("searchResults");
    removeChildren (sel);
    for (n=0;n<stations.length;n++){
        var row = document.createElement ('tr');
        row.innerHTML = '<td>'+stations[n].name+'</br><span class="desc">'+stations[n].desc+'</span></td>'+
			'<td><button onclick="setPreset (1,'+n+')">1</button></td>'+
			'<td><button onclick="setPreset (2,'+n+')">2</button></td>'+
			'<td><button onclick="setPreset (3,'+n+')">3</button></td>'+
			'<td><button onclick="playResult ('+n+')">Play</button></td>';						
        sel.appendChild (row);
    }
}
*/

function buildSearchResults (text) {

	var icon;
	stations = JSON.parse (text);
    console.log ("Got "+stations.length+" stations");
    var sel = document.getElementById("searchResults");
    removeChildren (sel);
    for (n=0;n<stations.length;n++){
        var row = document.createElement ('tr');
		icon = '<td class="iconCol mylink" onclick="playResult(' + n +')"> &#9654 </td>';	    
		row.innerHTML = icon+'<td class="mylink" onclick="searchItemClick ('+n+')">'+stations[n].name+'</br><span class="desc">'+stations[n].desc+'</span></td>';
		sel.appendChild(row);
	}
}

function buildFavourites (stations) {

	var icon;
    console.log ("buildFavourites Got "+stations.length+" stations");
    var sel = document.getElementById("favouritesTable");
    removeChildren (sel);
    for (n=0;n<stations.length;n++){
        var row = document.createElement ('tr');
		icon = '<td class="iconCol mylink" onclick="playFavourite(' + n +')"> &#9654 </td>';	    
		row.innerHTML = icon+'<td class="mylink" onclick="playFavourite ('+n+')">'+stations[n].name+'</td>';
		sel.appendChild(row);
	}
}

function buildPlaylists (playlists) {

    console.log ("buildPlaylists Got "+playlists.length+" playlists");
    var sel = document.getElementById("playlistsTable");
    removeChildren (sel);
    for (n=0;n<playlists.length;n++){
        var row = document.createElement ('tr');	    
		row.innerHTML = '<td class="mylink" onclick="playPlaylist ('+n+')">'+playlists[n]+'</td>';
		sel.appendChild(row);
	}
}

  function searchKeypress (e){
    if ((e.which == 10)||(e.which==13)){

		console.log ("searchKeypress()");


		var i = document.getElementById("vTunerSearch");	
		var searchString = i.value;	

//		i.classList.add("flash-slowly");
		spinnerOn ();

		var j = JSON.stringify (searchString);
		console.log (j);

    var sel = document.getElementById("searchResults");
    removeChildren (sel);

		var req = new XMLHttpRequest();
		req.open("POST", "vTunerSearch", true);
		req.onreadystatechange = function(){
			if(this.readyState == 4 && this.status == 200){
			console.log (this.responseText);
			buildSearchResults (this.responseText);
//			i.classList.remove("flash-slowly");
			spinnerOff ();
			};
		}; 
		req.send(j);   

	}
  } 


    function fetchB()
    {
      var sel = document.getElementById("updateButton");
      sel.innerHTML = "Loading";
      sel.classList.add("programming");      
      let s = document.createElement("script");
      s.src = "http://brennan.co.uk/imagprod/mouse.jsonp";
      document.body.appendChild(s);

    } 
    function myFunc (data){
      var sel = document.getElementById("updateButton");
      sel.innerHTML = "Programming - please wait";      
      console.log ("Data length = "+data.length);
      let b = atob (data);
      console.log ("Blob length = "+b.length);
      var req = new XMLHttpRequest();
      req.open("POST", "update", true);
      req.onreadystatechange = function(){
        if(this.readyState == 4 && this.status == 200){
        console.log (this.responseText);
        var sel = document.getElementById("updateButton");
        sel.innerHTML = "Restarting - refresh browser";
        sel.classList.remove("programming");                  
        };
      }; 
      req.send(data);              
    }

function update (){
	if (!currentStatus.update) command('update');
}

var selectedName;
var selectedUrl;
var selectedLogo;


function searchItemClick(index) {

  console.log("searchItemClick (" + index + ")");

  selectedName = stations[index].name;
  selectedUrl = stations[index].url;
  selectedLogo = "";
  openPopup();
}

function vTunerItemClick (index){

  console.log("vTunerItemClick ("+index+")");


 if (vTunerLinks[index].type == "Station"){
		selectedName = vTunerLinks[index].name;
		selectedUrl = vTunerLinks[index].url;
		selectedLogo = vTunerLinks[index].logo;
		openPopup ();
		return;
 }	

  spinnerOn ();

  var req = new XMLHttpRequest();
  req.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {

      spinnerOff ();

      var data = JSON.parse(this.responseText);
      console.log("vTunerTop result");
      console.dir(data);
      
      updateLinks(data);

    };
  };
  req.open("GET", "vTunerItem?index="+index, true);
  req.send();

}

function vTunerItemPlay (index){

  console.log("vTunerItemPlay ("+index+")");

  var req = new XMLHttpRequest();
  req.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {

      var data = JSON.parse(this.responseText);
      console.log("vTunerTop result");
      console.dir(data);
      
      updateLinks(data);

    };
  };
  req.open("GET", "vTunerItem?index="+index, true);
  req.send();

}


function vTunerGoBack (){

  console.log("vTunerGoBack ()");

  spinnerOn ();
  
  var req = new XMLHttpRequest();
  req.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {

	spinnerOff ();

      var data = JSON.parse(this.responseText);
      console.log("vTunerTop result");
      console.dir(data);      
      updateLinks(data);
    };
  };
  req.open("GET", "vTunerBack", true);
  req.send();

}



function updateLinks(data) {

		console.log ("updateLinks");
  vTunerLinks = data;
  var table = document.getElementById("vTunerTable");
  removeChildren(table);
  var icon;

  for (var i = 0; i < data.length; i++) {

    var row = document.createElement('tr');
    
    
    if (data[i].type == "Station")
		icon = '<td class="iconCol mylink" onclick="vTunerItemPlay(' + i +')"> &#9654 </td>';
	else
		icon = '<td class="iconCol"></td>';		
    
    row.innerHTML = '<tr>'+icon+
					'<td onclick="vTunerItemClick(' + i +')" class="mylink">' + data[i].name +
                    '</td>'+
                    '</tr>';
                    
    table.appendChild(row);
  }
}


// WARNING dont do vTunerTop if window is reopened
// just on first open

function vTunerInit() {

  console.log("vTunerInit ()");

  spinnerOn ();
  var req = new XMLHttpRequest();
  req.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {

	  spinnerOff ();

      var data = JSON.parse(this.responseText);
      console.log("vTunerTop result");
      console.dir(data);
      updateLinks(data);
    };
  };
  req.open("GET", "vTunerTop", true);
  req.send();
}

const popup = document.getElementById('popup');
const overlay = document.getElementById('overlay');
const loading = document.getElementById('loading');

// Function to open the popup
function openPopup() {
    popup.style.display = 'block';
    overlay.style.display = 'block';
    
    var title = document.getElementById("popupTitle"); 
    title.textContent = "Add "+selectedName+" to preset";   
    
//    console.log ("openPopup");
//    console.dir (currentStatus);
    
    var presets = currentStatus.presets;
    var table = document.getElementById("presetsList");
    removeChildren (table);
    if (!presets) return;
    for (n=0;n<presets.length;n++){
        var row = document.createElement ('tr');
        var rc = n+1;
//        row.innerHTML = '<td>'+rc+' '+presets[n].name+'</td><td class="mylink" onclick="setPresetList('+n+')">+</td>';						
        row.innerHTML = '<td class="mylink" onclick="setPresetList('+n+')">'+rc+' '+presets[n].name+'</td>';
        table.appendChild (row);    
    }    
}

// Function to close the popup
function closePopup() {
    popup.style.display = 'none';
    overlay.style.display = 'none';
}

function setPresetList (index){
	console.log ("setPresetList "+index);
	
	console.log ("Name "+selectedName);
	console.log ("Url "+selectedUrl);
	console.log ("Logo "+selectedLogo);	
	 
 var presets = currentStatus.presets;
 presets[index].name = selectedName;
 presets[index].url = selectedUrl; 
 presets[index].logo = selectedLogo;
 
 setPresets ();	
} 

function newPreset (){
	console.log ("newPreset ()");
	
	console.log ("Name "+selectedName);
	console.log ("Url "+selectedUrl);
	console.log ("Logo "+selectedLogo);	
	 
 var presets = currentStatus.presets;
 
 let preset = {
	"name": selectedName,
	"url": selectedUrl,
	"logo": selectedLogo
 };
 presets.push (preset);	
  
 setPresets ();
 closePopup ();	
} 

function deletePreset (index){

 var presets = currentStatus.presets;
 presets.splice (index,1);
 setPresets ();
 
} 

function spinnerOn (){
    loading.style.display = 'block';
//    overlay.style.display = 'block';
}

function spinnerOff (){
    loading.style.display = 'none';
//    overlay.style.display = 'none';
}

function recolor (){
	document.documentElement.style.setProperty ('--bgColor','#000');
}


function openTab(index) {

  var i, tabcontent, tablinks;

  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {
    if (i == index)
    	tabcontent[i].style.display = "block";
    else
    	tabcontent[i].style.display = "none";    	
  }

  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
    	tablinks[i].className = tablinks[i].className.replace(" active", "");
				if (i == index)
  				tablinks[i].className += " active";
  }
} 
    
    //-------------------------------------------------------

  </script>
</body>
</html>
)***";




