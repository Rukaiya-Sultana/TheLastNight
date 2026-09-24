import io

p = 'Game.cpp'
lines = io.open(p, encoding='utf-8', newline='').readlines()

start = None
for i, ln in enumerate(lines):
    if 'dev diagnostics' in ln:
        start = i
        break
assert start is not None, 'diag start not found'

end = None
for j in range(start, min(start + 20, len(lines))):
    if lines[j].rstrip('\r\n') == '        }':
        end = j
        break
assert end is not None, 'diag end not found'

print('removing lines', start + 1, 'to', end + 1)
del lines[start:end + 1]
io.open(p, 'w', encoding='utf-8', newline='').write(''.join(lines))
print('diag removed OK')
