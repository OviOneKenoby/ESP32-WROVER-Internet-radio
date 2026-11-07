# JSON built from external text must be valid STRICT JSON for any input bytes:
# every control character below 0x20 and 0x7F is escaped as \u00XX, invalid UTF-8
# becomes U+FFFD. The test builds the documents with the production json_text.h
# and the real ArduinoJson serializer (tests/json_text_dump.cpp) and parses every
# line with Python's strict json parser.
import glob, json, os, shutil, subprocess, sys, tempfile
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
gxx = shutil.which("g++") or r"C:\msys64\ucrt64\bin\g++.exe"
libs = glob.glob(os.path.join(root, ".pio", "libdeps", "*", "ArduinoJson", "src"))
if not os.path.exists(gxx) and not shutil.which("g++"):
    sys.exit("FAIL: g++ not found")
if not libs:
    sys.exit("FAIL: ArduinoJson sources not found under .pio/libdeps")
env = dict(os.environ)
env["PATH"] = os.path.dirname(gxx) + os.pathsep + env.get("PATH", "")
with tempfile.TemporaryDirectory() as work:
    exe = os.path.join(work, "dump.exe")
    c = subprocess.run([gxx, "-std=c++17", "-O1", "-I", os.path.join(root, "src"), "-I", libs[0],
                        os.path.join(root, "tests", "json_text_dump.cpp"), "-o", exe],
                       capture_output=True, text=True, env=env)
    if c.returncode:
        sys.exit("FAIL: compile\n" + c.stderr[-2000:])
    r = subprocess.run([exe], capture_output=True, env=env)
    if r.returncode:
        sys.exit("FAIL: dump exited %d" % r.returncode)
lines = r.stdout.split(b"\n")
if lines and lines[-1] == b"":
    lines.pop()
failures = []
exact = replaced = 0
for number, line in enumerate(lines):
    line = line.rstrip(b"\r")  # Windows text-mode stdout adds a CR
    hex_part, _, doc = line.partition(b"\t")
    raw = bytes.fromhex(hex_part.decode())
    if any(b < 0x20 for b in doc):
        failures.append((number, "raw control byte in the JSON text")); continue
    try:
        value = json.loads(doc.decode("utf-8"), strict=True)
    except Exception as error:
        failures.append((number, "strict parse failed: %s" % error)); continue
    if value.get("codec") != "MP3" or value["name"] != value["url"]:
        failures.append((number, "document shape")); continue
    try:
        expected = raw.decode("utf-8")
    except UnicodeDecodeError:
        if "\ufffd" not in value["name"]:
            failures.append((number, "invalid UTF-8 not replaced")); continue
        replaced += 1
        continue
    if value["name"] != expected:
        failures.append((number, "valid input not preserved exactly")); continue
    exact += 1
if len(lines) < 100000:
    failures.append((-1, "corpus too small: %d" % len(lines)))
if failures:
    for item in failures[:10]:
        print("FAIL:", item)
    sys.exit(1)
print("json strict: %d documents, %d exact round trips, %d with U+FFFD" % (len(lines), exact, replaced))
