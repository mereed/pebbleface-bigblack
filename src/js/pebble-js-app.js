var mConfig = {};

Pebble.addEventListener("ready", function(e) {

	loadLocalData();
  returnConfigToPebble();
});

Pebble.addEventListener("showConfiguration", function(e) {
	Pebble.openURL(mConfig.configureUrl);
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      returnConfigToPebble();
    }
  }
);

function saveLocalData(config) {

  //console.log("loadLocalData() " + JSON.stringify(config));

  localStorage.setItem("blink", parseInt(config.blink));  
  localStorage.setItem("invert", parseInt(config.invert)); 
  localStorage.setItem("bluetoothvibe", parseInt(config.bluetoothvibe)); 
  localStorage.setItem("hourlyvibe", parseInt(config.hourlyvibe)); 
  localStorage.setItem("germanlang", parseInt(config.germanlang)); 
  localStorage.setItem("russianlang", parseInt(config.russianlang)); 
  
  loadLocalData();

}
function loadLocalData() {
  
	mConfig.blink = parseInt(localStorage.getItem("blink"));
	mConfig.invert = parseInt(localStorage.getItem("invert"));
	mConfig.bluetoothvibe = parseInt(localStorage.getItem("bluetoothvibe"));
	mConfig.hourlyvibe = parseInt(localStorage.getItem("hourlyvibe"));
	mConfig.germanlang = parseInt(localStorage.getItem("germanlang"));
	mConfig.russianlang = parseInt(localStorage.getItem("russianlang"));
	mConfig.configureUrl = "http://www.themapman.com/pebblewatch/pebbleconfig3.html";

	if(isNaN(mConfig.blink)) {
		mConfig.blink = 1;
	}
	if(isNaN(mConfig.invert)) {
		mConfig.invert = 0;
	}
	if(isNaN(mConfig.bluetoothvibe)) {
		mConfig.bluetoothvibe = 0;
	}
	if(isNaN(mConfig.hourlyvibe)) {
		mConfig.hourlyvibe = 0;
	}
    if(isNaN(mConfig.germanlang)) {
		mConfig.germanlang = 0;
	}
	if(isNaN(mConfig.russianlang)) {
		mConfig.russianlang = 0;
	}

  //console.log("loadLocalData() " + JSON.stringify(mConfig));
}
function returnConfigToPebble() {
  //console.log("Configuration window returned: " + JSON.stringify(mConfig));
  Pebble.sendAppMessage({
    "blink":parseInt(mConfig.blink), 
    "invert":parseInt(mConfig.invert), 
    "bluetoothvibe":parseInt(mConfig.bluetoothvibe), 
    "hourlyvibe":parseInt(mConfig.hourlyvibe),
    "germanlang":parseInt(mConfig.germanlang),
    "russianlang":parseInt(mConfig.russianlang),
  });    
}