#!/usr/bin/env python2.4
import sys, os, os.path, select, string, getopt, cmd, traceback, rlcompleter
import cexpr

intro = 'X32 Debugger (c) 2006 Mark Dufour, Sijmen Woutersen, Arjan J.C. van Gemund\nLicensed under the GPL, version 2 (enter ? for help)\n'

# --- python2.2 compatibility
try: sorted
except NameError:
    def sorted(x): 
        l = list(x)
        l.sort()
        return l

# --- target architecture
opcode_name = {0x0100 : 'CNST', 0x0110 : 'ADDR_SP', 0x0120 : 'ADDR_AP', 0x0140 : 'ADDR_LP', 0x0160 : 'ADDR_VR', 0x0180 : 'ADDR_FP', 0x01A0 : 'ADDR_EL', 0x01C0 : 'ADDRG', 0x0310 : 'ADD', 0x0320 : 'BAND', 0x0330 : 'BOR', 0x0340 : 'BXOR', 0x0350 : 'DIV', 0x0360 : 'LSH', 0x0370 : 'MOD', 0x0380 : 'MUL', 0x0390 : 'RSH', 0x03A0 : 'SUB', 0x03D0 : 'DISCARD', 0x04B0 : 'NEG', 0x04C0 : 'BCOM', 0x0500 : 'CALL', 0x0510 : 'SYSCALL', 0x06D0 : 'JUMP', 0x07D0 : 'RET', 0x08D0 : 'INDIR', 0x0910 : 'ARGSTACK', 0x0A00 : 'MOVESTACK', 0x0A10 : 'VARSTACK', 0x0B80 : 'ARG', 0x0C10 : 'EQ', 0x0C20 : 'GT', 0x0C30 : 'GE', 0x0C40 : 'LT', 0x0C50 : 'LE', 0x0C80 : 'NE', 0x0DE0 : 'ASGN', 0x0F00 : 'CV', 0x0F01 : 'CVF', 0x0F02 : 'CVI', 0x0F03 : 'CVP', 0x0F04 : 'CVU', 0x8000 : 'TRAP', 0x8001 : 'HALT'}
name_opcode = dict([(v,k) for k,v in opcode_name.items()])

trap_flag = 0x8000

memory_size, peripheral_start, peripheral_end = 0xfffff, 0x80000000, 0x80001000 # XXX peripheral_end?
opcode_size, ptr_size, int_size, short_size, long_size = 2, 4, 4, 2, 4
pointer, integer, unsigned, void = 1, 2, 4, 5

def decode(instr): 
    opcode = ((instr[0] << 8) + (instr[1] & 0xf0)) & ~trap_flag 

    type_id = instr[1] & 0x0e
    if type_id == 0: type = void
    elif type_id == 12: type = pointer
    else: type = [unsigned, integer][instr[1] & 1]

    if opcode & 0xff00 == name_opcode['CV']: 
        if opcode & 0x10: name = 'CVI'
        else: name = 'CVU'
        opcode = name_opcode[name]

    try: size = {0: 0, 2: 1, 4: short_size, 6: int_size, 8: long_size, 10: long_size, 12: ptr_size}[type_id]
    except: size = 0

    if opcode not in opcode_name: return opcode, 'INVALID', 0, 0
    return opcode, opcode_name[opcode], type, size

def operand_size(mnem, size): 
    if mnem in ['ADDR_LP', 'ADDR_AP', 'ADDR_FP', 'MOVESTACK', 'ARGSTACK', 'VARSTACK']: return int_size
    elif mnem in ['ADDRG', 'EQ', 'GT', 'GE', 'LT', 'LE', 'NE']: return ptr_size 
    elif mnem in ['CNST']: return size
    return 0

def decode_instr(address):
    instr = nub_read(address, 2)

    opcode, opc_name, type, size = decode(instr)

    op_size = operand_size(opc_name, size)
    operand = nub_read(address+2, op_size)

    return instr, opcode, opc_name, size, type, operand, op_size

def address_after(address): # determine pc after executing current instruction
    instr, opcode, opc_name, size, type, operand, op_size = decode_instr(address) 
    
    if opc_name in ['CALL', 'JUMP', 'LT', 'GT', 'EQ', 'NE', 'GE', 'LE']:
        sp = int(to_nub('x\n'), 16)-20
        tos2, tos1 = nub_readw(sp-8, 2)
        target = sum_hex(operand)

        if opc_name == 'CALL':
            if tos1 in address_line:
                return tos1
        elif opc_name == 'JUMP':
            return tos1 
        else:
            if type == integer:
                tos2, tos1 = cexpr.limit(tos2, 'integer', 4), cexpr.limit(tos1, 'integer', 4) # XXX other sizes? cexpr.limit?

            if (opc_name == 'LT' and tos2 < tos1) or \
                 (opc_name == 'GT' and tos2 > tos1) or \
                 (opc_name == 'EQ' and tos2 == tos1) or \
                 (opc_name == 'NE' and tos2 != tos1) or \
                 (opc_name == 'GE' and tos2 >= tos1) or \
                 (opc_name == 'LE' and tos2 <= tos1):
                return target 

    elif opc_name == 'RET':
        state_address = int(to_nub('x\n'), 16)
        fp = nub_word(state_address-4)
        return nub_word(fp-20)

    return address+opcode_size+op_size

def continue_to(target=None): # continue, with optional target address 
    global proc

    while 1:
        if target == None and pc in break_instr: # continue from breakpoint # XXX always use next instr?
            next_addr = address_after(pc)
            continue_step(next_addr)
            if not halt and pc == next_addr and pc not in break_instr: 
                continue_step(target) # continue if no interrupt has intervened, and there is no break point on next instruction
        else:
            continue_step(target) # break point does not have to be restored

        if halt:
            break

        if pc in break_instr: 
            if pc in temp_break: # temp break point: remove it
                del temp_break[pc]
                toggle_breakpoint(pc) 

            if pc in cond_break and pc != target: # conditional break point: evaluate expression
                proc = line_proc[address_line[pc]] 
                result = evaluate_expr(cond_break[pc])[0]
                if result == None: break
                elif not result: 
                    continue

            break

        if pc == target:
            break
    
def continue_step(target=None): # low-level continue: set/reset break points
    if target != None and target not in break_instr:
        nub_break(target)

    old_pc = pc
    nub_continue()

    if not halt:
        update_cpu_state()

        if old_pc in break_instr: 
            nub_break(old_pc)

def toggle_breakpoint(address):
    if not option_textmode and address in address_line:
        if address in break_instr: w.textEdit1.markerDelete(address_line[address]-1, breakpoint_marker)
        else: w.textEdit1.markerAdd(address_line[address]-1, breakpoint_marker)

    if address in temp_break: del temp_break[address]
    if address in cond_break: del cond_break[address]

    if address in break_instr:
        nub_write(address, break_instr[address]) # restore instruction
        del break_instr[address]
    else:
        break_instr[address] = nub_read(address, 2) # save instruction
        if address != pc: # avoid setting on current address
            nub_break(address)

def update_cpu_state(): 
    global sp, pc, el, ap, lp, fp

    sp = int(to_nub('x\n'), 16)-20
    pc, el, ap, lp, fp = nub_readw(sp, 5) 

def local_var_address(address):
    return lp+address 

def parameter_address(address):
    return ap+address

def unassemble(address, lines):
    for l in range(lines):
        instr, opcode, opc_name, size, type, operand, op_size = decode_instr(address) # XXX instr = ..
        type = ' PIFUVB'[type]
        if type in 'PIFUB': type += str(size)

        line = ('%04x  ' % address)+list_hex([opcode >> 8, opcode & 0xff]+operand)+'   '*(4-op_size)+' '+opc_name+type.strip()
        
        if operand: 
            operand = sum_hex(operand)
            if opc_name in ['ADDR_LP', 'ADDR_AP']:
                id = {'ADDR_LP': 'local', 'ADDR_AP': 'parameter'}[opc_name] # XXX
                locals = None
                if proc in local_var:  
                    locals = [x for x in local_var[proc] if local_var[proc][x][0] == operand and local_var[proc][x][2] == id] # XXX
                if locals: 
                    line += ' '+locals.pop()
                else: line += ' $'+hex(operand)[2:]

            elif opc_name == 'ADDRG' and operand in address_proc:
                line += ' '+address_proc[operand]
            elif opc_name == 'ADDRG' and operand in address_global_var: # XXX
                line += ' '+address_global_var[operand]
            else:
                line += ' $'+hex(operand)[2:]

        if address in address_line: line += ' (line %d)' % address_line[address]
        if address in break_instr: 
            if address in cond_break: line += ' (break point: %s)' % cond_break[address]
            else: line += ' (break point)'

        to_scr(line)
        address += opcode_size+op_size

def is_return(address):
    opc_name = decode_instr(address)[2]
    return opc_name == 'RET'

def closest_proc(address): # used by print_callstack below
    min_dist, min_proc = 2**32, None # XXX merge with similar code, cleanup
    for proc, proc_addr in proc_address.items():
        dist = address-proc_addr
        if dist >= 0 and dist < min_dist:
            min_dist, min_proc = dist, proc   
    return min_proc

def print_callstack():
    p, f = pc, fp
    while 1:
        min_proc = closest_proc(p)
        if min_proc not in proc_line:
            break
        to_scr('%s (%s)' % (min_proc, hex(p)))
        p, f = nub_word(f-20), nub_word(f-4)

def out_of_mem(begin, end):
    if begin >= 0 and end >= 0 and begin < memory_size and end <= memory_size: return False
    if begin >= peripheral_start and end >= peripheral_start and begin < peripheral_end and end <= peripheral_end: return False
    return True

# --- debug information
def read_debug_symbols(module):
    for line in file(module+'.dbg'):
        if line.startswith('typedef:'):
            split = line.split(':')
            typedef[split[1].strip()] = split[2].strip()
         
    for line in file(module+'.dbg'):
         split = line.split()
         if split[0] in ['line:', 'function:', 'local:', 'global:', 'parameter:']:
             address = int(split[-1][2:], 16)
           
         if split[0] == 'line:':
             colon = split[1].rfind(':')
             filename, line = split[1][1:colon-1], int(split[1][colon+1:])
             if filename == module+'.c':
                 if address not in address_line or address_line[address] < line: # address can be on multiple lines
                     line_address[line] = address
                     address_line[address] = line

         elif split[0] == 'local:':
             proc, var = split[1].split(':')
             local_var.setdefault(proc, {})[var] = (address, parse_typedesc(line), 'local') 

         elif split[0] == 'function:':
             proc_address[split[1]] = address

         elif split[0] == 'parameter:': # XXX
             proc, var = split[1].split(':')
             local_var.setdefault(proc, {})[var] = (address, parse_typedesc(line), 'parameter') 

         elif split[0] == 'global:':
             global_var[split[1]] = (address, parse_typedesc(line))

    # --- calculate function for each line
    for line, address in line_address.items():
        line_proc[line] = closest_proc(address)

    # --- calculate line following each line
    proc_lines = {}
    for line, proc in line_proc.items():
        proc_lines.setdefault(proc, []).append(line)
    for proc, lines in proc_lines.items():
        lines.sort()
        for i in range(len(lines)-1):
            line_next[lines[i]] = lines[i+1]

def parse_typedesc(line): # '.. <int[4][5]**> ..' -> (2, (4,5), 'integer', 4)
    typedesc = line[line.find('<')+1:line.find('>')]

    ptrcount = typedesc.count('*')
    if ptrcount:
        typedesc = typedesc[:-ptrcount]

    local = None
    if '[' in typedesc:
        dimensions = typedesc[typedesc.find('[')+1:typedesc.rfind(']')].replace('][', ' ')
        local = tuple([int(d) for d in dimensions.split()])
        typedesc = typedesc[:typedesc.find('[')]

    if typedesc not in typedef:
        return 0, None, 'unsigned', 4 # unknown type: use unsigned
    typedesc = typedef[typedesc]

    type = typedesc[:typedesc.find('[')]
    if type == 'void': size = 4 # XXX debug output error
    else: size = int(typedesc[typedesc.find('[')+1: typedesc.find(']')])

    return ptrcount, local, type, size 

def load_debug_information(module):
    global line_address, proc_address, proc_line, line_next, break_instr, temp_break, cond_break, line_proc, local_var, global_var, address_proc, address_line, typedef, address_global_var
    line_address, address_line, proc_address, proc_line, line_next, break_instr, temp_break, cond_break, line_proc, local_var, global_var, typedef = {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {} # XXX split, move

    read_debug_symbols(module) 

    address_proc = dict([(v,k) for k,v in proc_address.items()])

    for a, b in proc_address.items(): 
        if b in address_line:
            proc_line[a] = address_line[b] 

    address_global_var = dict([(b[0],a) for a,b in global_var.items()]) # XXX use proc_address?

# --- communication with nub
incoming, last_char = '', '\n'
def getline(skip):
    global incoming, last_char
    line = ''
    while 1:
        l = poll.poll(1)
        for (o, evt) in l:
            if o == cout.fileno() and evt & select.POLLIN:
                new = os.read(cout.fileno(), 1024)
                incoming += new

        if not option_textmode: 
            app.processEvents()
        if halt:
            return None

        while incoming != '':
            if incoming[0] == '\a': # XXX use different escape sequence?
                if line != '':
                    return line
                pos = incoming.find('\n')
                if pos != -1:
                    line, incoming = incoming[:pos+1], incoming[pos+1:]
                    return line
                break # read more
            else:
                pop, incoming = incoming[0], incoming[1:]
                line += pop
                if not (skip or halt): 
                    to_scr(pop, newline=False)
                    last_char = pop
                if pop == '\n':
                    return line

def wait_dbg(skip=False):
    global halt
    while 1:
        reply = getline(skip)
        #print 'out', reply, repr(reply[0])
        if not reply: return
        elif reply.startswith('\aDBG '): 
            if reply[5:9] == 'halt': 
                halt = 1
            return reply[5:]

def to_nub(text, skip=False):
    #print 'in', text
    cin.write(text)
    cin.flush()
    #print 'dump', 
    getline(True) # loader echoes commands > /dev/null
    return wait_dbg(skip)

def nub_readw(address, nr_of_words):
    words = [sum_hex(nub_read(address+4*i, 4)) for i in range(nr_of_words)]
    return words

def nub_word(address):
    return nub_readw(address, 1)[0]

def sum_hex(bytes):
    return sum([256**(len(bytes)-1-i)*bytes[i] for i in range(len(bytes))])

def list_hex(bytes):
    return ''.join(['%02x ' % b for b in bytes])

def nub_read(address, nr_of_bytes):
    if out_of_mem(address, address+nr_of_bytes): raise Exception('illegal memory address')
    hexstr = to_nub('d %x %x h\n' % (address, address+nr_of_bytes))
    return [int(hexstr[2*i:2*i+2], 16) for i in range(nr_of_bytes)]

def nub_write(address, bytes):
    if not bytes: return
    if out_of_mem(address, address+len(bytes)): raise Exception('illegal memory address')
    hexstr = ''.join(['%02x' % b for b in bytes])
    to_nub('u %x %x h\n' % (address, address+len(bytes)) + hexstr)

def nub_break(address):
    to_nub('b %x\n' % address) 

def nub_continue(): 
    to_nub('c\n')

def nub_reset():
    global halt
    cin.write('r\n')
    cin.flush()
    halt = 0

def nub_start():
    to_nub('n 1\n', skip=True) # enable nub mode, skip x32-term intro
    nub_break(proc_address['main'])
    to_nub('s\n') # start program
    
# --- evaluate C expressions
def evaluate_expr(s, hexadecimal=False):
    return evaluate_exprs(s, hexadecimal)[0]

def evaluate_exprs(s, hexadecimal=False):
    exprs = []
    for t in s.split(): 
        try:
            exprs.append(cexpr.expr(t, proc, hexadecimal=hexadecimal))

        except Exception, msg:
            to_scr('*** '+str(msg)+' ***')
            return [(None, None)]
    return exprs

def var_address(proc, name):
    if proc in local_var and name in local_var[proc]: 
        address = local_var[proc][name][0]
        if local_var[proc][name][2] == 'local': return local_var_address(address)
        else: return parameter_address(address)
    elif name in global_var:
        return global_var[name][0]
    return proc_address[name]

def var_type(proc, name): 
    if proc in local_var and name in local_var[proc]: 
        return local_var[proc][name][1]
    elif name in global_var:
        return global_var[name][1]
    elif name in proc_address:
        return (1, True, 'unsigned', 1) # XXX look out with local
    raise Exception("no such symbol: '%s'" % name)

def hash_line(line):
    if line not in line_address:
        raise Exception("no assembly on this line")
    return line_address[line]

# --- process commands
def process_command(line, script=False):
    global proc, blocked, output_len, assembly_mode

    if not (option_textmode or script):
        if blocked: return
        blocked = 1

    if line.startswith('!'): 
        to_scr(os.popen(line[1:]).read().strip())
        update_ui()
        return

    if pc in address_line: proc = line_proc[address_line[pc]] 

    # --- assignment 
    is_pos = line.find('=')
    if is_pos != -1 and line[:is_pos].strip().replace('_','').isalnum():
        var, expr = line[:is_pos].strip(), line[is_pos+1:]
        try: 
            ptrcount, local, type, size = var_type(proc, var)
            if ptrcount: size = 4
            value = evaluate_expr(expr)[0]
            if value != None:
                nub_write(var_address(proc, var), [(value >> ((size-1-i)*8)) & 255 for i in range(size)])
        except Exception, msg: 
            to_scr('*** '+str(msg)+' ***')
        update_ui()
        return
   
    # --- check command syntax
    cmd = line.split()
    if not cmd:
        update_ui()
        return
    if len(cmd[0]) > 1 or cmd[0] not in '?qxdtuhpwsncbzaloifr': 
        update_ui('*** syntax error ***')
        return
    if halt and cmd[0] not in 'xqr?olz': 
        update_ui('*** halted ***')
        return
    if (cmd[0] in 'ainqtsxzr' and len(cmd) > 1) or (cmd[0] in '?ocfu' and len(cmd) > 2) or (cmd[0] in 'bd' and len(cmd) > 3):
        update_ui('*** too many arguments ***')
        return
    if cmd[0] in 'owf' and len(cmd) < 2:
        update_ui('*** missing arguments(s) ***')
        return
    if cmd[0] == 'l' and len(cmd) > 1 and cmd[1].startswith('#'):
        update_ui("*** cannot use '#' with this command ***")
        return

    # --- evaluate arguments
    if cmd[0] in '?f':
        args = []
    elif cmd[0] == 'b': # cannot evaluate breakpoint condition now, as it depends on context
        if len(cmd) > 1: args = evaluate_exprs(cmd[1])
        else: args = []
    else: 
        args = evaluate_exprs(line[2:], cmd[0] == 'h')

    # --- auto argument
    if cmd[0] in 'uhpdb' and not args:
        args = evaluate_exprs(str(pc), cmd[0] == 'h') # no args: use PC
        
    if args == [(None, None)]: # evaluation error
        update_ui()
        return

    if cmd[0] in 'cdwb' and args and args[0][0] < 0:
        update_ui('*** negative address ***')
        return

    if args:
        address = args[0][0]
        if cmd[0] in 'bwu':
            if out_of_mem(address, address+4):
                update_ui('*** illegal memory address ***')
                return

    if not option_textmode and cmd[0] in 'cns': 
        disable_highlight()
        w.textEdit2.setEnabled(False)

    # --- process individual command
    if cmd[0] == 'b':
        if len(cmd) <= 2 or address not in break_instr:
            toggle_breakpoint(address)
        if len(cmd) > 2: # conditional break point
            cond_break[address] = ' '.join(cmd[2:])
        if address in break_instr:
            to_scr('break point set at '+hex(address))

    elif cmd[0] == 'c':
        if len(cmd) > 1:
           continue_temp_break(address)
        else:
            continue_to()

    elif cmd[0] == 'n':
        if assembly_mode:
            next_address = pc+opcode_size+decode_instr(pc)[6]
        else:
            if pc not in address_line:
                update_ui('*** no current line ***') # XXX deduce line
                return
            line = address_line[pc]
            if line not in line_next:
                update_ui('*** no subsequent line ***')
                return
            next_address = line_address[line_next[line]]

        continue_temp_break(next_address)

    elif cmd[0] == 's':
        if not assembly_mode and pc not in address_line:
            update_ui('*** no current line ***') # XXX deduce line
            return
        while 1:
            address = address_after(pc)
            if is_return(pc) and address not in address_line:
                address = None
            continue_to(address)
            if pc in address_line or halt or assembly_mode:
                break

    elif cmd[0] == 'l':
        if len(cmd) > 1: line = address 
        else: 
            if pc not in address_line:
                update_ui('*** no current line ***') # XXX deduce line
                return
            line = address_line[pc] 

        if line < 1 or line >= len(source):
            update_ui('*** invalid line number ***')
            return

        if option_textmode: print_source(line, output_len)
        else: scroll_to_line(line-1)

    elif cmd[0] == 'i':
        if not break_instr: to_scr('*** no break points ***')
        for (counter, address) in enumerate(sorted(break_instr)):
            line = '%d: ' % counter 
            if address in address_proc: line += address_proc[address] 
            elif address in address_line: line += 'line %d' % address_line[address] 
            else: line += hex(address)
            if address in cond_break: line += ' (condition: %s)' % cond_break[address] 
            to_scr(line)

    elif cmd[0] == 'r':
        for address in break_instr.keys():
            toggle_breakpoint(address)
        nub_reset()
        nub_start()

    elif cmd[0] == 'z':
        assembly_mode = not assembly_mode
        to_scr('assembly level mode: %d' % assembly_mode)

    elif cmd[0] == 'w':
        nub_write(address, [cexpr.limit(arg[0], 'unsigned', 1) for arg in args[1:]]) 
        
    elif cmd[0] in 'ph':
        to_scr(' '.join([arg[1] for arg in args]))

    elif cmd[0] == 'u':
        unassemble(address, output_len)

    elif cmd[0] == 't':
        for address in range(sp-4*output_len, sp, 4):
            to_scr(('%04x  ' % address) + list_hex(nub_read(address, 4)))
          
    elif cmd[0] == 'd':
        begin = address
        if len(args) == 1: end = begin+16*output_len
        else: end = args[1][0]
        if out_of_mem(begin, end): 
            update_ui('*** illegal memory address ***')
            return
        pos = begin
        while pos < end:
            bytes = nub_read(pos, min(end, pos+16)-pos)
            ascii_repr = ''
            for byte in bytes:
                if chr(byte) in string.printable[:-5]: ascii_repr += chr(byte)
                else: ascii_repr += '.'
            to_scr('%04x  ' % pos + list_hex(bytes).ljust(3*16+2) + ascii_repr)
            pos += 16
         
    elif cmd[0] == 'o':
        if address > 0:
            output_len = address
        else:
            to_scr('*** illegal output length ***')

    elif cmd[0] == 'a':
        print_callstack()

    elif cmd[0] == 'x':
        to_scr('PC=%x EL=%x AP=%x SP=%x FP=%x' % (pc, el, ap, sp, fp))

    elif cmd[0] == 'f':
        if not os.path.isfile(cmd[1]):
            update_ui("*** no such file: '%s' ***" % cmd[1] )
            return
        else:
            for line in file(cmd[1]):
                if ';' in line: line, answer = line[:line.find(';')].strip(), line[line.find(';')+1:].strip()
                else: answer = None
                to_scr('> '+line.rstrip())
                process_command(line, script=True)
                reply = all_chars.split('\n')[-2]
                if answer and reply != answer:
                    to_scr('*** expected: %s ***' % answer)
                    break
            if not (option_textmode or halt):
                to_scr('> ', newline=False)
            return

    elif cmd[0] == 'q':
        command_quit()
        sys.exit()

    elif cmd[0] == '?':
        if len(cmd) > 1:
            if cmd[1] not in command_help:
                update_ui("*** no such command: '%s' ***" % cmd[1])
                return

            to_scr(command_help[cmd[1]][1:-1])
        else:
            for line in help: to_scr(line)

    if halt:
        if cmd[0] not in 'xq?olz': 
            update_ui('*** halted ***')
            return

    elif cmd[0] in 'csnrf':
        update_cpu_state()

    update_ui(script=script, highlight=cmd[0] in 'csnrf')

def command_quit():
    global halt
    pre_halt, halt = halt, 1
    if not pre_halt: nub_reset()
    cin.close()

def continue_temp_break(address): # one-shot break point on subsequent line
    if address not in break_instr: 
        toggle_breakpoint(address)
        temp_break[address] = True
    continue_to(address)

def address_prefix(address):
    if address == pc:
        if address in break_instr: return '*>'
        return '**'
    if address in break_instr: return '->'
    return '  '

def print_source(line, lines):
    for i in range(line, min(line+lines, len(source))):
        prefix = '  '
        if i in line_address: prefix = address_prefix(line_address[i])

        to_scr(prefix+str(i).zfill(4)+': '+source[i-1])

# --- user interface
all_chars = ''
def to_scr(text, newline=True):
    global all_chars
    if text != None:
        text.replace('\r', '')
        if newline: text += '\n'
        all_chars += text
        
        if option_textmode:
            sys.stdout.write(text) 
            sys.stdout.flush()
        else:
            w.textEdit2.moveCursor(qt.QTextEdit.MoveEnd, 0)
            w.textEdit2.insert(text) 

def update_ui(msg=None, script=False, highlight=False):
    global blocked, last_char
    if last_char != '\n':
        to_scr('')
        last_char = '\n'
    if not option_textmode:
        blocked = 0
        w.textEdit2.setEnabled(True)
        w.textEdit2.setFocus()

        w.lineEdit1.setText(hex(pc))
        w.lineEdit1_2.setText(hex(sp))
        w.lineEdit1_3.setText(hex(el))

        if highlight:
            disable_highlight()
            if not halt and pc in address_line:
                highlight_line(address_line[pc])

    if option_textmode and not script and highlight:
        if pc in address_line: print_source(address_line[pc], 1)
        else: to_scr(address_prefix(pc)+hex(pc))

    if msg: to_scr(msg)
    if not (option_textmode or script):
        to_scr('> ', newline=False)

def disable_highlight():
    if high_line != None:
        w.textEdit1.markerDelete(high_line, highlight_marker)

def highlight_line(nr):
    global high_line
    high_line = nr-1
    w.textEdit1.markerAdd(high_line, highlight_marker)
    scroll_to_line(high_line)

def scroll_to_line(line):
    begin = max(line-8,0)
    w.textEdit1.setCursorPosition(begin, 0)
    w.textEdit1.ensureCursorVisible()
    end = min(line+8, w.textEdit1.lines())
    w.textEdit1.setCursorPosition(end, 0)
    w.textEdit1.ensureCursorVisible()

def click_margin(margin, line, state):
    if not halt and not blocked:
        if line+1 in line_address: toggle_breakpoint(line_address[line+1])
        else: to_scr('*** no assembly on this line ***\n> ')
        w.textEdit2.setFocus()

history, history_index = [], 0

def updateline(y, check_hist=True):
    w.textEdit2.setUpdatesEnabled(False)
    w.textEdit2.removeParagraph(y)
    if not check_hist or history_index == len(history): w.textEdit2.append('> ')
    else: w.textEdit2.append('> '+history[history_index])
    w.textEdit2.setCursorPosition(y, w.textEdit2.paragraphLength(y))
    w.textEdit2.setUpdatesEnabled(True)
    w.textEdit2.insert('') # XXX

def keypress(e):
    global history_index, blocked
    if blocked: return False # XXX remove? 

    pass_through = False
    y, x = w.textEdit2.getCursorPosition()
    last_y = w.textEdit2.paragraphs()-1
    key, text = e.key(), str(e.text())
    line = str(w.textEdit2.text(last_y))[2:].strip()

    if key == qt.QKeyEvent.Key_Return:
        to_scr('')
        try: process_command(line)
        except SystemExit, e: raise e
        except Exception, e:
           print 'huh'
           traceback.print_exc()
           update_ui('*** internal debugger crash; please report ***') 
        if line:
            history.append(line)
            history_index = len(history) 

    elif key == qt.QKeyEvent.Key_Left:
        if y != last_y or x > 2: pass_through = True
    elif key == qt.QKeyEvent.Key_Backspace:
        if y == last_y and x > 2: pass_through = True
    elif key == qt.QKeyEvent.Key_Delete:
        if y == last_y and x >= 2: pass_through = True
    elif key in [qt.QKeyEvent.Key_Right, qt.QKeyEvent.Key_End]:
        pass_through = True
    elif key == qt.QKeyEvent.Key_Home:
        if y == last_y: w.textEdit2.setCursorPosition(last_y, 2)
        else: pass_through = True

    elif key == qt.QKeyEvent.Key_Up:
        if y == last_y:
            if history_index > 0:
                if history_index == len(history) and line: history.append(line)
                history_index -= 1
                updateline(y)
        else: pass_through = True

    elif key == qt.QKeyEvent.Key_Down:
        if y == last_y:
            if history_index < len(history):
                history_index += 1
                updateline(y)
        else: pass_through = True

    elif len(text) == 1 and text in string.printable:
        #if y != last_y:
        w.textEdit2.setSelection(last_y, w.textEdit2.last_x, last_y, w.textEdit2.last_x) 
        w.textEdit2.setCursorPosition(last_y, w.textEdit2.last_x) 

        #print 'neehee', y, last_y, text
        w.textEdit2.insert(text)

    else:
        pass_through = True
        if e.state() & qt.QKeyEvent.ControlButton:
            if key == qt.QKeyEvent.Key_D: process_command('q')
            elif key == qt.QKeyEvent.Key_V and y != last_y: pass_through = False 
            elif key == qt.QKeyEvent.Key_X: 
                sel = w.textEdit2.getSelection()
                if not sel[0] == sel[2] == last_y: pass_through = False 
            elif key == qt.QKeyEvent.Key_U and y == last_y: updateline(y, False)

    return pass_through

def usage():
    print '''Usage: x32-debug [OPTION]... FILE

 -s --simulation            Use CPU simulator
 -t --textmode              Use text mode interface
'''
    sys.exit()

if __name__ == '__main__':
    # --- parse/check command-line options, arguments
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hst', ['help', 'simulation', 'textmode']) 
    except getopt.GetoptError:
        usage()

    option_simulation = option_textmode = False

    for o, a in opts:
        if o in ['-h', '--help']: usage()
        if o in ['-s', '--simulation']: option_simulation = True
        if o in ['-t', '--textmode']: option_textmode = True

    if len(args) != 1 or not args[0].endswith('.ce'): 
        usage()
    module = args[0][:-3]

    for extension in ['.ce', '.c', '.dbg']:
        if not os.path.isfile(module+extension):
            print 'no such file: '+module+extension
            sys.exit()

    if os.stat(module+'.c')[-1] > os.stat(module+'.ce')[-1]:
        print 'source code more recent than binary' 
        sys.exit()

    source = [line.rstrip() for line in file(module+'.c')]
    help = [line.rstrip() for line in file(os.getenv('X32DBDIR')+'/help')]
    commands = file(os.getenv('X32DBDIR')+'/commands').read().split('::') 
    command_help = dict(zip(commands[1::2], commands[2::2]))

    # --- load debug information
    load_debug_information(module)

    # --- connect to nub
    if option_simulation:
        cin, cout = os.popen2('x32-sim ${X32_PACKAGE_ROOT}/x32/loader/nubloader.ce -base 7d000 -f %s.ce:0' % module)
    else:
        # upload program to x32 first
        if os.system('x32-upload %s.ce -c %s' % (module, os.getenv('X32_PACKAGE_SERIAL'))):
            sys.exit()

        cin, cout = os.popen2('x32-term') 

    poll = select.poll()
    poll.register(cout, select.POLLIN)

    # --- globals
    halt = 0
    output_len, assembly_mode = 8, 0
    cexpr.init(globals())

    # --- setup graphical interface
    if not option_textmode:
        import qt, form1

        app=qt.QApplication(sys.argv)
        w=form1.Form1(fontsize=10)
        app.setMainWidget(w)

        w.textEdit1.setText('\n'.join(source))
        w.textEdit1.setReadOnly(1)
        w.textEdit1.setMarginSensitivity(1, 1)

        w.textEdit2.setText(intro)
        w.textEdit2.moveCursor(qt.QTextEdit.MoveEnd, 0)
        w.textEdit2.last_x = 2
        w.textEdit2.setFocus()

        breakpoint_marker = w.textEdit1.markerDefine(w.textEdit1.RightArrow)
        w.textEdit1.setMarkerForegroundColor(qt.QColor("red"), breakpoint_marker)
        highlight_marker = w.textEdit1.markerDefine(w.textEdit1.Background)
        w.textEdit1.setMarkerBackgroundColor(qt.QColor("yellow"), highlight_marker)

        high_line = None

        form1.keypress = keypress

        qt.QObject.connect(w.textEdit1, qt.SIGNAL("marginClicked(int, int, Qt::ButtonState)"), click_margin)
        qt.QObject.connect(w.textEdit2, qt.SIGNAL("returnPressed()"), process_command)
        qt.QObject.connect(app, qt.SIGNAL("lastWindowClosed()"), command_quit)

    # --- start application and get initial cpu information from nub
    nub_start()
    update_cpu_state()
    update_ui(highlight=True)

    # --- main command/event loop
    if option_textmode:
        print '\n'+intro
        print_source(address_line[pc], 1)

        class Cmd(cmd.Cmd):
            def __init__(self):
                cmd.Cmd.__init__(self)

                completion_dict = proc_address.copy() # tab completion
                completion_dict.update(global_var)
                for p in local_var: 
                    completion_dict.update(local_var[p])
                self.complete = rlcompleter.Completer(completion_dict).complete

                self.prompt = '> '

            def do_EOF(self, args):
                print
                process_command('q')

            def do_help(self, args):
                process_command('? '+args)

            def do_shell(self, args):
                os.system(args)

            def emptyline(self):
                pass
            
            def default(self, line):
                process_command(line)

        try: Cmd().cmdloop()
        except KeyboardInterrupt: 
            print
    else: 
        w.show()
        app.exec_loop()


