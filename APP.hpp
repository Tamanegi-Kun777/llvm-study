#ifndef _APP_H
#define _APP_H   // ← この行が抜けている
#define SAFE_DELETE(x){delete x; x=NULL;}
#define SAFE_DELETEA(x){delete[]x; x=NULL;}

#endif