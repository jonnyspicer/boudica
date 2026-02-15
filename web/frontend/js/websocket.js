/**
 * WebSocket client with reconnect and message dispatch.
 */
var Boudica = window.Boudica || {};

Boudica.WebSocketClient = (function () {
    var ws = null;
    var gameId = null;
    var handlers = {};
    var reconnectAttempts = 0;
    var maxReconnect = 5;
    var reconnectTimer = null;

    function connect(id) {
        // Kill any pending reconnect for the old game
        clearTimeout(reconnectTimer);
        reconnectAttempts = 0;
        maxReconnect = 5;

        // Close old socket without triggering reconnect
        if (ws) {
            ws.onclose = null;
            ws.onerror = null;
            if (ws.readyState <= 1) {
                ws.close();
            }
            ws = null;
        }

        gameId = id;
        _doConnect();
    }

    function _doConnect() {
        var protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
        var url = protocol + '//' + location.host + '/ws/game/' + gameId;
        var connectingId = gameId;
        ws = new WebSocket(url);

        ws.onopen = function () {
            if (gameId !== connectingId) return;
            reconnectAttempts = 0;
            _emit('connected', {});
        };

        ws.onmessage = function (event) {
            if (gameId !== connectingId) return;
            try {
                var msg = JSON.parse(event.data);
                _emit(msg.type, msg);
            } catch (e) {
                console.error('WS parse error:', e);
            }
        };

        ws.onclose = function () {
            if (gameId !== connectingId) return;
            _emit('disconnected', {});
            _scheduleReconnect();
        };

        ws.onerror = function () {
            // onclose will fire after this
        };
    }

    function _scheduleReconnect() {
        if (reconnectAttempts >= maxReconnect) return;
        reconnectAttempts++;
        var delay = Math.min(1000 * Math.pow(2, reconnectAttempts), 30000);
        clearTimeout(reconnectTimer);
        reconnectTimer = setTimeout(_doConnect, delay);
    }

    function send(data) {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify(data));
        }
    }

    function on(type, fn) {
        if (!handlers[type]) handlers[type] = [];
        handlers[type].push(fn);
    }

    function off(type, fn) {
        if (!handlers[type]) return;
        handlers[type] = handlers[type].filter(function (h) { return h !== fn; });
    }

    function _emit(type, data) {
        var fns = handlers[type] || [];
        for (var i = 0; i < fns.length; i++) {
            try { fns[i](data); } catch (e) { console.error('Handler error:', e); }
        }
    }

    function disconnect() {
        clearTimeout(reconnectTimer);
        maxReconnect = 0; // prevent reconnect
        if (ws) ws.close();
    }

    return {
        connect: connect,
        send: send,
        on: on,
        off: off,
        disconnect: disconnect
    };
})();
