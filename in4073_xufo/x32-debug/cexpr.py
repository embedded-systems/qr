tokens = ('NUMBER', 'HEX_NUMBER', 'PLUS', 'NAME','LPAREN','RPAREN', 'LBRACKET', 'RBRACKET', 'MINUS', 'LSHIFT', 'RSHIFT', 'MODULO', 'TIMES', 'DIVIDE', 'AND', 'OR', 'XOR', 'LOGICAL_AND', 'LOGICAL_OR', 'LOGICAL_NOT', 'EQUAL', 'NOT_EQUAL', 'LT', 'GT', 'LTE', 'GTE', 'HASH')

t_PLUS, t_MINUS, t_MODULO, t_DIVIDE, t_TIMES, t_AND, t_OR, t_XOR, t_LOGICAL_AND, t_LOGICAL_OR, t_LOGICAL_NOT, t_LPAREN, t_RPAREN, t_LBRACKET, t_RBRACKET, t_LSHIFT, t_RSHIFT, t_EQUAL, t_NOT_EQUAL, t_LT, t_LTE, t_GT, t_GTE, t_HASH = (r'\+', r'\-', r'\%', r'\/', r'\*', r'\&', r'\|', r'\^', r'\&\&', r'\|\|', r'\!', r'\(', r'\)', r'\[', r'\]', r'\<\<', r'\>\>', r'\=\=', r'\!\=', r'<', r'<=', r'>', r'>=', r'\#')

def t_HEX_NUMBER(t): # the order of these defs is important (earlier ones are matched with first) 
    r'0x[0-9a-zA-Z]+'
    t.value = int(t.value[2:], 16)
    return t

def t_NUMBER(t):
    r'[0-9]+'
    t.value = int(t.value)
    return t

t_NAME = r'[a-zA-Z_][a-zA-Z0-9_]*'

t_ignore = " \t"

def t_error(t):
    raise Exception("illegal character '%s'" % t.value[0])

import lex
lex.lex()

precedence = (
    ('left', 'LOGICAL_OR'),
    ('left', 'LOGICAL_AND'),
    ('left', 'OR'),
    ('left', 'XOR'),
    ('left', 'AND'),
    ('left', 'EQUAL', 'NOT_EQUAL'),
    ('left', 'GT', 'GTE', 'LT', 'LTE'),
    ('left', 'LSHIFT', 'RSHIFT'),
    ('left','PLUS','MINUS'),
    ('left','TIMES','DIVIDE','MODULO'),
    ('right','UNARY_MINUS', 'UNARY_PLUS', 'INDIRECT', 'ADDRESS_OF', 'LOGICAL_NOT', 'HASH'),
    ('left', 'LPAREN', 'RPAREN', 'LBRACKET', 'RBRACKET'),
    )

def p_single_expression(p):
    '''expression : expression PLUS expression
    | expression MINUS expression
    | expression MODULO expression
    | expression TIMES expression
    | expression DIVIDE expression
    | expression OR expression
    | expression XOR expression
    | expression AND expression
    | expression LT expression
    | expression LTE expression
    | expression GT expression
    | expression GTE expression
    | expression LSHIFT expression
    | expression RSHIFT expression
    | expression EQUAL expression
    | expression NOT_EQUAL expression'''

    t1, op, t2 = p[1], p[2], p[3]

    if t1[1][0] or t2[1][0]: # pointer types
        if op in ['==', '!=', '>', '>=', '<', '<=']:
            p[0] = rel_eq(op, p[1], p[3])

        else: 
            t1, t2 = multiply_offset(t1, t2), multiply_offset(t2, t1)

            if op == '+': address = t1[0] + t2[0]
            elif op == '-': address = t1[0] - t2[0] 
            else: raise Exception('pointers can only be added/subtracted')

            if t1[1][0]: result_type = t1
            else: result_type = t2 

            p[0] = (limit(address, 'unsigned', 4), result_type[1])

    else: # arithmetic types
        t1, t2 = integral_promotion(t1), integral_promotion(t2)
        t1, t2 = unsigned_promotion(t1, t2)

        if t1[1][2] == 'unsigned': type = 'unsigned'
        else: type = 'integer'

        if op == '+': result = t1[0] + t2[0]
        elif op == '-': result = t1[0] - t2[0]

        elif op == '<<': result = t1[0] << (t2[0] % 32) # machine-dependent if t2[0] not in 0..31
        elif op == '>>': result = t1[0] >> (t2[0] % 32)

        elif op == '*': result = t1[0] * t2[0]
        elif op == '/': result = t1[0] / t2[0]
        elif op == '%': 
            result = t1[0] % t2[0]
            if t1[0] < 0 and t2[0] > 0: result -= t2[0]

        elif op == '&': result = t1[0] & t2[0]
        elif op == '|': result = t1[0] | t2[0]
        elif op == '^': result = t1[0] ^ t2[0]

        elif op in ['==', '!=', '>', '>=', '<', '<=']:
            p[0] = rel_eq(op, t1[0], t2[0])
            return

        p[0] = (limit(result, type, 4), (0, False, type, 4))

def rel_eq(op, t1, t2):
    if op == '==': return (int(t1 == t2), (0, False, 'integer', 4))
    elif op == '!=': return (int(t1 != t2), (0, False, 'integer', 4))
    elif op == '>': return (int(t1 > t2), (0, False, 'integer', 4))
    elif op == '<': return (int(t1 < t2), (0, False, 'integer', 4))
    elif op == '>=': return (int(t1 >= t2), (0, False, 'integer', 4))
    elif op == '<=': return (int(t1 <= t2), (0, False, 'integer', 4))

def integral_promotion(t):
    if t[1][3] < 4:
        return (t[0], (0, False, 'integer', 4))
    return t

def unsigned_promotion(t1, t2): 
    if t1[1][2] == 'unsigned' and t2[1][2] == 'integer':
        return t1, make_unsigned(t2)
    elif t1[1][2] == 'integer' and t2[1][2] == 'unsigned':
        return make_unsigned(t1), t2
    return t1, t2

def make_unsigned(t):
    ptrcount, local, type, size = t[1]
    return (limit(t[0], 'unsigned', size), (ptrcount, local, 'unsigned', size)) 

def multiply_offset(t1, t2):
    offset, ptrcount1 = t1[0], t1[1][0]
    ptrcount2, local2, type2, size2 = t2[1]

    if not ptrcount1:
        if ptrcount2 > 1: offset *= 4
        else: offset *= size2

    return (offset, t1[1])

def p_expression_logical(p): # XXX function calls: lazy evaluation
    '''expression : expression LOGICAL_AND expression
                  | expression LOGICAL_OR expression'''

    if p[2] == '&&': result = p[1][0] and p[3][0]
    else: result = p[1][0] or p[3][0]

    p[0] = (int(bool(result)), (0, False, 'integer', 4))

def p_expression_indir(p):
    'expression : TIMES expression %prec INDIRECT'
    ptrcount, local, type, size = p[2][1]
    ssize = size
    if ptrcount > 1: ssize = 4
    p[0] = (limit(sum_hex(nub_read(p[2][0], ssize)), type, size), (max(ptrcount-1, 0), local, type, size))

def p_expression_addressof(p):
    'expression : AND NAME %prec ADDRESS_OF'
    ptrcount, local, type, size = var_type(proc, p[2])
    p[0] = (var_address(proc, p[2]), (1, local, type, size))

def p_expression_name(p):
    'expression : NAME'
    ptrcount, local, type, size = var_type(proc, p[1])
    if ptrcount: size = 4
    if local:
        p[0] = (var_address(proc, p[1]), (ptrcount, local, type, size))
    else:
        p[0] = (limit(sum_hex(nub_read(var_address(proc, p[1]), size)), type, size), (ptrcount, local, type, size))

def p_expression_number(p):
    '''expression : NUMBER
                  | HEX_NUMBER'''
    p[0] = (p[1], (0, False, 'integer', 4))

def p_expression_plus(p):
    'expression : PLUS expression %prec UNARY_PLUS'
    p[0] = p[2]

def p_expression_minus(p):
    'expression : MINUS expression %prec UNARY_MINUS'
    value, (ptrcount, local, type, size) = integral_promotion(p[2])
    p[0] = (limit(-value, type, size), (ptrcount, local, type, size))

def p_expression_not(p):
    'expression : LOGICAL_NOT expression'
    p[0] = (int(not p[2][0]), (0, False, 'integer', 4))

def p_expression_hash(p):
    'expression : HASH expression'
    p[0] = (hash_line(p[2][0]), (1, False, 'unsigned', 1))

def p_expression_group(p):
    'expression : LPAREN expression RPAREN'
    p[0] = p[2]

def p_expression_index(p):
    'expression : expression LBRACKET expression RBRACKET'
    ptrcount, local, type, size = p[1][1]
    if ptrcount > 1: size = 4
    base_address, offset = p[1][0], p[3][0]*size

    if local: # implicit pointer (e.g. int array[4][4];)
        if len(local) == 1: # one-dimensional indexing
            value = sum_hex(nub_read(base_address+offset, size))
        else: # multi-dimensional indexing: calculate strides
            value = base_address+offset*reduce(lambda x,y:x*y, local[1:])
        p[0] = (value, (ptrcount, local[1:], type, size))
    else: # regular pointer (e.g. int **)
        p[0] = (sum_hex(nub_read(base_address+offset, size)), (ptrcount-1, local, type, size))

def p_error(p):
    raise Exception('syntax error')

import yacc
yacc.yacc()

def limit(n, type, size):
    inner, outer = 2**(8*size-1), 2**(8*size)
    if type == 'integer': return int(((n+inner)%outer)-inner)
    return n%outer

def init(glob):
    globals().update(glob) 

def expr(s, p, hexadecimal=False):
    global proc
    proc = p

    value, (ptrcount, local, type, size) = yacc.parse(s)
    if hexadecimal:
        if value < 0: # hex asked, negative value: make positive
            value = 256**size+value
        s = hex(value).replace('L','').lower()
    else:
        if value < 0: s = '-'+str(abs(value))
        else: s = str(value)
       
    return (value, s)

