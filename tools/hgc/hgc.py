#!/usr/bin/env python3

from io import open
from lark import Lark
from lark import Transformer

hgcc_parser = Lark(r"""
    hg_proto        : ( rpc )+
                    | empty_statement
    rpc             : "rpc" rpc_name rpc_body
    rpc_name        : ident
    rpc_body        : "{" ( rpc_args | rpc_return | empty_statement )+ "};"
    rpc_args        : "arguments" rpc_args_body
    rpc_args_body   : "{" ( arg | empty_statement )+ "};"

    arg             : type ident ";"

    type            : "double"         -> double
                    | "float"          -> float
                    | "int32"          -> int32
                    | "uint32"         -> uint32
                    | "string"         -> string
                    | "exposed_buffer" -> exposed_buffer
    rpc_return      : "returns" rpc_return_body
    rpc_return_body : "{" ( arg | empty_statement ) "};"
    field_name      : ident
    ident           : CNAME
    empty_statement : ";"*
    COMMENT         : /#+[^\n]*/

    %import common.CNAME
    %import common.WS
    %ignore WS
    %ignore COMMENT

    """, start = 'hg_proto', parser='lalr')


def open_file(filename) :
    try:
        # specify Unicode for Python 2.7
        return open(filename, "r", encoding="utf-8").read()
    except UnicodeDecodeError as ex:
        raise

def parse_file(filename) :
    text = open_file(filename)
    return hgcc_parser.parse(text)

class HGProtoToCpp(Transformer):

    def rpc_name(self, node):
        subtree = node[0]
        print("->" + str(subtree))
        print("==>" + str(subtree.data))
        print("**>" + str(subtree.children))
        return subtree


    def rpc_body(self, leaf):
        return str(leaf)

ast = parse_file("../rpcs.protoh")
#print(ast.pretty())

foo = HGProtoToCpp().transform(ast)
print(foo.pretty())


