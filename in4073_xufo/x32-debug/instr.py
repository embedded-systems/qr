
d = {}
for line in file('instructions.h'):
    split = line.split()
    d[split[2]] = split[1]
print 'opcode_name = {'+', '.join([str(key)+" : '"+d[key]+"'" for key in sorted(d)]) + '}'
