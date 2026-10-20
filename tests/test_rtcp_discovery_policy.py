from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / "src/rtcp_discovery.cpp").read_text(encoding="utf-8")
server = (root / "src/rtcp_server.cpp").read_text(encoding="utf-8")
web = (root / "src/web_portal.cpp").read_text(encoding="utf-8")

assert 'MDNS.addService("rtcp", "tcp", 46411)' in source
assert 'pair' not in source.lower()
assert 'psk' not in source.lower()
assert 'discovery_.poll(enabled_ && wifi_connected' in server
assert 'rtcp_mdns_active' in web
assert 'rtcp_mdns_failures' in web
print("RTCP DISCOVERY POLICY 6/6 PASS")
