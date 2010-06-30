var geocoder;
var eventBuffer = new Array();

function wmwPostEventString(eventString){
    eventBuffer.push(eventString);
    window.status = '(event)';
}

function showText(){

  //      alert(window.status);

}

function wmwReadEventStrings(){

    var eventBufferString = eventBuffer.join('|');
    eventBuffer = new Array();
    //no more events
    window.status = '()';
    return eventBufferString;

}

var resultContent="";
var text = "";

function rezultat(id, results, status){

    

if (status == google.maps.GeocoderStatus.OK) {
            
            if (results[0]) {
                resultContent = id+"~";
                for ( var i = 0; i < results[0].address_components.length; ++i){

                    resultContent += results[0].address_components[i].types + ":" + results[0].address_components[i].long_name + "\n";

                }
            wmwPostEventString(resultContent);
            }else{
                alert("No result found");
            }

        } else {
            alert("Geocoder failed to: "+status);
        }
    

}



function reverseGeocoding(photoId, wanted_language, lat, lng){

    geocoder = new google.maps.Geocoder();

    if(wanted_language == ""){

        wanted_language = null;

    }



    var latlng = new google.maps.LatLng(lat, lng);    
    if(geocoder){
        
        geocoder.geocode({'latLng': latlng, language:wanted_language }, function(result, status){ 
                    rezultat(photoId, result, status); });

    }

} 
