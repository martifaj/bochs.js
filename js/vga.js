var LibraryVGA = {
    vgajs_dimension_update: function (x, y, bpp) {
       Module['msg'].push({cmd: "dimensionupdate",
                         data: {x: x,
                                y: y, 
                                bpp: bpp},
                        });
    },
    vgajs_setpalette: function (index, red, green, blue) {
       Module['msg'].push({cmd: "setpalette",
                         data: {index: index,
                                red: red, 
                                green: green, 
                                blue: blue},
                        });
    },
    vgajs_tile_update_in_place: function (x, y, x_tilesize, y_tilesize, subtile, pitch) {
        console.log("sending data");
       var buffer = new ArrayBuffer(x_tilesize * y_tilesize * 3);
       var tile = new Uint8ClampedArray(buffer);
       for (var tmpy = 0; tmpy < y_tilesize; tmpy++) {
           for (var tmpx = 0; tmpx < x_tilesize; tmpx++) {
               var i_subtile = tmpy*pitch + tmpx*4;
               var i_tile    = tmpy*x_tilesize*3 + tmpx*3;
               var curr_pixel = getValue(subtile+i_subtile, 'i32');
               tile[i_tile]     = curr_pixel & 0xFF;
               tile[i_tile + 1] = (curr_pixel >>  8) & 0xFF;
               tile[i_tile + 2] = (curr_pixel >> 16) & 0xFF;
           }
       }
       var result = {cmd: "tileupdate",
                      data: {x: x,
                             y: y, 
                             x_tilesize: x_tilesize,
                             y_tilesize: y_tilesize,
                             bpp: 24,
                             tile: buffer},
                     };
       Module['msg'].push(result, [result.data.tile]);
    },
    vgajs_tile_update: function (x, y, x_tilesize, y_tilesize, snapshot, bpp) {
       var buffer = new ArrayBuffer(x_tilesize * y_tilesize * (bpp>>3));
       var tile = new Uint8ClampedArray(buffer);
       for (var i = 0; i < tile.length; i++) {
           tile[i] = getValue(snapshot+i, 'i8');
       }
       var result = {cmd: "tileupdate",
                     data: {x: x,
                            y: y, 
                            x_tilesize: x_tilesize,
                            y_tilesize: y_tilesize,
                            bpp: bpp,
                            tile: buffer},
                    };
       Module['msg'].push(result, [result.data.tile]);
    },
    vgajs_clear_screen: function () {
       Module['msg'].push({cmd: "clearscreen"});
    },
};
mergeInto(LibraryManager.library, LibraryVGA);
