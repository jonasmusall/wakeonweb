from typing import List, Tuple
import os
import re


ignore = [
  'web/t/index.html',
  'web/state.css'
]
binary = [
]
bytesPerLine = 20


reDefineName = re.compile(r'[/\.]')
reLinebreak = re.compile(r'\r?\n\s*')


files: List[Tuple[str, bool]] = []
for root, dirs, filenames in os.walk('web'):
  for filename in filenames:
    path = os.path.join(root, filename)
    if path not in ignore:
      files.append((path, path in binary))


stringContents: List[Tuple[str, str]] = []
byteContents: List[Tuple[str, bytes]] = []
for path, bin in files:
  defineName = reDefineName.sub('_', path.upper())
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


with open('web.c', 'w') as f:
  print('#include <pgmspace.h>', file=f)

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
