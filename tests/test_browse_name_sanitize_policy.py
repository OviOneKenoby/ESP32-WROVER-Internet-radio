from pathlib import Path

root = Path(__file__).resolve().parents[1]
rb = (root / "src/remote_browser.cpp").read_text(encoding="utf-8")
rbh = (root / "src/remote_browser.h").read_text(encoding="utf-8")
hdr = root / "src/browse_text.h"


def between(text, start, end):
    s = text.index(start)
    return text[s:text.index(end, s)]


parse = between(rb, "bool RemoteBrowserService::parseJob() {", "void RemoteBrowserService::poll(")
checks = {
    "browse_text.h exists": hdr.exists(),
    "remote_browser.cpp includes it": '#include "browse_text.h"' in rb,
    "country names are sanitized at parse time":
        "browse_text::sanitize(name, countries_[country_count_].name, sizeof countries_[0].name" in parse,
    "genre names are sanitized at parse time":
        "browse_text::sanitize(name, genres_[genre_count_++].name, sizeof genres_[0].name" in parse,
    "result names are sanitized at parse time":
        "browse_text::sanitize(name, result.name, sizeof result.name" in parse,
    "no raw strlcpy of a name field is left in parseJob":
        "strlcpy(countries_" not in parse and "strlcpy(genres_" not in parse and
        "strlcpy(result.name" not in parse,
    "all three parse sites use a placeholder":
        parse.count("browse_text::sanitize(") == 3 and parse.count('"(unknown)"') == 2 and
        parse.count('"(no name)"') == 1,
    "url logic of the previous fix is unchanged":
        "urlFitsBuffer(urlLength, sizeof result.url)" in parse and "result.url_length" in parse,
    "buffers were not enlarged":
        "char name[MAX_NAME_LENGTH]; char url[MAX_URL_LENGTH]" in rbh and
        "struct Country { char name[BROWSE_COUNTRY_NAME_LEN]; };" in rbh and
        "struct Genre { char name[BROWSE_TAG_NAME_LEN]; };" in rbh,
}
failed = [name for name, ok in checks.items() if not ok]
assert not failed, failed
print("browse name sanitize policy: PASS (%d checks)" % len(checks))
