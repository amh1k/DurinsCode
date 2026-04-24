#ifndef durin_lexer_h
#define durin_lexer_h
#include <string>

enum TokenType
{
    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    // Keywords
    TOKEN_ROOM,
    TOKEN_ACTION,
    TOKEN_ITEM,
    TOKEN_NPC,
    TOKEN_EXIT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_PRINT,
    TOKEN_REMOVE,
    TOKEN_PLAYER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_DESCRIPTION,
    TOKEN_CURRENT_ROOM,

    // Context-sensitive attribute tokens
    TOKEN_PLAYER_ATTR, // identifier after "player."
    TOKEN_ROOM_ATTR,   // identifier after roomName.  (future use)

    // Punctuation
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_DOT,
    TOKEN_SEMICOLON,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,

    // Operators
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_PLUS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_AND_AND,
    TOKEN_OR_OR,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MINUS_EQUAL,

    // Special
    TOKEN_ERROR,
    TOKEN_EOF,
};

struct Token
{
    TokenType type;
    const char *start;
    int length;
    int line;
    int column;
};

void initLexer(const char *source);
Token scanToken();

#endif