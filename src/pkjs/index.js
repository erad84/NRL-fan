var Clay = require("@rebble/clay");
var clayConfig = require("./config");
var nrl = require("./nrl");

var clay = new Clay(clayConfig, null, { autoHandleEvents: true });

function settingValue(raw, key, fallback) {
  var value = raw[key];
  if (value && typeof value === "object" && value.value != null) {
    value = value.value;
  }
  return value == null || value === "" ? fallback : value;
}

function boolSetting(raw, key, fallback) {
  var value = settingValue(raw, key, fallback);
  return value === true || value === 1 || value === "1" || value === "true";
}

function readSettings() {
  var raw = {};
  try {
    raw = JSON.parse(localStorage.getItem("clay-settings") || "{}") || {};
  } catch (e) {
    raw = {};
  }
  return {
    FAV_NRL: settingValue(raw, "FAV_NRL", "Broncos"),
    FAV_NRLW: settingValue(raw, "FAV_NRLW", "Broncos"),
    FAV_ORIGIN: settingValue(raw, "FAV_ORIGIN", "NSW"),
    DEFAULT_COMP: parseInt(settingValue(raw, "DEFAULT_COMP", "0"), 10),
    PIN_REMINDER: settingValue(raw, "PIN_REMINDER", "60"),
    ODDS_RAW: boolSetting(raw, "ODDS_RAW", false)
  };
}

function favForComp(comp, settings) {
  if (comp === 1) {
    return settings.FAV_NRLW;
  }
  if (comp === 2 || comp === 3) {
    return settings.FAV_ORIGIN;
  }
  return settings.FAV_NRL;
}

function sendError(req, message) {
  Pebble.sendAppMessage({
    REQ: req,
    STATUS: 2,
    ERROR: String(message || "No data").slice(0, 18)
  });
}

function sendPinStatus(ok, title, error) {
  var msg = {
    REQ: 9,
    STATUS: ok ? 0 : 2,
    TITLE: String(title || "").slice(0, 28)
  };
  if (!ok) {
    msg.ERROR = String(error || "Pin failed").slice(0, 18);
  }
  Pebble.sendAppMessage(msg, function () {}, function () {
    Pebble.sendAppMessage(msg);
  });
}

function insertPinFn() {
  return Pebble.insertTimelinePin;
}

function localPin(pin) {
  var layout = pin.layout || {};
  var iso = pin.time || pin.time;
  var out = {
    id: String(pin.id).slice(0, 64),
    time: iso,
    layout: {
      type: "genericPin",
      title: layout.title || "NRL",
      tinyIcon: layout.tinyIcon || "system://images/AMERICAN_FOOTBALL",
      largeIcon: layout.largeIcon || layout.tinyIcon || "system://images/AMERICAN_FOOTBALL"
    }
  };
  if (layout.subtitle) {
    out.layout.subtitle = layout.subtitle;
  }
  if (layout.body) {
    out.layout.body = String(layout.body).slice(0, 512);
  }
  if (pin.reminders && pin.reminders.length) {
    out.reminders = pin.reminders;
  }
  return out;
}

function classicPin(pin) {
  var layout = pin.layout || {};
  var iso = pin.time || pin.time;
  var out = {
    id: String(pin.id).slice(0, 64),
    time: iso,
    duration: pin.duration || pin.duration || 60,
    layout: {
      type: "genericPin",
      title: layout.title || "NRL",
      tinyIcon: layout.tinyIcon || "system://images/AMERICAN_FOOTBALL",
      largeIcon: layout.largeIcon || layout.tinyIcon || "system://images/AMERICAN_FOOTBALL"
    }
  };
  if (layout.subtitle) {
    out.layout.subtitle = layout.subtitle;
  }
  if (layout.body) {
    out.layout.body = String(layout.body).slice(0, 512);
  }
  var loc = layout.locationName || layout.locationName;
  if (loc) {
    out.layout.locationName = loc;
  }
  if (pin.reminders && pin.reminders.length) {
    out.reminders = pin.reminders;
  }
  return out;
}

function xhrPutPin(pin, token, callback) {
  var hosts = [
    "https://timeline-api.rebble.io/v1/user/pins/"
  ];
  var h = 0;
  function tryHost() {
    if (h >= hosts.length) {
      callback(new Error("Pin failed"));
      return;
    }
    var xhr = new XMLHttpRequest();
    var settled = false;
    function finish(err) {
      if (settled) {
        return;
      }
      settled = true;
      if (err) {
        h += 1;
        tryHost();
        return;
      }
      callback(null);
    }
    xhr.open("PUT", hosts[h] + encodeURIComponent(pin.id), true);
    xhr.setRequestHeader("Content-Type", "application/json");
    if (token) {
      xhr.setRequestHeader("X-User-Token", token);
    }
    xhr.onreadystatechange = function () {
      if (xhr.readyState !== 4) {
        return;
      }
      if (xhr.status >= 200 && xhr.status < 300) {
        finish(null);
      } else {
        finish(new Error("Pin failed"));
      }
    };
    xhr.onerror = function () {
      finish(new Error("Pin failed"));
    };
    setTimeout(function () {
      finish(new Error("Pin failed"));
    }, 8000);
    try {
      xhr.send(JSON.stringify(pin));
    } catch (e) {
      finish(new Error("Pin failed"));
    }
  }
  tryHost();
}

function putOnePin(pin, token, callback) {
  var insert = insertPinFn();
  if (typeof insert === "function") {
    try {
      insert.call(Pebble, localPin(pin));
      callback(null);
      return;
    } catch (e) {
      console.log("NRL Fan pin insert: " + e);
    }
  }
  xhrPutPin(classicPin(pin), token || "local", callback);
}

function pinToTimeline(pins, callback) {
  function run(token) {
    var i = 0;
    function next() {
      if (i >= pins.length) {
        callback(null, pins.length);
        return;
      }
      putOnePin(pins[i], token, function (err) {
        if (err) {
          callback(err);
          return;
        }
        i += 1;
        setTimeout(next, 80);
      });
    }
    next();
  }
  if (insertPinFn()) {
    run("");
    return;
  }
  if (typeof Pebble.getTimelineToken !== "function") {
    run("local");
    return;
  }
  var timedOut = false;
  var timer = setTimeout(function () {
    timedOut = true;
    run("local");
  }, 4000);
  try {
    Pebble.getTimelineToken(function (token) {
      if (timedOut) {
        return;
      }
      clearTimeout(timer);
      run(token || "local");
    }, function () {
      if (timedOut) {
        return;
      }
      clearTimeout(timer);
      run("local");
    });
  } catch (e) {
    clearTimeout(timer);
    run("local");
  }
}

function chunkLimit() {
  try {
    var info = typeof Pebble.getActiveWatchInfo === "function" ? Pebble.getActiveWatchInfo() : null;
    if (info && info.platform === "aplite") {
      return 220;
    }
  } catch (e) {
    /* ignore */
  }
  return 400;
}

function sendList(req, title, lines) {
  var limit = chunkLimit();
  var chunks = [];
  var buf = "";
  for (var i = 0; i < lines.length; i++) {
    var line = String(lines[i] || "");
    if (line.length > limit) {
      line = line.slice(0, limit);
    }
    var next = buf ? buf + "\n" + line : line;
    if (next.length > limit && buf) {
      chunks.push(buf);
      buf = line;
    } else {
      buf = next;
    }
  }
  if (buf) {
    chunks.push(buf);
  }
  if (!chunks.length) {
    chunks.push("");
  }

  function sendOne(index) {
    Pebble.sendAppMessage({
      REQ: req,
      STATUS: 0,
      TITLE: String(title || "").slice(0, 28),
      CHUNK: chunks[index],
      CHUNK_INDEX: index,
      CHUNK_COUNT: chunks.length
    }, function () {
      if (index + 1 < chunks.length) {
        sendOne(index + 1);
      }
    }, function () {
      console.log("NRL Fan: chunk send failed " + index);
    });
  }
  sendOne(0);
}

function syncDefaults() {
  var settings = readSettings();
  var comp = isNaN(settings.DEFAULT_COMP) ? 0 : settings.DEFAULT_COMP;
  Pebble.sendAppMessage({
    DEFAULT_COMP: comp,
    FAV_NRL: settings.FAV_NRL,
    FAV_NRLW: settings.FAV_NRLW,
    FAV_ORIGIN: settings.FAV_ORIGIN,
    ODDS_RAW: settings.ODDS_RAW ? 1 : 0
  });
}

Pebble.addEventListener("ready", function () {
  console.log("NRL Fan PKJS ready");
  syncDefaults();
});

Pebble.addEventListener("appmessage", function (e) {
  var payload = e.payload || {};
  var req = payload.REQ;
  var comp = payload.COMP;
  if (req == null) {
    return;
  }
  if (comp == null || isNaN(comp)) {
    comp = 0;
  }

  var settings = readSettings();
  var fav = favForComp(comp, settings);
  var year = payload.YEAR;
  var team = payload.TEAM;

  if (req === 9) {
    try {
      nrl.setReminderLeadMins(settings.PIN_REMINDER);
      if (!team) {
        sendPinStatus(false, "", "Pin failed");
        return;
      }
      nrl.prepareDrawRoundPins(comp, team, payload.ROW, function (err, pins) {
        if (err) {
          console.log("NRL Fan pin: " + err.message);
          sendPinStatus(false, "", err.message === "No Timeline token" ? "No Timeline token" : "Pin failed");
          return;
        }
        pinToTimeline(pins, function (pinErr, count) {
          if (pinErr) {
            console.log("NRL Fan pin: " + pinErr.message);
            sendPinStatus(false, "", pinErr.message === "No Timeline token" ? "No Timeline token" : "Pin failed");
            return;
          }
          nrl.rememberPins(pins);
          var title = count === 1 ? "Pinned" : ("Pinned " + count + " games");
          sendPinStatus(true, title, "");
        });
      });
    } catch (e) {
      sendPinStatus(false, "", "Pin failed");
    }
    return;
  }

  nrl.handleRequest(comp, req, fav, year, team, function (err, result) {
    if (err) {
      console.log("NRL Fan error: " + err.message);
      sendError(req, err.message === "network" ? "No network" : "No data");
      return;
    }
    sendList(req, result.title, result.lines || []);
  });
});
