var LibraryEventDispatcher = {
    init_event_dispatcher_js: function() {
       Module['msg'].registerEventHandler(function(e) {
        var msg = e.data;
        switch (msg.cmd) {
            case "mouse_event":
                _mousejs_motion(msg.dx, -msg.dy, 0, msg.buttons_state, 0);
                break;
            case "key_up":
                _kbdjs_keyup(msg.keycode);
                break;
            case "key_down":
                _kbdjs_keydown(msg.keycode);
                break;
        }
        }, false);
    },
};
mergeInto(LibraryManager.library, LibraryEventDispatcher);
