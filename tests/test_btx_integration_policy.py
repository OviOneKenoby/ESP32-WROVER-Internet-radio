from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
server = (root / "src/rtcp_server.cpp").read_text(encoding="utf-8")
header = (root / "src/rtcp_server.h").read_text(encoding="utf-8")
audio = (root / "src/audio.cpp").read_text(encoding="utf-8")
main = (root / "src/main.cpp").read_text(encoding="utf-8")
vectors = json.loads((root / "tests/btx_v1_vectors.json").read_text(encoding="utf-8"))

checks = 0
def check(value, message):
    global checks
    checks += 1
    assert value, message

check('kCapabilityResponse = 12' in (root / 'src/rtcp_btx.h').read_text(), 'ID 12')
check('kStateRequest = 13' in (root / 'src/rtcp_btx.h').read_text(), 'ID 13')
check('kStateResponse = 14' in (root / 'src/rtcp_btx.h').read_text(), 'ID 14')
check('kCommandRequest = 15' in (root / 'src/rtcp_btx.h').read_text(), 'ID 15')
check('btx_session_.capabilityRequest()' in server, 'second-ID-3 negotiation')
check('btx_session_.observeNonCapability()' in server, 'probe window locking')
check('btx_session_.clear()' in server and 'btx_mailbox_.clear()' in server,
      'session and mailbox teardown')
check('btx_mailbox_.put(command)' in server and 'serviceBluetooth()' in server,
      'bounded command ownership')
check('bluetooth_owner_(command)' in server, 'main-loop owner callback')
check('rtcpServer.setBluetoothOwner(executeRtcpBluetooth)' in main,
      'owner registered by application')
check('audioPlayer.executeBluetoothRtcp(command)' in main,
      'owner executes through AudioPlayer')
check('sendAvrcCommandChecked(ESP_AVRC_PT_CMD_PLAY)' in audio and
      'sendAvrcCommandChecked(ESP_AVRC_PT_CMD_PAUSE)' in audio and
      'sendAvrcCommandChecked(ESP_AVRC_PT_CMD_BACKWARD)' in audio and
      'sendAvrcCommandChecked(ESP_AVRC_PT_CMD_FORWARD)' in audio and
      'avrcTransactionLabel = 5' in audio,
      'checked collision-free AVRCP operations')
check('set_on_connection_state_changed' in audio and 'set_on_audio_state_changed' in audio,
      'authoritative callbacks registered')
check('Metadata fields updated' in audio and 'Metadata: %s' not in audio,
      'new path does not log metadata payload')
check('new ' not in (root / 'src/rtcp_btx.cpp').read_text() and
      'malloc' not in (root / 'src/rtcp_btx.cpp').read_text(),
      'codec has no dynamic allocation')
check('sizeof(rtcp_btx::SourceState)' in header and '<= 64' in header,
      'persistent-state budget guarded')
check(vectors['extension_capability_response'] == '0c01003f607f07d00000000000000001',
      'approved capability vector preserved')
check(vectors['bluetooth_state_maximum_plaintext_bytes'] == 255 and
      vectors['bluetooth_state_maximum_authenticated_bytes'] == 299,
      'approved size vectors preserved')
check(vectors['bluetooth_command'].startswith('0f') and len(bytes.fromhex(vectors['bluetooth_command'])) == 19,
      'approved command vector preserved')
check('nowPlaying(projection_.wire' in server,
      'D-009 ID 10/11 path retained')
check('audio.setVolume(percent)' in server,
      'existing volume path retained')

print(f'PASS {checks}/{checks} BTX integration policy checks')
