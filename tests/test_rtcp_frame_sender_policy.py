from pathlib import Path

root = Path(__file__).resolve().parents[1]
server = (root / "src/rtcp_server.cpp").read_text(encoding="utf-8")
sender = (root / "src/rtcp_frame_sender.h").read_text(encoding="utf-8")

send_application = server.split("bool Server::sendApplication", 1)[1].split(
    "size_t Server::handleApplication", 1
)[0]

checks = {
    "production uses bounded sender": "sendFrameBounded(" in send_application,
    "no nested WiFiClient write": "client_.write(" not in send_application,
    "explicit 500 ms budget": "kFrameSendBudgetMs = 500" in server,
    "raw nonblocking socket send": "lwip_send(" in server and "MSG_DONTWAIT" in server,
    "writable wait is bounded": "lwip_select(" in server,
    "one absolute deadline": "deadline_ms" in sender
    and "deadline_ms =" not in sender,
    "positive progress advances offset": "result.written += progress" in sender,
    "successful prefix is not replayed": "frame + result.written" in sender,
    "retryable socket errors": all(
        token in sender for token in ("EAGAIN", "EWOULDBLOCK", "EINTR")
    ),
    "deadline is terminal": "FrameSendStatus::deadline" in sender
    and "ETIMEDOUT" in sender,
    "partial byte count reaches diagnostics": "responseWrite(send.written" in server,
    "counter advances only after completion":
        send_application.index("if (!send.complete()) return false;")
        < send_application.index("++tx_counter_"),
}

failed = [name for name, passed in checks.items() if not passed]
if failed:
    raise AssertionError("failed: " + ", ".join(failed))

print(f"RTCP bounded frame sender policy: PASS ({len(checks)}/{len(checks)})")
