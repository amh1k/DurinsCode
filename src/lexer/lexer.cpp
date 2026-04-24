#include <iostream>
#include <lexer/lexer.h>
#include <cctype>
#include <cstring>
#include <unordered_map>
#include <set>
struct Lexer
{
    const char *start;
    const char *current;
    int line;
    int column;
    TokenType lastEmitted;
    TokenType lastBeforeDot;
};
Lexer lexer;
static const std::unordered_map<std::string, TokenType> keywords = {
    {"room", TOKEN_ROOM},
    {"action", TOKEN_ACTION},
    {"item", TOKEN_ITEM},
    {"npc", TOKEN_NPC},
    {"exit", TOKEN_EXIT},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"print", TOKEN_PRINT},
    {"remove", TOKEN_REMOVE},
    {"player", TOKEN_PLAYER},
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
    {"description", TOKEN_DESCRIPTION},
    {"current_room", TOKEN_CURRENT_ROOM},
};
void initLexer(const char *source)

{
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;
    lexer.column = 1;
    lexer.lastEmitted = TOKEN_EOF;
    lexer.lastBeforeDot = TOKEN_EOF;
}
// void initKeywordTable()
// {
//     std::string arr[] = {
//         "room", "item", "npc", "exit", "action",
//         "if", "else", "print", "remove", "player",
//         "true", "false", "and", "has_item", "description"};
//     for (auto it : arr)
//         st.insert(it);
// }
static bool isAtEnd()
{
    return *lexer.current == '\0';
}

static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = lexer.start;
    token.length = (int)(lexer.current - lexer.start);
    token.line = lexer.line;
    token.column = lexer.column - token.length;
    lexer.lastEmitted = type;
    return token;
}
static Token errorToken(const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer.line;
    token.column = lexer.column - 1; // column after advance; subtract 1 for the bad char
    lexer.lastEmitted = TOKEN_ERROR;
    return token;
}
static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}
static bool match(char expected)
{
    if (isAtEnd())
        return false;
    if (*lexer.current != expected)
        return false;
    lexer.current++;
    lexer.column++;
    return true;
}
static char advance()
{
    char c = lexer.current[0];
    lexer.current++;
    lexer.column++;
    if (c == '\n')
    {
        lexer.line++;
        lexer.column = 1;
    }
    return c;

    return lexer.current[-1];
}
static char peek()
{
    return *lexer.current;
}
static char peekNext()
{
    if (isAtEnd() || *(lexer.current + 1) == '\0')
        return '\0';
    return lexer.current[1];
}

static void skipWhiteSpace()
{
    for (;;)
    {
        char c = peek();
        switch (c)
        {
        case ' ':
        case '\t':
            advance();
            break;
        case '\r':
            advance();
            if (peek() == '\n') advance(); // consume \r\n as one newline
            break;
        case '\n':
            advance(); // advance() already increments line + resets column
            break;
        case '/':
            if (peekNext() == '/')
            {
                // A comment goes until the end of the line.
                while (peek() != '\n' && !isAtEnd())
                    advance();
            }
            else
            {
                return;
            }
            break;
        default:
            return;
        }
    }
}
static Token number()
{
    while (isDigit(peek()))
        advance();
    if (peek() == '.' && isDigit(peekNext()))
    {
        return errorToken("Floating-point numbers are not supported. Use integers only.");
    }

    return makeToken(TOKEN_NUMBER);
}

static Token string()
{
    while (peek() != '"' && !isAtEnd())
    {
        if (peek() == '\n')
        {
            lexer.line++;
            lexer.column = 1;
        }
        advance();
    }
    if (isAtEnd())
        return errorToken("Unterminated string.");
    advance();
    return makeToken(TOKEN_STRING);
}
static TokenType identifierType(const std::string &str)
{
    auto it = keywords.find(str);
    return (it != keywords.end()) ? it->second : TOKEN_IDENTIFIER;
}
static Token identifier(char c)
{
    std::string temp(1, c);
    while (std::isalpha(peek()) || isDigit(peek()) || peek() == '_')
    {
        temp += advance();
    }
    TokenType baseType = identifierType(temp);
    if (lexer.lastEmitted == TOKEN_DOT)
    {
        if (lexer.lastBeforeDot == TOKEN_PLAYER)
            return makeToken(TOKEN_PLAYER_ATTR);

        if (lexer.lastBeforeDot == TOKEN_ROOM ||
            lexer.lastBeforeDot == TOKEN_IDENTIFIER)
            return makeToken(TOKEN_ROOM_ATTR);
    }

    return makeToken(baseType);
}
Token scanToken()
{
    skipWhiteSpace();
    lexer.start = lexer.current;
    if (isAtEnd())
    {
        return makeToken(TOKEN_EOF);
    }
    char c = advance();
    if (isDigit(c))
    {
        return number();
    };
    if (std::isalpha(c) || c == '_')
    {
        return identifier(c);
    }

    switch (c)
    {

    case '.':
        lexer.lastBeforeDot = lexer.lastEmitted;
        return makeToken(TOKEN_DOT);
    case ';':
        return makeToken(TOKEN_SEMICOLON);
    case '(':
        return makeToken(TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(TOKEN_LEFT_BRACE);
    case '}':
        return makeToken(TOKEN_RIGHT_BRACE);
    case ',':
        return makeToken(TOKEN_COMMA);
    case ':':
        return makeToken(TOKEN_COLON);
    case '&':
        if (match('&'))
        {
            return makeToken(TOKEN_AND_AND);
        }
        return errorToken("Expected '&&', single '&' is not valid.");
    case '|':
        if (match('|'))
            return makeToken(TOKEN_OR_OR);
        return errorToken("Expected '||', single '|' is not valid.");
    case '=':
        return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '+':
        return makeToken(match('=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
    case '-':
        return makeToken(match('=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
    case '>':
        return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '<':
        return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '"':
        return string();
    }
    return errorToken("Unexpected character");
}
