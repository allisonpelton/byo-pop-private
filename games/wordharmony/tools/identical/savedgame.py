# savedgame.py <name> <value name> <xml file>: writes <name>.ini (the port's registry.ini) and <name>.reg (the original's
# registry, UTF-16) holding one saved game, so that both programs start with the same one (case.sh's PORTREG and REG).
import sys

name, value, xml_file = sys.argv[1], sys.argv[2], sys.argv[3]
xml = open(xml_file, encoding='utf-8').read().strip()
with open(name + '.ini', 'w', encoding='utf-8', newline='\n') as f:
    f.write('[SOFTWARE\\GameHouse\\WordHarmony]\n%s=sz:%s\n' % (value, xml))
escaped = xml.replace('\\', '\\\\').replace('"', '\\"')
with open(name + '.reg', 'w', encoding='utf-16', newline='') as f:
    f.write('Windows Registry Editor Version 5.00\r\n\r\n[HKEY_CURRENT_USER\\Software\\GameHouse\\WordHarmony]\r\n'
            '"%s"="%s"\r\n' % (value, escaped))
