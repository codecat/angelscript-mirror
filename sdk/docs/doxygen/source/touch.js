function isTouchDevice(){
    try{
        document.createEvent("TouchEvent");
        return true;
    }catch(e){
        return false;
    }
}

function touchScroll(id){
    if(isTouchDevice()){
        var el=document.getElementById(id);
        var scrollStartPos=0;
        var lastPos=0;
        var delta=0;
        var capture=false;

        el.addEventListener("touchstart", function(event) {
            scrollStartPos=this.scrollTop+event.touches[0].pageY;
            lastPos = event.touches[0].pageY;
            if (capture) {
                event.preventDefault();
                capture = false;
            }
        },false);

        el.addEventListener("touchmove", function(event) {
            var deltaY = scrollStartPos-event.touches[0].pageY;
            delta = event.touches[0].pageY - lastPos;
            lastPos = event.touches[0].pageY;
            capture = !(delta <= 0 && this.scrollTop+this.clientHeight==this.scrollHeight) && !(delta >= 0 && this.scrollTop == 0);
            if (capture) {
               this.scrollTop = deltaY;
               event.preventDefault();
            }
        },false);
    }
}

$(document).ready(function() {
    $('.contents').each(function(){ touchScroll(this.id); })
});

