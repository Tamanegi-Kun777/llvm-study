#include "lexer.hpp"

TokenStream *LexicalAnalysis(std::string input_filename){
  TokenStream *tokens = new TokenStream();
  std::ifstream ifs;
  std::string cur_line;
  std::string token_str;
  int line_num = 0;
  bool iscomment = false;

  ifs.open(input_filename.c_str(), std::ios::in);
  if(!ifs){
    return NULL;
  }
  while(ifs && getline(ifs, cur_line)){
    char next_char;
    std::string line;
    Token *next_token;
    int index = 0;
    int length = cur_line.length();

    while (index < length){
      next_char = cur_line.at(index++);

      // skip comment
      if(iscomment){
        if( (length - index) < 2 || (cur_line.at(index) != '*') || (cur_line.at(index++) != '/')){
          continue;
        }
        else{
          iscomment = false;
        }
      }

      // EOF
      if(next_char == EOF){
        token_str = EOF;
        next_token = new Token(token_str, TOK_EOF, line_num);
      }
      else if(isspace(next_char)){
        continue;
      }
      // IDENTIFIRE
      else if(isalpha(next_char)){
        token_str += next_char;
        next_char = cur_line.at(index++);
        while(isalnum(next_char)){
          token_str += next_char;
          next_char = cur_line.at(index++);
          if(index == length){
            break;
          }
        }
        index--;

        if(token_str == "int"){
          next_token = new Token(token_str, TOK_INT, line_num);
        }
        else if(token_str == "return"){
          next_token = new Token(token_str, TOK_RETURN, line_num);
        }
        else if(token_str == "if"){
          next_token = new Token(token_str, TOK_IF, line_num);
        }
        else if(token_str == "else"){
          next_token = new Token(token_str, TOK_ELSE, line_num);
        }
        else if(token_str == "while"){
          next_token = new Token(token_str, TOK_WHILE, line_num);
        }
        else if(token_str == "for"){
          next_token = new Token(token_str, TOK_FOR, line_num);
        }
        else if(token_str == "class"){
          next_token = new Token(token_str, TOK_CLASS, line_num);
        }
        else if(token_str == "char"){
          next_token = new Token(token_str, TOK_CHAR, line_num);
        }
        else{
          next_token = new Token(token_str, TOK_IDENTIFIER, line_num);
        }
      }
      // Number
      else if(isdigit(next_char)){
        if(next_char == '0'){
          token_str += next_char;
          next_token = new Token(token_str, TOK_DIGIT, line_num);
        }
        else{
          token_str += next_char;
          next_char = cur_line.at(index++);
          while(isdigit(next_char)){
            token_str += next_char;
            next_char = cur_line.at(index++);
          }
          next_token = new Token(token_str, TOK_DIGIT, line_num);
          index--;
        }
      }
      //　Division, Comment
      else if(next_char == '/'){
        token_str += next_char;
        next_char = cur_line.at(index++);

        // single line comment
        if(next_char == '/'){
          break;
        }
        // multi line comment
        else if(next_char == '*'){
          iscomment = true;
          continue;
        }
        // Division
        else{
          index--;
          next_token = new Token(token_str, TOK_SYMBOL, line_num);
        }
      }
      else{
        if(next_char == '=' || next_char == '<' || next_char == '>' || next_char == '!'){
          if(index < length && cur_line.at(index) == '='){
            token_str += next_char;
            token_str += '=';
            index++;
            next_token = new Token(token_str, TOK_SYMBOL, line_num);
          }
          else if(next_char == '!'){
            fprintf(stderr, "unclear token : %c", next_char);
            SAFE_DELETE(tokens);
            return NULL;
          }
          else{
            token_str += next_char;
            next_token = new Token(token_str, TOK_SYMBOL, line_num);
          }
        }
        else if(next_char == '*' ||
           next_char == '+' ||
           next_char == '-' ||
           next_char == ';' ||
           next_char == ',' ||
           next_char == '(' ||
           next_char == ')' ||
           next_char == '{' ||
           next_char == '}' ||
           next_char == '.' ){
           token_str += next_char;
           next_token = new Token(token_str, TOK_SYMBOL, line_num);
        }
        else{
          fprintf(stderr, "unclear token : %c", next_char);
          SAFE_DELETE(tokens);
          return NULL;
        }
      }

      // Add Tokens
      tokens->pushToken(next_token);
      token_str.clear();
    }

    token_str.clear();
    line_num++;
  }

  // check EOF
  tokens->pushToken(new Token("", TOK_EOF, line_num));

  // close
  ifs.close();
  return tokens;
}
