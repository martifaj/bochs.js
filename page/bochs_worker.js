var Module = {
    msg: {
        push: function () {
            self.postMessage.apply(self, arguments);
        },
        registerEventHandler: function (eventHandler) {
            self.addEventListener('message', eventHandler);
        },
    },
};

importScripts("bochs.js");
