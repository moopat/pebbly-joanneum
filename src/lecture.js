var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function getNextLecture(){
	// Course you want to query
	var course = "EHT";
	// Class year (not current year)
	var year = "2013";
	// This is a demo key that only works on the moop domain.
	var key = "zxj0JdZXiI";
	// Root of FHPI server
	var fhpi = "https://vhost.moop.at/fh2go/";
	
	var url = fhpi + "getNextLecture.php?c=" + course + "&y=" + year + "&k=" + key;
	
	// Send request to FHPI
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with lecture info
      var json = JSON.parse(responseText);

      var location = json.lecture.location;
			var time = json.lecture.time;
			var title = json.lecture.title;
      
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_LOCATION": location,
        "KEY_TIME": time, 
				"KEY_TITLE": title
      };
			
			console.log(JSON.stringify(dictionary));

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Lecture info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending lecture info to Pebble!");
        }
      );
    }      
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
		getNextLecture();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
		getNextLecture();
  }                     
);