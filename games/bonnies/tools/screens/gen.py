"""gen.py: one step list -> a port script (ms timeline) and an origrun script (waits). Steps: ('shot',n) ('click',x,y,[d])
('key',portkey,vk,[d]) ('type',text) ('wait',ms)."""
VK = {c: 0x41 + i for i, c in enumerate('abcdefghijklmnopqrstuvwxyz')}
def gen(steps, t0, pre):
    port, orig, t = [], [], t0
    for s in steps:
        k = s[0]
        if k == 'shot':
            port.append(f"{t} screenshot @DIR@/{pre}{s[1]}.png"); orig.append(f"shot {pre}{s[1]}"); t += 10
        elif k == 'click':
            d = s[3] if len(s) > 3 else 1200
            port.append(f"{t} click {s[1]} {s[2]}"); orig.append(f"click {s[1]} {s[2]}"); orig.append(f"wait {d}"); t += d
        elif k == 'key':
            d = s[3] if len(s) > 3 else 1200
            port.append(f"{t} key {s[1]}"); orig.append(f"key {s[2]}"); orig.append(f"wait {d}"); t += d
        elif k == 'type':
            for c in s[1]:
                port.append(f"{t} char {c}"); orig.append(f"key {VK[c]:#x}"); orig.append("wait 150"); t += 150
        elif k == 'wait':
            orig.append(f"wait {s[1]}"); t += s[1]
    return port, orig, t
