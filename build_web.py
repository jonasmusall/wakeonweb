from typing import List, Tuple
import re


# entries: (defineName, path, binary)
files: List[Tuple[str, str, bool]] = [
  ('bodyRoot', 'web/index.html', False),
  ('bodyMainCss', 'web/main.css', False),
  ('bodyFaviconSvg', 'web/favicon.svg', False)
]
bytesPerLine = 20


reDefineName = re.compile(r'[/\.]')
reLinebreak = re.compile(r'\r?\n\s*')


stringContents: List[Tuple[str, str]] = []
byteContents: List[Tuple[str, bytes]] = []
for defineName, path, bin in files:
  print(defineName, f'binary={bin}')

  if bin:
    with open(path, 'rb') as f:
      byteContents.append((defineName, f.read()))
  else:
    with open(path, 'r') as f:
      content = f.read()
      content = content.replace('\\', '\\\\').replace('"', '\\"')
      content = reLinebreak.sub('', content)
      stringContents.append((defineName, content))


with open('web.h', 'w') as f:
  for defineName, content in stringContents:
    print(f'const char {defineName}[] PROGMEM = "{content}";', file=f)

  for defineName, content in byteContents:
    byteStrings = [f'0x{b:02x}' for b in content]
    print(f'const char {defineName}[] PROGMEM = {{', file=f)
    for i in range(0, len(byteStrings), bytesPerLine):
      bytesInLine = byteStrings[i : i + bytesPerLine]
      if i < len(byteStrings) - bytesPerLine:
        print(f'    {", ".join(bytesInLine)}', end=',\n', file=f)
      else:
        print(f'    {", ".join(bytesInLine)}}};', file=f)
