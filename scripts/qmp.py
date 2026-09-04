#!/usr/bin/env python3
import json, socket, struct, sys, time, zlib, os

SOCK = "/tmp/kikos.qmp"

def qmp_connect():
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(SOCK)
    f = s.makefile("rw")
    while True:
        line = f.readline()
        if not line:
            break
        msg = json.loads(line)
        if "event" in msg:
            continue
        return s, f, msg

def cmd(f, s, d):
    f.write(json.dumps(d) + "\n")
    f.flush()
    while True:
        r = json.loads(f.readline())
        if "return" in r or "error" in r:
            return r

QCODE = {
    ' ': "spc", '.': "dot", ',': "comma", '-': "minus", '=': "equal",
    '[': "bracket_left", ']': "bracket_right", ';': "semicolon",
    "'": "apostrophe", '`': "grave_accent", '\\': "backslash",
    '/': "slash", '\n': "ret",
}
SHIFTED = {
    '!': ('1', True), '@': ('2', True), '#': ('3', True), '$': ('4', True),
    '%': ('5', True), '^': ('6', True), '&': ('7', True), '*': ('8', True),
    '(': ('9', True), ')': ('0', True), '_': ("minus", True), '+': ("equal", True),
    '{': ("bracket_left", True), '}': ("bracket_right", True),
    ':': ("semicolon", True), '"': ("apostrophe", True),
    '~': ("grave_accent", True), '|': ("backslash", True),
    '<': ("comma", True), '>': ("dot", True), '?': ("slash", True),
}

def key_events(qcode, down):
    return [{"type": "key", "data": {"down": down,
             "key": {"type": "qcode", "data": qcode}}}]

def send_key(f, s, qcode):
    cmd(f, s, {"execute": "human-monitor-command",
               "arguments": {"command-line": f"sendkey {qcode}"}})

def type_text(f, s, text):
    for ch in text:
        low = ch.lower()
        need_shift = ch.isupper() or ch in SHIFTED
        if ch in SHIFTED:
            qc, sh = SHIFTED[ch]
            qc = QCODE.get(ch, qc)
            base = qc if isinstance(qc, str) and len(qc) > 1 else (ch.lower() if ch.isalpha() else qc)
            events_down = [{"type": "key", "data": {"down": True, "key": {"type": "qcode", "data": "shift"}}},
                           {"type": "key", "data": {"down": True, "key": {"type": "qcode", "data": qc}}}]
            events_up = [{"type": "key", "data": {"down": False, "key": {"type": "qcode", "data": qc}}},
                         {"type": "key", "data": {"down": False, "key": {"type": "qcode", "data": "shift"}}}]
            cmd(f, s, {"execute": "input-send-event", "arguments": {"events": events_down}})
            cmd(f, s, {"execute": "input-send-event", "arguments": {"events": events_up}})
        elif ch == '\n':
            send_key(f, s, "ret")
        elif ch == ' ':
            send_key(f, s, "spc")
        elif ch.isalnum():
            send_key(f, s, ch.lower())
        else:
            qc = QCODE.get(ch)
            if qc:
                send_key(f, s, qc)

def mouse_move(f, s, dx, dy):
    evs = []
    if dx:
        evs.append({"type": "rel", "data": {"axis": "x", "value": dx}})
    if dy:
        evs.append({"type": "rel", "data": {"axis": "y", "value": dy}})
    if evs:
        cmd(f, s, {"execute": "input-send-event", "arguments": {"events": evs}})

def mouse_btn(f, s, btn, down):
    # Use HMP mouse_button which reliably generates PS/2 events
    btn_num = {"left": 1, "right": 2, "middle": 4}.get(btn, 1)
    cmd(f, s, {"execute": "human-monitor-command",
               "arguments": {"command-line": f"mouse_button {btn_num if down else 0}"}})

def click(f, s, btn="left"):
    mouse_btn(f, s, btn, True)
    time.sleep(0.1)
    mouse_btn(f, s, btn, False)

def move_to(f, s, x, y):
    cur_x = getattr(move_to, "_x", 512)
    cur_y = getattr(move_to, "_y", 384)
    while abs(x - cur_x) > 40 or abs(y - cur_y) > 40:
        dx = max(-40, min(40, x - cur_x))
        dy = max(-40, min(40, y - cur_y))
        mouse_move(f, s, dx, dy)
        cur_x += dx
        cur_y += dy
        time.sleep(0.5)
    mouse_move(f, s, x - cur_x, y - cur_y)
    move_to._x, move_to._y = x, y
    time.sleep(0.5)

def screenshot(f, s, path):
    cmd(f, s, {"execute": "screendump", "arguments": {"filename": path}})
    time.sleep(0.4)

def ppm_to_png(ppm_path, png_path):
    with open(ppm_path, "rb") as fp:
        data = fp.read()
    parts = data.split(b"\n", 3)
    magic = parts[0]
    assert magic == b"P6"
    wh = parts[1].split()
    w, h = int(wh[0]), int(wh[1])
    raw = parts[3]

    def chunk(tag, payload):
        c = tag + payload
        return struct.pack(">I", len(payload)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    rows = b""
    stride = w * 3
    for y in range(h):
        rows += b"\x00" + raw[y * stride:(y + 1) * stride]

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
           + chunk(b"IDAT", zlib.compress(rows, 6)) + chunk(b"IEND", b""))
    with open(png_path, "wb") as fp:
        fp.write(png)

def main():
    args = sys.argv[1:]
    if not args:
        print("usage: qmp.py shot OUT.png | move X Y | click [left|right] | type TEXT | key QCODE | boot")
        return
    s, f, _ = qmp_connect()
    try:
        cmd(f, s, {"execute": "qmp_capabilities"})
        op = args[0]
        if op == "boot":
            cmd(f, s, {"execute": "qmp_capabilities"})
            time.sleep(4)
        elif op == "shot":
            tmp = "/tmp/kikos_shot.ppm"
            screenshot(f, s, tmp)
            ppm_to_png(tmp, args[1])
            os.remove(tmp)
            print("saved", args[1])
        elif op == "move":
            move_to(f, s, int(args[1]), int(args[2]))
        elif op == "click":
            click(f, s, args[1] if len(args) > 1 else "left")
        elif op == "type":
            type_text(f, s, args[1])
        elif op == "key":
            send_key(f, s, args[1])
        else:
            print("unknown op", op)
    finally:
        s.close()

if __name__ == "__main__":
    main()
