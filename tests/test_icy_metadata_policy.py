# Source-structure checks for the ICY metadata reader: the firmware must use the
# bounded reader (icy_metadata::consume) for BOTH plain http and https streams, and
# the read timing / HTTP handling of the stream classes must stay as it was.
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parents[1]


def read(name):
    return (root / "src" / name).read_text(encoding="utf-8").replace("\r\n", "\n")


audio = read("audio.cpp")
audio_h = read("audio.h")
plain = read("AudioFileSourcePlainICYStream.cpp")
plain_h = read("AudioFileSourcePlainICYStream.h")
secure = read("AudioFileSourceICYSStream.cpp")
reader = read("icy_metadata.h")

play = audio[audio.index("const bool isSecure"):audio.index("RegisterMetadataCB(metadataCallback")]

checks = {
    "plain http uses the vendored plain class":
        "new AudioFileSourcePlainICYStream()" in play and "new AudioFileSourceICYSStream()" in play
        and "isSecure" in play.split("new AudioFileSourcePlainICYStream()")[0],
    "the library ICY class is no longer used":
        "AudioFileSourceICYStream()" not in audio and "<AudioFileSourceICYStream.h>" not in audio_h
        and '#include "AudioFileSourcePlainICYStream.h"' in audio_h,
    "plain class reads metadata through icy_metadata::consume":
        "icy_metadata::consume(" in plain and "icyBuff" not in plain and "strstr(" not in plain
        and "memmem(" not in plain,
    "https class reads metadata through icy_metadata::consume":
        "icy_metadata::consume(" in secure and "icyBuff" not in secure and "strstr(" not in secure
        and "memmem(" not in secure,
    "both classes treat a stream error as before (return what was read)":
        plain.count("return read;") >= 2 and secure.count("return read;") >= 2,
    "reader terminates the buffer before any string search":
        reader.index("buf[have] = '") < reader.index("std::strstr(") < reader.index("std::strchr("),
    "reader keeps the original buffer size": "256 + 16 + 1" in reader,
    "plain class keeps its 3000 ms wait, https keeps 500 ms":
        "millis() - start < 3000" in plain and "millis() - start < 500" in secure,
    "plain class keeps the HTTP request setup":
        all(s in plain for s in ('http.addHeader("Icy-MetaData", "1");', "http.collectHeaders(hdr, 4);",
                                  "http.setReuse(true);", "HTTPC_FORCE_FOLLOW_REDIRECTS")),
    "plain class does not add TLS or timeouts":
        "setInsecure" not in plain and "setConnectTimeout" not in plain and "setTimeout" not in plain,
    "plain class keeps the library licence header":
        "GNU General Public License" in plain and "Earle F. Philhower" in plain
        and "Earle F. Philhower" in plain_h and "Philhower" in reader,
    "buffer sizes of the audio path are unchanged":
        "char saveURL[128];" in plain_h,
}

failed = [name for name, ok in checks.items() if not ok]
for name in failed:
    print("FAIL:", name)
if failed:
    sys.exit(1)
print("icy metadata policy: %d checks passed" % len(checks))
