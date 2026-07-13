#include <vector>
#include <string>
/*
 *   切り出したToken格納クラス
 */
class TokenStream {
    private:
        std::vector<Token*> Tokens;
        int CurIndex;

    public:
        TokenStream():CurIndex(0){}
        ~TokenStream();

        bool ungetToken(int Times=1);
        bool getNextToken();
        bool pushToken(Token * token){
            Tokens.push_back(token);
            return true;
        }
        Token getToken();

        // トークンの種類を取得
        TokenType getCurType() {return Tokens[CurIndex]->getTokenType();}
        // トークンの文字列表現を取得
        std::string getCurString(){return Tokens[CurIndex]->getTokenString();}

        // トークンの数値を取得
        int getCurNumVal(){return Tokens[CurIndex]->getNumberValue();}

        //現在のインデックスを取得
        int getCurIndex(){return CurIndex;}

        //インデックスを指定した値に設定
        bool applyTokenIndex(int index){CurIndex=index; return true;}
        bool printTokens();
};