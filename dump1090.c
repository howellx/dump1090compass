<!DOCTYPE html>

<html>
  <head>
    <meta name="viewport" content="initial-scale=1.0, user-scalable=no" />
     <link rel="stylesheet" href="https://unpkg.com/leaflet@1.6.0/dist/leaflet.css" integrity="sha512-xwE/Az9zrjBIphAcBb3F6JVqxf46+CDLwfLMHloNu6KEQCAWi6HcDUbeOfBIptF7tcCzusKFjFw2yuvEpDL9wQ==" crossorigin=""/>
    <style type="text/css">
      html { height: 100% }
      body { height: 100%; margin: 0; padding: 0 }
      .plane-icon {
        padding:0px;
        margin:0px;
      }
      #map_canvas { height: 100% }
      #info {
        position: absolute;
        width:20%;
        height:100%;
        bottom:0px;
        right:0px;
        top:0px;
        background-color: white;
        border-left:1px #666 solid;
        font-family:Helvetica;
      }
      #info div {
        padding:0px;
        padding-left:10px;
        margin:0px;
      }
      #info div h1 {
        margin-top:10px;
        font-size:16px;
      }
      #info div p {
        font-size:14px;
        color:#333;
      }
    </style>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>
    <script src="https://unpkg.com/leaflet@1.6.0/dist/leaflet.js" integrity="sha512-gZwIG9x3wUXg2hdXF6+rVkLF/0Vi9U8D2Ntg4Ga5I5BZpVkVxlJWbSQtXPSiUTtC0TjtGOmxa1AJPuV0CPthew==" crossorigin=""></script>
    <script type="text/javascript">
    Map=null;
    CenterLat=45.0;
    CenterLon=9.0;
    Planes={};
    NumPlanes = 0;
    Selected=null

    function getIconForPlane(plane) {
        var r = 255, g = 255, b = 0;
        var maxalt = 40000; /* Max altitude in the average case */
        var invalt = maxalt-plane.altitude;
        var selected = (Selected == plane.hex);

        if (invalt < 0) invalt = 0;
        b = parseInt(255/maxalt*invalt);

        /* As Icon we use the plane emoji, this is a simple solution but
           is definitely a compromise: we expect the icon to be rotated
           45 degrees facing north-east by default, this is true in most
           systems but not all. */
        var he = document.createElement("P");
        he.innerHTML = '>';
        var rotation = 45+360-plane.track;
        var selhtml = '';

        /* Give a border to the selected plane. */
        if (Selected == plane.hex) {
            selhtml = 'border:1px dotted #0000aa; border-radius:10px;';
        } else {
            selhtml = '';
        }
        he = '<div style="transform: rotate(-'+rotation+'deg); '+selhtml+'">✈️</div>';
        var icon = L.divIcon({html: he, className: 'plane-icon'});
        return icon;
    }

    function selectPlane(planehex) {
        if (!Planes[planehex]) return;
        var old = Selected;
        Selected = planehex;
        if (Planes[old]) {
            /* Remove the highlight in the previously selected plane. */
            Planes[old].marker.setIcon(getIconForPlane(Planes[old]));
        }
        Planes[Selected].marker.setIcon(getIconForPlane(Planes[Selected]));
        refreshSelectedInfo();
    }

    /* Return a closure to caputure the 'hex' argument. This way we don't
       have to care about how Leaflet passes the object to the callback. */
    function selectPlaneCallback(hex) {
        return function() {
            return selectPlane(hex);
        }
    }
    
    function refreshGeneralInfo() {
        var i = document.getElementById('geninfo');

        i.innerHTML = NumPlanes+' planes on screen.';
    }

    function refreshSelectedInfo() {
        var i = document.getElementById('selinfo');
        var p = Planes[Selected];

        if (!p) return;
        var html = 'ICAO: '+p.hex+'<br>';
        if (p.flight.length) {
            html += '<b>'+p.flight+'</b><br>';
        }
        html += 'Altitude: '+p.altitude+' feet<br>';
        html += 'Speed: '+p.speed+' knots<br>';
        html += 'Coordinates: '+p.lat+', '+p.lon+'<br>';
        i.innerHTML = html;
    }

    function fetchData() {
        $.getJSON('/data.json', function(data) {
            var stillhere = {}
            for (var j=0; j < data.length; j++) {
                var plane = data[j];
                var marker = null;
                stillhere[plane.hex] = true;
                plane.flight = $.trim(plane.flight);

                if (Planes[plane.hex]) {
                    var myplane = Planes[plane.hex];
                    marker = myplane.marker;
                    marker.setLatLng([plane.lat,plane.lon]);
                    marker.setIcon(getIconForPlane(plane));
                    myplane.altitude = plane.altitude;
                    myplane.speed = plane.speed;
                    myplane.lat = plane.lat;
                    myplane.lon = plane.lon;
                    myplane.track = plane.track;
                    myplane.flight = plane.flight;
                    if (myplane.hex == Selected)
                        refreshSelectedInfo();
                } else {
                    var icon = getIconForPlane(plane);
                    var marker = L.marker([plane.lat, plane.lon], {icon: icon}).addTo(Map);
                    var hex = plane.hex;
                    marker.on('click',selectPlaneCallback(plane.hex));
                    plane.marker = marker;
                    marker.planehex = plane.hex;
                    Planes[plane.hex] = plane;
                    icon.style.fontsize = (Map.getZoom()*3).toString() + "px";
                }

                // FIXME: Set the title
                // if (plane.flight.length == 0)
                //     marker.setTitle(plane.hex)
                // else
                //    marker.setTitle(plane.flight+' ('+plane.hex+')')
            }
            NumPlanes = data.length;

            /* Remove idle planes. */
            for (var p in Planes) {
                if (!stillhere[p]) {
                    Map.removeLayer(Planes[p].marker);
                    delete Planes[p];
                }
            }
        });
    }

  // Function to get style of select CSS class
  function getStyle(ruleClass) {
    for (var s = 0; s < document.styleSheets.length; s++) {
      var sheet = document.styleSheets[s];
      if (sheet.href == null) {
        var rules = sheet.cssRules ? sheet.cssRules : sheet.rules;
        if (rules == null) return null;
        for (var i = 0; i < rules.length; i++) {
          if (rules[i].selectorText == ruleClass) {
              return rules[i].style;
          }
        }
      }
    }
    return null;
  }

  function initialize() {
    Map = L.map('map_canvas').setView([42.0, -72.0], 8);

    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: 'Map data &copy; <a href="https://www.openstreetmap.org/">OpenStreetMap</a> contributors, <a href="https://creativecommons.org/licenses/by-sa/2.0/">CC-BY-SA</a>, Imagery © <a href="https://www.mapbox.com/">Mapbox</a>',
        maxZoom: 18,
        id: 'mapbox/streets-v11',
        accessToken: 'your.mapbox.access.token'
    }).addTo(Map);

          /* Setup our timer to poll from the server. */
          window.setInterval(function() {
              fetchData();
              refreshGeneralInfo();
          }, 100);
  }

    </script>
  </head>
  <body onload="initialize()">
    <button onclick="alert(Map.getZoom());">Get Zoom</button>
    <div id="map_canvas" style="width:80%; height:100%"></div>
    <div id="info">
      <div>
        <h1>Dump1090</h1>
        <p id="geninfo"></p>
        <p id="selinfo">Click on a plane for info.</p>
      </div>
    </div>
  </body>
</html>
