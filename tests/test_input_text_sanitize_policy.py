# Source-structure checks: every place where a station name or URL enters from
# outside, every place where stored records are loaded, and the three RTCP
# encoders must go through the shared text rules (browse_text.h / station_text.h /
# json_text.h). The behaviour itself is tested by test_input_text_sanitize.cpp
# and test_json_text_strict.py.
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parents[1]


def read(name):
    return (root / "src" / name).read_text(encoding="utf-8").replace("\r\n", "\n")


def between(text, start, end):
    a = text.index(start)
    return text[a:text.index(end, a)]


st = read("stations.cpp")
wp = read("web_portal.cpp")
au = read("audio.cpp")
rb = read("remote_browser.cpp")
rbh = read("remote_browser.h")
mn = read("main.cpp")
cat = read("rtcp_catalog.cpp")
fc = read("rtcp_full_control.cpp")
d9 = read("rtcp_d009.cpp")
stt = read("station_text.h")
jt = read("json_text.h")
bt = read("browse_text.h")

add_station = between(st, "uint8_t StationManager::addStation(", "bool StationManager::removeStation(")
add_recent = between(st, "void StationManager::addToRecent(", "RadioStation* StationManager::getRecent(")
add_fav = between(st, "bool StationManager::addToFavorites(", "bool StationManager::removeFavorite(")
load_saved = between(st, "bool StationManager::loadDefaultStations() {", "void StationManager::saveStationsToPrefs()")
load_lists = between(st, "void StationManager::loadPersistedLists() {", "void StationManager::addToRecent(")
handle_add = between(wp, "void WebPortal::handleAddStation() {", "void WebPortal::handleDeleteStation()")
handle_stations = between(wp, "void WebPortal::handleStations() {", "void WebPortal::handleAddStation()")
set_np = between(au, "void AudioPlayer::setNowPlaying(", "bool AudioPlayer::getNowPlaying(")
execute = between(mn, "uint8_t executeRtcpBrowser(const rtcp_browser::Command& command) {", "// ====")
reject = between(mn, "uint8_t rejectBrowsePickUrl(", "uint8_t executeRtcpBrowser(")
take = between(rb, "bool RemoteBrowserService::takePlay(", "bool RemoteBrowserService::urlTooLong(")
copy_cat = between(cat, "bool copyText(", "}  // namespace")
copy_fc = between(fc, "bool copyMenuText(", "}  // namespace")
copy_d9 = between(d9, "bool copyText(", "bool same(")

NO_RAW_NAME_COPY = "strncpy(%s"

checks = {
    "helper headers exist": all((root / "src" / f).exists() for f in ("station_text.h", "json_text.h")),
    "station_text reuses browse_text (no second sanitizer)":
        '#include "browse_text.h"' in stt and "browse_text::sanitize(" in stt and "validWidth" not in stt,
    "json_text reuses browse_text::validWidth":
        "browse_text::validWidth(" in jt,
    # ingress
    "addStation cleans the name": "station_text::copyName(station.name" in add_station
        and "strncpy(station.name" not in add_station,
    "addStation rejects a bad URL before storing":
        "station_text::urlAcceptable(url)" in add_station
        and add_station.index("urlAcceptable") < add_station.index("station_text::copyName"),
    "addToRecent cleans the name": "station_text::copyName(recentRecords()[0].name" in add_recent
        and "strncpy(recentRecords()[0].name" not in add_recent,
    "addToFavorites cleans the name":
        "station_text::copyName(favoriteRecords()[favoriteCount].name" in add_fav
        and "strncpy(favoriteRecords()[favoriteCount].name" not in add_fav,
    "portal rejects a bad URL with the existing error path":
        "station_text::urlAcceptable(url.c_str(), url.length())" in handle_add
        and "sendJsonError(400" in handle_add.split("station_text::urlAcceptable")[1].split(";")[0] + ";",
    "portal cleans the name before addStation":
        "station_text::copyName(cleanName" in handle_add
        and handle_add.index("copyName") < handle_add.index("stationManager.addStation(")
        and "stationManager.addStation(name.c_str()" not in handle_add,
    # load
    "saved stations are cleaned at load": "station_text::cleanStoredName(stationRecords()[i].name)" in load_saved,
    "recent records are cleaned at load": "station_text::cleanStoredName(recentRecords()[i].name)" in load_lists,
    "favorites are cleaned at load": "station_text::cleanStoredName(favoriteRecords()[i].name)" in load_lists,
    "load does not write to NVS":
        "putString" not in load_saved and "putString" not in load_lists
        and "saveStationsToPrefs();" not in load_saved and "saveRecentToPrefs();" not in load_lists
        and "saveFavoritesToPrefs();" not in load_lists,
    # stream title
    "ICY title is cleaned before it is stored":
        "browse_text::sanitize(" in set_np and "strncpy(nowPlaying" not in set_np,
    # Browse pick
    "Browse pick checks unusable URLs before takePlay and deferPlay":
        "urlUnusable(command.token)" in execute
        and execute.index("urlUnusable(") < execute.index("takePlay(") < execute.index("audioPlayer.deferPlay("),
    "Browse unusable URL reuses the rejection path with its own log line":
        "return rejectBrowsePickUrl(command, 0, true)" in execute
        and "URL contains a space, control or non-ASCII character" in reject
        and "Pick rejected: URL length %u" in reject
        and "browsePickResolved(command.request, DeferredPlayStatus::failed)" in reject,
    "takePlay refuses an unusable URL": "station_text::urlAcceptable(results_[token].url)" in take,
    "urlUnusable is declared": "bool urlUnusable(uint16_t token) const;" in rbh,
    # encoders
    "catalog encoder sanitizes first": "browse_text::sanitize(source, clean" in copy_cat
        and copy_cat.index("sanitize(") < copy_cat.index("validText("),
    "menu encoder sanitizes first": "browse_text::sanitize(source, clean" in copy_fc
        and copy_fc.index("sanitize(") < copy_fc.index("validMenuText("),
    "now-playing encoder sanitizes first": "browse_text::sanitize(source" in copy_d9
        and copy_d9.index("sanitize(") < copy_d9.index("validUtf8Text("),
    "encoders keep their wire limit": "kNameBytes + 1" in copy_cat and "kMenuNameBytes + 1" in copy_fc,
    # JSON
    "station list JSON goes through json_text":
        "jsonText(s->name)" in handle_stations and "jsonText(s->url)" in handle_stations
        and 'o["name"]=s->name' not in handle_stations,
    "json_text escapes control characters as \\u00XX":
        '"\\\\u00"' in jt and "c < 0x20 || c == 0x7f" in jt,
    "other JSON text fields use jsonText":
        'doc["ssid"] = jsonText(' in wp and 'doc["stream_url"] = jsonText(' in wp
        and 'doc["wifi_ssid"] = jsonText(' in wp,
    # unchanged on purpose
    "buffers are not enlarged": "#define MAX_NAME_LENGTH     64" in read("config.h")
        and "#define MAX_URL_LENGTH      256" in read("config.h"),
    "browse_text keeps its signature for existing callers":
        "const char* placeholder, bool* cut = nullptr)" in bt,
}

failed = [name for name, ok in checks.items() if not ok]
for name in failed:
    print("FAIL:", name)
if failed:
    sys.exit(1)
print("input text sanitize policy: %d checks passed" % len(checks))
