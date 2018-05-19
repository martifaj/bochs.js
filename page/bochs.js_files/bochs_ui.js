// bochs_ui.js

var canvas; 
var ctx;
var palette;
var bpp;
var resx, resy;
var system_started = 0;
var buttons_state = 0;

window.addEventListener('load', function() {

  canvas = document.getElementById("vgadisp");
  xres = canvas.width;
  yres = canvas.height;

  canvas.requestPointerLock = canvas.requestPointerLock ||
             canvas.mozRequestPointerLock ||
             canvas.webkitRequestPointerLock;
  
  document.exitPointerLock = document.exitPointerLock ||
           document.mozExitPointerLock ||
           document.webkitExitPointerLock;

  canvas.onclick = function() {
      canvas.requestPointerLock();
  }

  document.addEventListener('pointerlockchange', lockChangeAlert, false);
  document.addEventListener('mozpointerlockchange', lockChangeAlert, false);
  document.addEventListener('webkitpointerlockchange', lockChangeAlert, false);


  ctx = canvas.getContext("2d");
  palette = new Array(256);
  for (var x=0; x<palette.length; x++) {
      if (palette[x] == null) {
          palette[x] = {};
      }
      palette[x].red = 0;
      palette[x].green = 0;
      palette[x].blue = 0;
  }

  loadingCanvas();
}, false);

var worker = new Worker('bochs_worker.js');
worker.postMessage('');
worker.addEventListener('message', function(e) {
  var msg = e.data;
  if (!system_started) {
    system_started = 1;
    clearScreen();
  }
  switch (msg.cmd) {
      case "serial":
        break;
      case "dimensionupdate":
        var curr_data = msg.data;
        xres = curr_data.x;
        yres = curr_data.y;
        bpp  = curr_data.bpp;
        canvas.width  = xres; // in pixels
        canvas.height = yres; // in pixels
        clearScreen();
        break;
      case "clearscreen":
        clearScreen();
        break;
      case "setpalette":
        var curr_data = msg.data;
        if (palette[msg.index] == null) {
            palette[msg.index] = {};
        }
        palette[msg.index].red   = curr_data.red;
        palette[msg.index].green = curr_data.green;
        palette[msg.index].blue  = curr_data.blue;
        break;
      case "tileupdate":
        var curr_data = msg.data;
        var curr_tile = new Uint8ClampedArray(curr_data.tile);
        var imageData = ctx.createImageData(curr_data.x_tilesize, curr_data.y_tilesize);
        for (var x=0; x<curr_data.x_tilesize; x++) {
            for (var y=0; y<curr_data.y_tilesize; y++) {
                 var pixelIndexTile = (x + y * curr_data.x_tilesize) * (curr_data.bpp >> 3);
                 var pixelIndexCanvas = (x + y * curr_data.x_tilesize) * 4;
                 switch (curr_data.bpp) {
                     case 8:
                        imageData.data[pixelIndexCanvas    ] = palette[curr_tile[pixelIndexTile]].red;  // red   color
                        imageData.data[pixelIndexCanvas + 1] = palette[curr_tile[pixelIndexTile]].green;  // green color
                        imageData.data[pixelIndexCanvas + 2] = palette[curr_tile[pixelIndexTile]].blue;  // blue  color
                        imageData.data[pixelIndexCanvas + 3] = 255;  // alpha
                        break;
                     case 24:
                     case 32:
                        imageData.data[pixelIndexCanvas    ] = curr_tile[pixelIndexTile];  // red   color
                        imageData.data[pixelIndexCanvas + 1] = curr_tile[pixelIndexTile + 1];  // green color
                        imageData.data[pixelIndexCanvas + 2] = curr_tile[pixelIndexTile + 2];  // blue  color
                        break;
                 }
                 if (curr_data.bpp == 8 || curr_data.bpp == 24) {
                    imageData.data[pixelIndexCanvas + 3] = 255;  // alpha
                 } else if (curr_data.bpp == 32) {
                    imageData.data[pixelIndexCanvas + 3] = curr_tile[pixelIndexTile + 3];  // alpha
                 }
            }
        }
        ctx.putImageData(imageData, curr_data.x, curr_data.y);
        break;
      case "clearscreen":
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        break;
  }
}, false);

function clearScreen() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
}

function lockChangeAlert() {
  $(document).unbind("mousedown", mousedown);
  $(document).unbind("mouseup", mouseup);
  $(document).unbind("keydown", keydown);
  $(document).unbind("keyup", keyup);
  if(document.pointerLockElement === canvas ||
  document.mozPointerLockElement === canvas ||
  document.webkitPointerLockElement === canvas) {
    document.addEventListener("mousemove", mousemove, false);
    $(document).mousedown(mousedown);
    $(document).mouseup(mouseup);
    $(document).keydown(keydown);
    $(document).keyup(keyup);
  } else {
    document.removeEventListener("mousemove", mousemove, false);
  }
}

function mousemove(e) {
  var movementX = e.movementX ||
      e.mozMovementX          ||
      e.webkitMovementX       ||
      0;

  var movementY = e.movementY ||
      e.mozMovementY      ||
      e.webkitMovementY   ||
      0;

  worker.postMessage({cmd: "mouse_event",
                      dx: movementX,
                      dy: movementY,
                      buttons_state: buttons_state});
}

function mousedown(event) {
    switch (event.which) {
        case 1:
            buttons_state |= 1;
            break;
        case 2:
            buttons_state |= 4;
            break;
        case 3:
            buttons_state |= 2;
            break;
    }
    worker.postMessage({cmd: "mouse_event",
                        dx: 0,
                        dy: 0,
                        buttons_state: buttons_state });
}

function mouseup(event) {
    switch (event.which) {
        case 1:
            buttons_state &= ~1;
            break;
        case 2:
            buttons_state &= ~4;
            break;
        case 3:
            buttons_state &= ~2;
            break;
    }
    worker.postMessage({cmd: "mouse_event",
                        dx: 0,
                        dy: 0,
                        buttons_state: buttons_state });
}

function keydown(event) {
    worker.postMessage({cmd: "key_down", keycode: event.which });
    if (event.which === 8) {
      event.preventDefault();
    }
}

function keyup(event) {
    worker.postMessage({cmd: "key_up", keycode: event.which });
    if (event.which === 8) {
      event.preventDefault();
    }
}


/*

  html5 canvas loading animation.
  
  when minimized has a smaller size than
  equivalent .gif or pure CSS and can be 
  scaled to any dimensions. 
  
  Written to be small rather than fast or
  readable!
  
  remi-shergold.co.uk

*/

function loadingCanvas() {
    var pi = Math.PI,
        xCenter = canvas.width/2,
        yCenter = canvas.height/2,
        radius = canvas.height/3,
        startSize = radius/3,
        num=5,
        posX=[],posY=[],angle,size,i;
    
    loadingCanvasStep();
    function loadingCanvasStep() {
        setTimeout(function () {
            num++;
            ctx.clearRect ( 0 , 0 , xCenter*2 , yCenter*2 );
            for (i=0; i<9; i++){
              ctx.beginPath();
              ctx.fillStyle = 'rgba(69,99,255,'+.1*i+')';
              if (posX.length==i){
                angle = pi*i*.25;
                posX[i] = xCenter + radius * Math.cos(angle);
                posY[i] = yCenter + radius * Math.sin(angle);
              }
              ctx.arc(
                posX[(i+num)%8],
                posY[(i+num)%8],
                startSize/9*i,
                0, pi*2, 1); 
              ctx.fill();
            }
            if (!system_started) {
                loadingCanvasStep();
            }
        }, 100);
    }
}
