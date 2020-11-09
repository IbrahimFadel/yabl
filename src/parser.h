#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#include <string.h>
#include <memory>
#include <vector>
#include <variant>
#include <map>

#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>

class Node;
class Expression_Node;
class Number_Expression_Node;
class Variable_Expression_Node;
class Binary_Expression_Node;
class Call_Expression_Node;
class Prototype_Node;
class Function_Node;
class Variable_Node;
class Return_Node;

using std::unique_ptr;

enum Node_Types
{
    UnknownNode,
    NumberExpressionNode,
    VariableExpressionNode,
    BinaryExpressionNode,
    CallExpressionNode,
    FunctionDeclarationNode,
    VariableDeclarationNode,
    ReturnNode,
    TypeCastNode,
    AssignmentNode,
    IfNode,
    ImportNode
};

enum Variable_Types
{
    type_null = -1,
    type_i64,
    type_i32,
    type_i16,
    type_i8,
    type_float,
    type_double,
    type_bool,
    type_void
};

class Node
{
public:
    Node_Types type;
    std::variant<std::unique_ptr<Expression_Node>, std::unique_ptr<Variable_Node>, std::unique_ptr<Function_Node>, std::unique_ptr<Prototype_Node>, std::unique_ptr<Return_Node>, std::unique_ptr<Call_Expression_Node>> expression_node, variable_node, function_node, prototype_node, return_node, call_node;
};

class Expression_Node
{
public:
    Node_Types type;
    virtual llvm::Value *code_gen() = 0;
};

class Number_Expression_Node : public Expression_Node
{
private:
    Variable_Types variable_type;
    double value;

public:
    Number_Expression_Node(double value, Variable_Types type) : value(value), variable_type(type){};
    virtual llvm::Value *code_gen();
};

class Variable_Expression_Node : public Expression_Node
{
private:
    std::string name;

public:
    Variable_Expression_Node(std::string name) : name(name) {}
    virtual llvm::Value *code_gen();
};

class Binary_Expression_Node : public Expression_Node
{
private:
    std::string op;
    unique_ptr<Expression_Node> lhs, rhs;

public:
    Binary_Expression_Node(std::string op, unique_ptr<Expression_Node> lhs, unique_ptr<Expression_Node> rhs) : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)){};
    virtual llvm::Value *code_gen();
};

class Call_Expression_Node : public Expression_Node
{
private:
    std::string callee;
    std::vector<unique_ptr<Expression_Node>> args;

public:
    Call_Expression_Node(const std::string &callee, std::vector<unique_ptr<Expression_Node>> args) : callee(callee), args(std::move(args)){};
    virtual llvm::Value *code_gen();
};

class Prototype_Node
{
private:
    std::string name;
    std::vector<Variable_Types> arg_types;
    std::vector<std::string> arg_names;
    Variable_Types return_type;

public:
    Prototype_Node(std::string name, std::vector<Variable_Types> arg_types, std::vector<std::string> arg_names, Variable_Types return_type) : name(name), arg_types(arg_types), arg_names(arg_names), return_type(return_type) {}
    llvm::Function *code_gen();
    void create_argument_allocas(llvm::Function *f);
    std::vector<Variable_Types> get_arg_types();
    std::string get_name();
    Variable_Types get_return_type();
};

class Function_Node
{
    std::unique_ptr<Prototype_Node> proto;
    std::vector<std::unique_ptr<Node>> body;
    std::map<std::string, llvm::Value *> variables;
    std::vector<Variable_Types> arg_types;
    llvm::Value *return_value_ptr;
    llvm::BasicBlock *end_bb;

public:
    Function_Node(std::unique_ptr<Prototype_Node> proto,
                  std::vector<std::unique_ptr<Node>> body, std::vector<Variable_Types> types)
        : proto(std::move(proto)), body(std::move(body)), arg_types(types)
    {
    }
    llvm::Function *code_gen();

    void set_variables(std::string name, llvm::Value *var);
    llvm::Value *get_variable(std::string name);
    std::unique_ptr<Prototype_Node> get_proto();
    std::vector<Variable_Types> get_arg_types();
    llvm::Value *get_return_value_ptr();
    llvm::BasicBlock *get_end_bb();
};

class Variable_Node
{
private:
    std::string name;
    Variable_Types type;
    std::unique_ptr<Expression_Node> value;

public:
    Variable_Node(std::string name, Variable_Types type, std::unique_ptr<Expression_Node> value) : name(name), type(type), value(std::move(value)){};
    virtual llvm::Value *code_gen();
};

class Return_Node
{
private:
    std::unique_ptr<Expression_Node> value;

public:
    Return_Node(std::unique_ptr<Expression_Node> value) : value(std::move(value)) {}
    llvm::Value *code_gen();
};

class Type_Cast_Node : public Expression_Node
{
private:
    std::unique_ptr<Expression_Node> value;
    Variable_Types new_type;

public:
    Type_Cast_Node(std::unique_ptr<Expression_Node> value, Variable_Types new_type) : value(std::move(value)), new_type(new_type) {}
    virtual llvm::Value *code_gen();
};

class Assignment_Node : public Expression_Node
{
private:
    std::string name;
    std::unique_ptr<Expression_Node> value;

public:
    Assignment_Node(std::string name, std::unique_ptr<Expression_Node> value) : name(name), value(std::move(value)){};
    llvm::Value *code_gen();
};

class Condition_Expression
{
private:
    std::unique_ptr<Expression_Node> lhs;
    Token_Types op;
    std::unique_ptr<Expression_Node> rhs;

public:
    Condition_Expression(std::unique_ptr<Expression_Node> lhs, Token_Types op, std::unique_ptr<Expression_Node> rhs) : lhs(std::move(lhs)), op(op), rhs(std::move(rhs)){};
    std::unique_ptr<Expression_Node> get_lhs();
    std::unique_ptr<Expression_Node> get_rhs();
    Token_Types get_op();
    llvm::Value *code_gen();
};

class If_Node : public Expression_Node
{
private:
    std::vector<std::unique_ptr<Condition_Expression>> conditions;
    std::vector<Token_Types> condition_seperators;
    std::vector<std::unique_ptr<Node>> then;

public:
    If_Node(std::vector<std::unique_ptr<Condition_Expression>> conditions, std::vector<Token_Types> condition_seperators, std::vector<std::unique_ptr<Node>> then) : conditions(std::move(conditions)), condition_seperators(condition_seperators), then(std::move(then)){};
    llvm::Value *code_gen();
};

class Import_Node : public Expression_Node
{
private:
    std::string path;

public:
    Import_Node(std::string path) : path(path){};
    llvm::Value *code_gen();
};

class String_Expression : public Expression_Node
{
private:
    std::string value;

public:
    String_Expression(std::string value) : value(value){};
    llvm::Value *code_gen();
};

std::vector<std::unique_ptr<Node>>
parse_tokens(std::vector<std::shared_ptr<Token>> tokens);
unique_ptr<Node> parse_token(std::shared_ptr<Token> tokens);

static std::vector<std::shared_ptr<Token>> toks;
static std::shared_ptr<Token> cur_tok;
static int tok_pointer = 0;
static std::map<std::string, int> bin_op_precedence;

// static std::string identifier_string;
// static double num_val;
// static int cur_tok;
static int get_tok_precedence();
static void get_next_token();
// static int get_tok();
static std::unique_ptr<Expression_Node> error(const char *str);
static std::unique_ptr<Prototype_Node> error_p(const char *str);
static Variable_Types type_string_to_variable_type(const char *str);
static Variable_Types token_type_to_variable_type(Token_Types type);
// static std::map<char, int> bin_op_precedence;
// static int get_tok_precedence();

static std::unique_ptr<Expression_Node> parse_number_expression(Variable_Types type);
static std::unique_ptr<Expression_Node> parse_paren_expression();
static std::unique_ptr<Expression_Node> parse_identifier_expression();
static std::unique_ptr<Expression_Node> parse_primary(Variable_Types type = Variable_Types::type_null);
static std::unique_ptr<Expression_Node> parse_bin_op_rhs(int expr_prec, std::unique_ptr<Expression_Node> lhs, Variable_Types type = Variable_Types::type_null);
static std::unique_ptr<Expression_Node> parse_expression(bool needs_semicolon = true, Variable_Types = Variable_Types::type_i32);
static std::vector<std::unique_ptr<Node>> parse_fn_body();
static std::unique_ptr<Prototype_Node> parse_prototype();
static std::unique_ptr<Function_Node> parse_fn_declaration();
static std::unique_ptr<Variable_Node> parse_variable_declaration();
static std::unique_ptr<Return_Node> parse_return_statement();
static std::unique_ptr<Expression_Node> parse_typecast_expression();
static std::unique_ptr<Expression_Node> parse_if();
static std::unique_ptr<Expression_Node> parse_import();
static std::unique_ptr<Expression_Node> parse_string_expression();

#endif