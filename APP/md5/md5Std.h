#ifndef _MD5_STD_H
#define _MD5_STD_H

#define F(x,y,z) ((x & y) | (~x & z))  
#define G(x,y,z) ((x & z) | (y & ~z))  
#define H(x,y,z) (x^y^z)  
#define I(x,y,z) (y ^ (x | ~z))  
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))  
#define FF(a,b,c,d,x,s,ac) \  
          { \  
          a += F(b,c,d) + x + ac; \  
          a = ROTATE_LEFT(a,s); \  
          a += b; \  
          }  
#define GG(a,b,c,d,x,s,ac) \  
          { \  
          a += G(b,c,d) + x + ac; \  
          a = ROTATE_LEFT(a,s); \  
          a += b; \  
          }  
#define HH(a,b,c,d,x,s,ac) \  
          { \  
          a += H(b,c,d) + x + ac; \  
          a = ROTATE_LEFT(a,s); \  
          a += b; \  
          }  
#define II(a,b,c,d,x,s,ac)\  
          {\  
          a += I(b,c,d) + x + ac;\  
          a = ROTATE_LEFT(a,s);\  
          a += b;\  
          }
typedef struct _md5_std_ctx{  
    unsigned int count[2];  
    unsigned int state[4];  
    unsigned char buffer[64];     
}MD5_STD_CTX,*pMD5_STD_CTX;  

void md5Init(pMD5_STD_CTX context);  
void md5Update(pMD5_STD_CTX context,unsigned char *input,unsigned int inputlen);  
void md5Final(pMD5_STD_CTX context,unsigned char digest[16]);  
void md5Transform(unsigned int state[4],unsigned char block[64]);  
void md5Encode(unsigned char *output,unsigned int *input,unsigned int len);  
void md5Decode(unsigned int *output,unsigned char *input,unsigned int len);

#endif