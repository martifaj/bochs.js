var LibraryUART = {
    $typeInBuffer: [],
    init_uart_js__deps: ['$typeInBuffer'],
    init_uart_js: function() {
       /*self.addEventListener('message', function(e) {
           typeInBuffer.push(e.data);
       }, false);*/
    },
    write_uart_js: function(text) {
       Module['msg'].push({
           msg: "serial",
           data: text
       });
    },
    read_uart_js__deps: ['$typeInBuffer'],
    read_uart_js: function() {
       return typeInBuffer.shift(); 
    },
    has_data_uart_js__deps: ['$typeInBuffer'],
    has_data_uart_js: function() {
       return typeInBuffer.length > 0;
    },
};
mergeInto(LibraryManager.library, LibraryUART);
