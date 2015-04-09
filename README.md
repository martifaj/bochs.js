# bochs.js
This repository contains a port of bochs 2.6.2 to Javascript using Emscripten toolchain.
The emulator displays VGA text and graphics to an HTML canvas, and can also hook the serial port to Javascript code.
A demo of this project can be found in http://www.jaumemarti.org/bochs.js.html
## Building
To build the project you need to have Emscripten installed on your system and accessible from your PATH.
The following command will generate all the files required to build the project using Emscripten:
``{r, engine='sh', configure}
emconfigure ./configure
```
The directory `data` in the root of the project contains the bochs configuration file, bios and the image files for the hard drives, cdroms etc.
Make sure that this directory contains the corresponding files before you start the compilation.
Depending on the bochs configuration file (particularly the RAM size) you may need to change the emscripten configuration to allow a higher memory limit (see TOTAL_MEMORY variable) or to grow memory on demand (see ALLOW_MEMORY_GORWTH).
Check http://kripken.github.io/emscripten-site/docs/porting/emscripten-runtime-environment.html#emscripten-memory-model for more information about the way emscripten manages dynamic memory.
The following command should compile the project into a file named `bochs.js`:
```{r, engine='sh', make}
emmake make
```
Now you can include the Javascript file `bochs.js` in an HTML page, or execute it from Node.js. But before doing so you have to make sure the code has access to a variable named `Module`, which should contain two functions that will be used to send and receive messages between the emulator and custom Javascript code. 
```{r, engine='javascript', Module}
var Module = {
    msg: {
        push: function () {
            // sends a message from the emulator to custom Javascript code.
        },
        registerEventHandler: function (eventHandler) {
            // registers a function that will be called each time custom Javascript code sends a message to the emulator.
        },
    },
};
```
A sample implementation of those functions can be found in `js/bochs_worker.js`. This implementation expects the emulator to run inside a WebWorker.
The format of the different messages expected and sent by the emulator can be found in `js/bochs_ui.js`.

