# utf16.py <text .reg>: the file as origrun --reg wants it (UTF-16 with a byte order mark, CRLF lines), on stdout
import sys
text = open(sys.argv[1], encoding='utf-8-sig').read().replace('\r\n', '\n').replace('\n', '\r\n')
sys.stdout.buffer.write(b'\xff\xfe' + text.encode('utf-16-le'))
