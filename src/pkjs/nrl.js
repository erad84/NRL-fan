var COMP_IDS = {
  0: 111,
  1: 161,
  2: 116,
  3: 156
};

var SHORT = {
  Broncos: "BRO",
  Bulldogs: "BUL",
  Cowboys: "COW",
  Dolphins: "DOL",
  Dragons: "DRA",
  Eels: "EEL",
  Knights: "KNI",
  Panthers: "PAN",
  Rabbitohs: "SOU",
  Raiders: "CAN",
  Roosters: "SYD",
  "Sea Eagles": "MAN",
  Sharks: "CRO",
  Storm: "MEL",
  Titans: "GLD",
  Warriors: "WAR",
  "Wests Tigers": "WST",
  Blues: "NSW",
  Maroons: "QLD",
  "Blues Women": "NSW"
};

var cache = {};
var CACHE_DRAW = 5 * 60 * 1000;
var CACHE_LIVE = 20 * 1000;
var CACHE_TEAM = 10 * 60 * 1000;
var CACHE_LADDER = 5 * 60 * 1000;
var lastUpcoming = null;
var PIN_STORE = "nrl-pinned-ids";
var reminderLeadMins = [60];

function seasonYear() {
  return new Date().getFullYear();
}

function decodeEntities(s) {
  return s
    .replace(/&quot;/g, '"')
    .replace(/&#39;/g, "'")
    .replace(/&lt;/g, "<")
    .replace(/&gt;/g, ">")
    .replace(/&amp;/g, "&");
}

function extractQData(html, id) {
  var marker = 'id="' + id + '"';
  var start = html.indexOf(marker);
  if (start < 0) {
    return null;
  }
  var q = html.indexOf('q-data="', start);
  if (q < 0) {
    return null;
  }
  q += 8;
  var end = html.indexOf('"', q);
  if (end < 0) {
    return null;
  }
  try {
    return JSON.parse(decodeEntities(html.substring(q, end)));
  } catch (e) {
    return null;
  }
}

function fetchPage(url, callback, attempt) {
  attempt = attempt || 0;
  var done = false;
  var req = new XMLHttpRequest();
  req.open("GET", url, true);

  function finish() {
    if (done) {
      return;
    }
    done = true;
    var status = req.status;
    if (status >= 200 && status < 300 && req.responseText) {
      callback(null, req.responseText);
    } else if ((status >= 500 || status === 0) && attempt < 2) {
      fetchPage(url, callback, attempt + 1);
    } else if (status === 0) {
      callback(new Error("network"));
    } else {
      callback(new Error("HTTP " + status));
    }
  }

  req.onreadystatechange = function () {
    if (req.readyState === 4) {
      finish();
    }
  };
  req.onload = finish;
  req.onerror = function () {
    if (done) {
      return;
    }
    if (attempt < 2) {
      done = true;
      fetchPage(url, callback, attempt + 1);
    } else {
      done = true;
      callback(new Error("network"));
    }
  };
  req.send();
}

function cachedGet(url, maxAge, elementId, callback) {
  var now = Date.now();
  var hit = cache[url];
  if (hit && now - hit.t < maxAge) {
    callback(null, hit.data);
    return;
  }
  fetchPage(url, function (err, html) {
    if (err) {
      callback(err);
      return;
    }
    var data = extractQData(html, elementId);
    if (!data) {
      callback(new Error("parse"));
      return;
    }
    discoverComps(data.filterCompetitions);
    cache[url] = { t: now, data: data };
    callback(null, data);
  });
}

function discoverComps(list) {
  if (!list || !list.length) {
    return;
  }
  for (var i = 0; i < list.length; i++) {
    var name = String(list[i].name || "").toLowerCase();
    var value = list[i].value;
    if (name.indexOf("u19") >= 0) {
      continue;
    }
    if (name.indexOf("origin") >= 0 && name.indexOf("women") >= 0) {
      COMP_IDS[3] = value;
    } else if (name.indexOf("origin") >= 0) {
      COMP_IDS[2] = value;
    } else if (name.indexOf("women") >= 0 && name.indexOf("premiership") >= 0) {
      COMP_IDS[1] = value;
    } else if (name === "telstra premiership") {
      COMP_IDS[0] = value;
    }
  }
}

function compId(comp) {
  return COMP_IDS[comp] || 111;
}

function isOrigin(comp) {
  return comp === 2 || comp === 3;
}

function shortCode(nick) {
  if (!nick) {
    return "???";
  }
  if (SHORT[nick]) {
    return SHORT[nick];
  }
  if (/blues/i.test(nick)) {
    return "NSW";
  }
  if (/maroons/i.test(nick)) {
    return "QLD";
  }
  return nick.replace(/[^A-Za-z]/g, "").slice(0, 3).toUpperCase() || "???";
}

function teamMatches(nick, fav, comp) {
  if (!nick || !fav) {
    return false;
  }
  if (isOrigin(comp)) {
    if (fav === "NSW") {
      return /blues/i.test(nick);
    }
    if (fav === "QLD") {
      return /maroons/i.test(nick);
    }
  }
  return nick === fav;
}

function fixtureInvolves(f, fav, comp) {
  if (f.type === "Bye") {
    return teamMatches(f.teamNickName, fav, comp);
  }
  if (f.type !== "Match") {
    return false;
  }
  return teamMatches((f.homeTeam || {}).nickName, fav, comp) ||
    teamMatches((f.awayTeam || {}).nickName, fav, comp);
}

function isComplete(f) {
  if (f.type !== "Match") {
    return false;
  }
  var state = f.matchState || "";
  var mode = f.matchMode || "";
  return state === "FullTime" || mode === "Post";
}

function isLive(f) {
  if (f.type !== "Match") {
    return false;
  }
  var state = f.matchState || "";
  var mode = f.matchMode || "";
  return mode === "Live" || state === "Live" || state === "FirstHalf" ||
    state === "SecondHalf" || state === "HalfTime" || state === "GoldenPoint";
}

function isUpcoming(f) {
  return f.type === "Match" && !isComplete(f) && !isLive(f);
}

function roundNum(title) {
  var m = String(title || "").match(/(\d+)/);
  return m ? parseInt(m[1], 10) : 0;
}

function lastCompletedRound(fx) {
  var max = 0;
  for (var i = 0; i < fx.length; i++) {
    if (isComplete(fx[i])) {
      var n = roundNum(fx[i].roundTitle);
      if (n > max) {
        max = n;
      }
    }
  }
  return max;
}

function isFutureBye(f, lastDone) {
  return f.type === "Bye" && roundNum(f.roundTitle) > lastDone;
}

function roundLabel(title, comp) {
  var text = title || "";
  var origin = isOrigin(comp);
  if (/grand\s*final/i.test(text)) {
    return "Grand Final";
  }
  if (/prelim/i.test(text)) {
    return "Prelim Final";
  }
  if (/semi/i.test(text)) {
    return "Semi Final";
  }
  if (/qualif/i.test(text)) {
    return "Qual Final";
  }
  if (/eliminat/i.test(text)) {
    return "Elim Final";
  }
  var week = text.match(/week\s*(\d+)/i);
  if (week && /final/i.test(text)) {
    return "Finals W" + week[1];
  }
  var m = text.match(/Round\s+(\d+)/i) || text.match(/Game\s+(\d+)/i);
  if (m) {
    return (origin ? "Game " : "Round ") + m[1];
  }
  return text.slice(0, 12) || (origin ? "Game" : "Round");
}

function isFinalsMatch(f) {
  return f && f.type === "Match" && /final/i.test(String(f.roundTitle || ""));
}

function finalsRank(title) {
  var t = String(title || "").toLowerCase();
  if (/grand/.test(t)) {
    return 0;
  }
  if (/prelim/.test(t) || /week\s*3/.test(t)) {
    return 1;
  }
  if (/semi/.test(t) || /week\s*2/.test(t)) {
    return 2;
  }
  return 3;
}

function kickoffMs(f) {
  var iso = ((f && f.clock) || {}).kickOffTimeLong;
  var n = Date.parse(iso || 0);
  return isNaN(n) ? 0 : n;
}

function compareFinals(a, b) {
  var ra = finalsRank(a.roundTitle);
  var rb = finalsRank(b.roundTitle);
  if (ra !== rb) {
    return ra - rb;
  }
  return kickoffMs(b) - kickoffMs(a);
}

function finalsLines(fx, comp) {
  var titles = ["Grand Final", "Preliminary Finals", "Semi Finals", "Week 1"];
  var games = fx.filter(isFinalsMatch).sort(compareFinals);
  var lines = [];
  var last = -1;
  for (var i = 0; i < games.length && lines.length < 32; i++) {
    var rank = finalsRank(games[i].roundTitle);
    if (rank !== last) {
      lines.push("HDR|" + titles[rank]);
      last = rank;
    }
    if (lines.length >= 32) {
      break;
    }
    lines.push(matchLine(games[i], comp));
  }
  return lines;
}

function formatKickoff(iso) {
  if (!iso) {
    return "";
  }
  var d = new Date(iso);
  if (isNaN(d.getTime())) {
    return "";
  }
  var days = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
  var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
  var h = d.getHours();
  var m = d.getMinutes();
  var mm = m < 10 ? "0" + m : String(m);
  var hour = h % 12;
  if (hour === 0) {
    hour = 12;
  }
  var ap = h >= 12 ? "p" : "a";
  return days[d.getDay()] + " " + d.getDate() + " " + months[d.getMonth()] + " " + hour + ":" + mm + ap;
}

function liveExtra(f) {
  var clock = f.clock || {};
  var game = clock.gameTime || "";
  var mins = game.split(":")[0];
  if (f.matchState === "HalfTime") {
    return "HT";
  }
  if (mins) {
    return mins + "'";
  }
  return "";
}

function matchLine(f, comp) {
  if (f.type === "Bye") {
    return [roundLabel(f.roundTitle, comp), shortCode(f.teamNickName), "-", "-", "-", "BYE", ""].join("|");
  }
  var home = f.homeTeam || {};
  var away = f.awayTeam || {};
  var hs = (home.score === 0 || home.score) ? String(home.score) : "-";
  var as = (away.score === 0 || away.score) ? String(away.score) : "-";
  var state = "UP";
  var extra = formatKickoff((f.clock || {}).kickOffTimeLong);
  if (isLive(f)) {
    state = "LIVE";
    extra = liveExtra(f);
  } else if (isComplete(f)) {
    state = "FT";
  }
  return [
    roundLabel(f.roundTitle, comp),
    shortCode(home.nickName),
    hs,
    shortCode(away.nickName),
    as,
    state,
    extra
  ].join("|");
}

function drawUrl(comp, teamId, year) {
  var url = "https://www.nrl.com/draw/?competition=" + compId(comp) +
    "&season=" + (year || seasonYear());
  if (teamId) {
    url += "&team=" + teamId;
  }
  return url;
}

function ladderUrl(comp, year) {
  return "https://www.nrl.com/ladder/?competition=" + compId(comp) +
    "&season=" + (year || seasonYear());
}

function findTeamId(filterTeams, fav) {
  if (!filterTeams) {
    return null;
  }
  for (var i = 0; i < filterTeams.length; i++) {
    if (filterTeams[i].name === fav) {
      return filterTeams[i].value;
    }
  }
  return null;
}

function matchesOf(data) {
  return (data.fixtures || []).filter(function (f) {
    return f.type === "Match" || f.type === "Bye";
  });
}

function originSeriesTitle(fixtures) {
  var nsw = 0;
  var qld = 0;
  fixtures.forEach(function (f) {
    if (f.type !== "Match" || !isComplete(f)) {
      return;
    }
    var home = f.homeTeam || {};
    var away = f.awayTeam || {};
    if (home.score === away.score) {
      return;
    }
    var winner = home.score > away.score ? home.nickName : away.nickName;
    if (/blues/i.test(winner)) {
      nsw++;
    } else if (/maroons/i.test(winner)) {
      qld++;
    }
  });
  return "NSW " + nsw + "-" + qld + " QLD";
}

function nickOf(p) {
  return p.teamNickname || p.teamNickName || p.teamNickname || "";
}

function ladderLines(data) {
  var positions = data.positions || [];
  var lines = [];
  for (var i = 0; i < positions.length && lines.length < 18; i++) {
    var p = positions[i];
    var s = p.stats || {};
    var pd = s["points difference"];
    var pdText = (pd > 0 ? "+" : "") + String(pd);
    lines.push([
      String(i + 1),
      shortCode(nickOf(p)),
      String(s.played != null ? s.played : 0),
      String(s.wins != null ? s.wins : 0),
      String(s.lost != null ? s.lost : 0),
      String(s.drawn != null ? s.drawn : 0),
      pdText,
      String(s.points != null ? s.points : 0)
    ].join("|"));
  }
  return lines;
}

function addStat(lines, label, value) {
  if (value == null || value === "") {
    return;
  }
  lines.push(label + "|" + value);
}

function ladderStats(data, query) {
  var positions = data.positions || [];
  for (var i = 0; i < positions.length; i++) {
    var p = positions[i];
    var nick = nickOf(p);
    if (nick !== query && shortCode(nick) !== query) {
      continue;
    }
    var s = p.stats || {};
    var lines = [];
    addStat(lines, "Position", i + 1);
    addStat(lines, "Played", s.played);
    addStat(lines, "Won", s.wins);
    addStat(lines, "Drawn", s.drawn);
    addStat(lines, "Lost", s.lost);
    addStat(lines, "Byes", s.byes);
    addStat(lines, "Points", s.points);
    addStat(lines, "For", s["points for"]);
    addStat(lines, "Against", s["points against"]);
    addStat(lines, "Diff", s["points difference"]);
    addStat(lines, "Home", s["home record"]);
    addStat(lines, "Away", s["away record"]);
    addStat(lines, "Form", s.form);
    addStat(lines, "Streak", s.streak);
    addStat(lines, "Avg win", s["average winning margin"]);
    addStat(lines, "Avg loss", s["average losing margin"]);
    addStat(lines, "Close", s["close games"]);
    addStat(lines, "Golden pt", s["golden point"]);
    addStat(lines, "Day", s["day record"]);
    addStat(lines, "Night", s["night record"]);
    addStat(lines, "Players", s["players used"]);
    return { title: nick || query, lines: lines };
  }
  return { title: query || "Stats", lines: [] };
}

function loadDraw(comp, teamId, year, maxAge, callback) {
  cachedGet(drawUrl(comp, teamId, year), maxAge, "vue-draw", callback);
}

function loadFinalsDraw(comp, year, callback) {
  loadDraw(comp, null, year, CACHE_DRAW, function (err, data) {
    if (err) {
      callback(err);
      return;
    }
    var rounds = (data.filterRounds || []).filter(function (r) {
      return r && /final/i.test(String(r.name || ""));
    });
    if (!rounds.length) {
      callback(null, data);
      return;
    }
    var pending = rounds.length;
    var all = [];
    var lastErr = null;
    rounds.forEach(function (r) {
      var url = drawUrl(comp, null, year) + "&round=" + encodeURIComponent(r.value);
      cachedGet(url, CACHE_DRAW, "vue-draw", function (err2, roundData) {
        pending--;
        if (err2) {
          lastErr = err2;
        } else {
          all = all.concat(matchesOf(roundData));
        }
        if (pending > 0) {
          return;
        }
        if (!all.length) {
          callback(lastErr || new Error("parse"));
          return;
        }
        callback(null, { fixtures: all });
      });
    });
  });
}

function loadLadder(comp, year, callback) {
  var primary = ladderUrl(comp, year);
  cachedGet(primary, CACHE_LADDER, "vue-ladder", function (err, data) {
    if (!err) {
      callback(null, data);
      return;
    }
    var fallback = "https://www.nrl.com/ladder/?competition=" + compId(comp);
    if (fallback === primary) {
      callback(err);
      return;
    }
    cachedGet(fallback, CACHE_LADDER, "vue-ladder", callback);
  });
}

function withTeamDraw(comp, fav, year, callback) {
  loadDraw(comp, null, year, CACHE_DRAW, function (err, data) {
    if (err) {
      callback(err);
      return;
    }
    if (isOrigin(comp)) {
      callback(null, data);
      return;
    }
    var teamId = findTeamId(data.filterTeams, fav);
    if (!teamId) {
      callback(null, data);
      return;
    }
    loadDraw(comp, teamId, year, CACHE_TEAM, callback);
  });
}

function animalName(nick) {
  if (nick === "Wests Tigers") {
    return "Tigers";
  }
  if (nick === "NSW") {
    return "Blues";
  }
  if (nick === "QLD") {
    return "Maroons";
  }
  return nick || "Team";
}

function ladderPos(data, fav, comp) {
  var positions = data.positions || [];
  for (var i = 0; i < positions.length; i++) {
    var nick = nickOf(positions[i]);
    if (teamMatches(nick, fav, comp) || shortCode(nick) === shortCode(fav)) {
      return String(i + 1);
    }
  }
  return "";
}

function opponentOf(f, fav, comp) {
  if (!f || f.type === "Bye") {
    return { code: "BYE", name: "Bye" };
  }
  var home = f.homeTeam || {};
  var away = f.awayTeam || {};
  if (teamMatches(home.nickName, fav, comp)) {
    return { code: shortCode(away.nickName), name: animalName(away.nickName) };
  }
  return { code: shortCode(home.nickName), name: animalName(home.nickName) };
}

function handleRequest(comp, req, fav, year, team, callback) {
  if (req === 5) {
    if (isOrigin(comp)) {
      loadDraw(comp, null, year, CACHE_DRAW, function (err, data) {
        if (err) {
          callback(err);
          return;
        }
        var fx = matchesOf(data).filter(function (f) { return f.type === "Match"; });
        callback(null, {
          title: originSeriesTitle(fx),
          lines: fx.map(function (f) { return matchLine(f, comp); })
        });
      });
      return;
    }
    loadLadder(comp, year, function (err, data) {
      if (err) {
        callback(err);
        return;
      }
      callback(null, {
        title: "Ladder",
        lines: ladderLines(data)
      });
    });
    return;
  }

  if (req === 7) {
    loadLadder(comp, year, function (err, data) {
      if (err) {
        callback(err);
        return;
      }
      callback(null, ladderStats(data, team || fav));
    });
    return;
  }

  if (req === 2) {
    loadDraw(comp, null, year, CACHE_LIVE, function (err, data) {
      if (err) {
        callback(err);
        return;
      }
      var fx = matchesOf(data).filter(function (f) { return f.type === "Match"; });
      fx.sort(function (a, b) {
        return (isLive(b) ? 1 : 0) - (isLive(a) ? 1 : 0);
      });
      var liveCount = 0;
      for (var i = 0; i < fx.length; i++) {
        if (isLive(fx[i])) {
          liveCount++;
        }
      }
      var rnd = fx[0] ? roundLabel(fx[0].roundTitle, comp) : "Live";
      callback(null, {
        title: liveCount ? rnd + " LIVE" : (rnd || "Live"),
        lines: fx.map(function (f) { return matchLine(f, comp); })
      });
    });
    return;
  }

  if (req === 8) {
    var finishSummary = function (pos, data, err) {
      if (err) {
        callback(err);
        return;
      }
      var fx = matchesOf(data).filter(function (f) {
        return fixtureInvolves(f, fav, comp);
      });
      var live = fx.filter(isLive)[0];
      var lastDone = lastCompletedRound(fx);
      var next = fx.filter(function (f) {
        return isUpcoming(f) || isFutureBye(f, lastDone);
      })[0];
      var f = live || next;
      var rnd = f ? roundLabel(f.roundTitle, comp) : (isOrigin(comp) ? "Game" : "Round");
      var lines = ["POS|" + (pos || "-")];
      if (!f) {
        lines.push("NEXT|||");
      } else if (f.type === "Bye") {
        lines.push("NEXT|BYE|Bye");
      } else {
        var opp = opponentOf(f, fav, comp);
        lines.push("NEXT|" + opp.code + "|" + opp.name);
      }
      callback(null, { title: rnd, lines: lines });
    }
    if (isOrigin(comp)) {
      loadDraw(comp, null, year, CACHE_DRAW, function (err, data) {
        finishSummary("", data, err);
      });
      return;
    }
    loadLadder(comp, year, function (err, ladder) {
      var pos = "";
      if (!err && ladder) {
        pos = ladderPos(ladder, fav, comp);
      }
      withTeamDraw(comp, fav, year, function (err2, data) {
        finishSummary(pos, data, err2);
      });
    });
    return;
  }

  var queryTeam = team || fav;
  if (req === 6 && queryTeam === "Finals") {
    loadFinalsDraw(comp, year, function (err, data) {
      if (err) {
        callback(err);
        return;
      }
      callback(null, {
        title: "Finals " + (year || seasonYear()),
        lines: finalsLines(matchesOf(data), comp)
      });
    });
    return;
  }

  withTeamDraw(comp, queryTeam, year, function (err, data) {
    if (err) {
      callback(err);
      return;
    }
    var fx = matchesOf(data);
    if (!isOrigin(comp)) {
      fx = fx.filter(function (f) { return fixtureInvolves(f, queryTeam, comp); });
    }

    if (req === 1) {
      var live = fx.filter(isLive);
      var done = fx.filter(isComplete);
      var up = fx.filter(isUpcoming);
      var lastDone = lastCompletedRound(fx);
      var lines = [];
      live.forEach(function (f) { lines.push(matchLine(f, comp)); });
      if (done.length) {
        lines.push(matchLine(done[done.length - 1], comp));
      }
      if (up.length) {
        lines.push(matchLine(up[0], comp));
      } else {
        var nextBye = fx.filter(function (f) { return isFutureBye(f, lastDone); })[0];
        if (nextBye) {
          lines.push(matchLine(nextBye, comp));
        }
      }
      callback(null, { title: queryTeam || "My Team", lines: lines });
      return;
    }

    if (req === 3) {
      var lastDoneUp = lastCompletedRound(fx);
      var upcoming = fx.filter(function (f) {
        return isUpcoming(f) || isFutureBye(f, lastDoneUp);
      });
      var shown = upcoming.slice(0, 16);
      lastUpcoming = {
        comp: comp,
        fav: queryTeam,
        full: upcoming,
        shown: shown
      };
      callback(null, {
        title: animalName(queryTeam) + " upcoming rounds",
        lines: shown.map(function (f) { return upcomingLine(f, comp); })
      });
      return;
    }

    if (req === 4) {
      var results = fx.filter(function (f) { return f.type === "Match" && isComplete(f); });
      results = results.reverse();
      if (results.length > 32) {
        results = results.slice(0, 32);
      }
      callback(null, {
        title: "My Team Results",
        lines: results.map(function (f) { return matchLine(f, comp); })
      });
      return;
    }

    if (req === 6) {
      callback(null, {
        title: animalName(queryTeam) + " " + (year || seasonYear()),
        lines: fx.map(function (f) { return matchLine(f, comp); })
      });
      return;
    }

    callback(new Error("bad req"));
  });
}

function loadPinned() {
  try {
    return JSON.parse(localStorage.getItem(PIN_STORE) || "{}") || {};
  } catch (e) {
    return {};
  }
}

function rememberPins(pins) {
  var map = loadPinned();
  var now = Date.now();
  var i;
  for (i = 0; i < (pins || []).length; i++) {
    var pin = pins[i];
    if (!pin || !pin.id) {
      continue;
    }
    map[pin.id] = Date.parse(pin.time) || now;
  }
  Object.keys(map).forEach(function (id) {
    if (map[id] && map[id] + 4 * 60 * 60 * 1000 < now) {
      delete map[id];
    }
  });
  try {
    localStorage.setItem(PIN_STORE, JSON.stringify(map));
  } catch (e) {
    return;
  }
}

function isPinnedId(id) {
  return !!(id && loadPinned()[id]);
}

function isPinnedFixture(comp, f) {
  return f && f.type === "Match" && isPinnedId(pinId(comp, f));
}

function upcomingLine(f, comp) {
  var line = matchLine(f, comp);
  if (isPinnedFixture(comp, f)) {
    line += "|1";
  }
  return line;
}

function setReminderLeadMins(raw) {
  var s = String(raw == null ? "" : raw);
  if (s === "15+60") {
    reminderLeadMins = [60, 15];
    return;
  }
  var n = parseInt(s, 10);
  reminderLeadMins = n === 15 || n === 60 ? [n] : [];
}

function remindersFor(iso, title) {
  var list = [];
  var i;
  if (!iso || !reminderLeadMins.length) {
    return list;
  }
  for (i = 0; i < reminderLeadMins.length; i++) {
    var at = new Date(Date.parse(iso) - reminderLeadMins[i] * 60 * 1000);
    if (at.getTime() > Date.now()) {
      list.push({
        time: at.toISOString().replace(/\.\d{3}Z$/, "Z"),
        layout: {
          type: "genericReminder",
          title: title || "NRL",
          tinyIcon: "system://images/ALARM_CLOCK"
        }
      });
    }
  }
  return list;
}

function venueName(f) {
  if (!f) {
    return "";
  }
  if (typeof f.venue === "string" && f.venue) {
    return f.venue;
  }
  var venue = f.venue || {};
  return venue.name || venue.nickName || venue.venueName || f.venueName || f.ground || "";
}

function pinIcon() {
  return "system://images/AMERICAN_FOOTBALL";
}

function pinBody(comp, f) {
  var lines = [];
  var rnd = roundLabel(f && f.roundTitle, comp);
  if (rnd) {
    lines.push(rnd);
  }
  var when = formatKickoff(((f && f.clock) || {}).kickOffTimeLong);
  if (when) {
    lines.push(when);
  }
  var venue = venueName(f);
  if (venue) {
    lines.push(venue);
  }
  if (!lines.length) {
    lines.push("Upcoming NRL match.");
  }
  return lines.join("\n");
}

function kickoffIso(f) {
  var ms = kickoffMs(f);
  if (!ms) {
    return "";
  }
  return new Date(ms).toISOString().replace(/\.\d{3}Z$/, "Z");
}

function pinId(comp, f) {
  var home = shortCode((f.homeTeam || {}).nickName);
  var away = shortCode((f.awayTeam || {}).nickName);
  var ms = kickoffMs(f);
  var year = ms ? new Date(ms).getFullYear() : seasonYear();
  var rid = String(f.roundId || f.round || roundNum(f.roundTitle) || "0");
  rid = rid.replace(/[^A-Za-z0-9]/g, "").slice(0, 16);
  return ("nrl-" + compId(comp) + "-" + year + "-" + rid + "-" + home + "-" + away).slice(0, 64);
}

function buildSportsPin(comp, f) {
  var iso = kickoffIso(f);
  if (!iso) {
    return null;
  }
  var home = f.homeTeam || {};
  var away = f.awayTeam || {};
  var homeName = animalName(home.nickName);
  var awayName = animalName(away.nickName);
  var venue = venueName(f);
  var pin = {
    id: pinId(comp, f),
    time: iso,
    duration: 100,
    layout: {
      type: "genericPin",
      title: homeName + " v " + awayName,
      subtitle: roundLabel(f.roundTitle, comp),
      body: pinBody(comp, f),
      locationName: venue || "NRL",
      tinyIcon: pinIcon(),
      largeIcon: pinIcon(),
      lastUpdated: new Date().toISOString().replace(/\.\d{3}Z$/, "Z"),
      nameHome: shortCode(home.nickName).slice(0, 4),
      nameAway: shortCode(away.nickName).slice(0, 4),
      sportsGameState: "pre-game"
    }
  };
  var reminders = remindersFor(iso, homeName + " v " + awayName);
  if (reminders.length) {
    pin.reminders = reminders;
  }
  return pin;
}

function pinsFromRow(comp, row, all) {
  if (!lastUpcoming || !lastUpcoming.shown || !lastUpcoming.full) {
    return [];
  }
  row = parseInt(row, 10);
  if (isNaN(row) || row < 0 || row >= lastUpcoming.shown.length) {
    return [];
  }
  var start = lastUpcoming.shown[row];
  var full = lastUpcoming.full;
  var startIdx = 0;
  for (var i = 0; i < full.length; i++) {
    if (full[i] === start) {
      startIdx = i;
      break;
    }
  }
  var fixtures = [];
  if (all) {
    for (var j = startIdx; j < full.length; j++) {
      if (isUpcoming(full[j])) {
        fixtures.push(full[j]);
      }
    }
  } else if (isUpcoming(start)) {
    fixtures.push(start);
  }
  var pins = [];
  for (var k = 0; k < fixtures.length; k++) {
    var pin = buildSportsPin(comp, fixtures[k]);
    if (pin) {
      pins.push(pin);
    }
  }
  return pins;
}

function preparePins(comp, fav, row, all, year, team, callback) {
  var queryTeam = team || fav;
  row = parseInt(row, 10) || 0;
  function finish() {
    var pins = pinsFromRow(comp, row, all);
    if (!pins.length) {
      callback(new Error("Pin failed"));
      return;
    }
    callback(null, pins);
  }
  if (lastUpcoming && lastUpcoming.comp === comp && lastUpcoming.fav === queryTeam) {
    finish();
    return;
  }
  handleRequest(comp, 3, fav, year, team, function (err) {
    if (err) {
      callback(err);
      return;
    }
    finish();
  });
}

module.exports = {
  handleRequest: handleRequest,
  isOrigin: isOrigin,
  preparePins: preparePins,
  buildSportsPin: buildSportsPin,
  rememberPins: rememberPins,
  setReminderLeadMins: setReminderLeadMins
};
