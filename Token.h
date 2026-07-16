#pragma once

#include <string>
#include <cstdlib>
/*
 *   トークン種別
 */

enum TokenType {
    TOK_IDENTIFIER, // 識別子
    TOK_DIGIT, // 数字
    TOK_SYMBOL, // 記号
    TOK_INT, // INT
    TOK_RETURN, // RETURN
    TOK_IF, // IF
    TOK_ELSE, // ELSE
    TOK_WHILE, // WHILE
    TOK_FOR, // FOR
    TOK_CLASS, // CLASS
    TOK_CHAR,
    TOK_FLOAT, // FLOAT
    TOK_DOUBLE, // DOUBLE
    TOK_USING, // USING
    TOK_EOF // EOF
};

/*
 *   個別トークン格納クラス
 */
class Token {
    private:
    TokenType Type;
    std::string TokenString;
    int Number;
    int Line;

    public:
    Token(std::string string, TokenType type, int line)
        : TokenString(string), Type(type), Line(line){
            if(type == TOK_DIGIT)
                Number = atoi(string.c_str());
            else
                Number = 0x7ffffff;
    };
    ~Token(){};

    // トークンの種別を取得
    TokenType getTokenType(){return Type;};

    // トークンの文字列表現を取得
    std::string getTokenString(){return TokenString;};
  
    // トークンの数値を取得（種別が数字である場合に使用）
    int getNumberValue(){return Number;};
  
    // トークンの出現した行数を取得
    int getLine(){return Line;};
};