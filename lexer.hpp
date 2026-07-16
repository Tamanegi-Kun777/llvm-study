#ifndef LEXER_HPP
#define LEXER_HPP

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <list>
#include <string>
#include <vector>
#include "APP.hpp"

enum TokenType
{
  TOK_IDENTIFIER,
  TOK_DIGIT,
  TOK_SYMBOL,
  TOK_INT,
  TOK_RETURN,
  TOK_IF,
  TOK_ELSE,
  TOK_WHILE, // WHILE
  TOK_FOR, // FOR
  TOK_CLASS, // CLASS
  TOK_CHAR,
  TOK_FLOAT,
  TOK_DOUBLE,
  TOK_USING,
  TOK_EOF
};

class Token
{
private:
  TokenType Type;
  std::string TokenString;
  int Number;
  int Line;

public:
  Token(std::string string, TokenType type, int line) : TokenString(string), Type(type), Line(line){
    if(Type == TOK_DIGIT){
      Number = atoi(string.c_str());
    }
    else{
      Number = 0x7fffffff;
    }
  };

  ~Token(){};

  TokenType getTokenType(){return Type;};

  std::string getTokenString(){return TokenString;};

  int getNumberValue(){return Number;};

  int getLine(){return Line;};
};

class TokenStream
{
private:
  std::vector<Token*> Tokens;
  int CurIndex;

public:
  TokenStream() : CurIndex(0){};
  ~TokenStream();

  bool ungetToken(int Times=1);
  bool getNextToken();
  bool pushToken(Token *token){
    Tokens.push_back(token);
    return true;
  }
  Token getToken();

  TokenType getCurType(){return Tokens[CurIndex]->getTokenType();};

  std::string getCurString(){return Tokens[CurIndex]->getTokenString();};

  int getCurNumVal(){return Tokens[CurIndex]->getNumberValue();};

  int getCurIndex(){return CurIndex;};

  bool applyTokenIndex(int index){CurIndex = index; return true;}
  bool printTokens();
};



TokenStream *LexicalAnalysis(std::string input_filename);

#endif
