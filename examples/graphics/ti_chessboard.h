/*
   Chessboard lib for TI85, TI86 and VZ200 (128x64 pixel screen)
   By Stefano Bodrato - 13/08/2001
*/

#include <graphics.h>
#include <games.h>

#define P_BLACK 1
#define P_WHITE 0

#define P_PAWN 5
#define P_ROOK 0
#define P_KNIGHT 1
#define P_BISHOP 2
#define P_QUEEN 4
#define P_KING 3


extern char pieces[];

#asm
._pieces

; White Pieces

 defb    9,10
 defb    @00000000, @00000000 ; Rook
 defb    @01111111, @00000000
 defb    @01010101, @00000000
 defb    @01010101, @00000000
 defb    @01000001, @00000000
 defb    @00100010, @00000000
 defb    @00100010, @00000000
 defb    @00100010, @00000000
 defb    @00111110, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Rook mask
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Knight
 defb    @00011100, @00000000
 defb    @00100010, @00000000
 defb    @01000001, @00000000
 defb    @01001111, @00000000
 defb    @01001000, @00000000
 defb    @00100110, @00000000
 defb    @01000010, @00000000
 defb    @01111110, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Knight mask
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111000, @00000000
 defb    @00111110, @00000000
 defb    @01111110, @00000000
 defb    @01111110, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Bishop
 defb    @00001000, @00000000
 defb    @00010100, @00000000
 defb    @00100010, @00000000
 defb    @01000100, @00000000
 defb    @01001011, @00000000
 defb    @01001101, @00000000
 defb    @01000001, @00000000
 defb    @01111111, @00000000
 defb    @00000000, @00000000
 
 defb    9,10
 defb    @00000000, @00000000 ; Bishop mask
 defb    @00001000, @00000000
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @01111110, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00000000, @00000000
 
 defb    9,10
 defb    @00000000, @00000000 ; King
 defb    @00011100, @00000000
 defb    @00010100, @00000000
 defb    @01110111, @00000000
 defb    @01000001, @00000000
 defb    @01110111, @00000000
 defb    @00010100, @00000000
 defb    @00100010, @00000000
 defb    @01111111, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; King mask
 defb    @00011100, @00000000
 defb    @00011100, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @01111111, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Queen
 defb    @01001001, @00000000
 defb    @10110110, @10000000
 defb    @10010100, @10000000
 defb    @10001000, @10000000
 defb    @01000001, @00000000
 defb    @01000001, @00000000
 defb    @01000001, @00000000
 defb    @00111110, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Queen mask
 defb    @01001001, @00000000
 defb    @11111111, @10000000
 defb    @11111111, @10000000
 defb    @11111111, @10000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00111110, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Pawn
 defb    @00011100, @00000000
 defb    @00100010, @00000000
 defb    @00100010, @00000000
 defb    @00010100, @00000000
 defb    @00100010, @00000000
 defb    @01000001, @00000000
 defb    @01000001, @00000000
 defb    @01111111, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Pawn mask
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00000000, @00000000


; Black Pieces

 defb    9,10
 defb    @00000000, @00000000 ; Rook
 defb    @00000000, @00000000
 defb    @00101010, @00000000
 defb    @00101010, @00000000
 defb    @00111110, @00000000
 defb    @00011100, @00000000
 defb    @00011100, @00000000
 defb    @00011100, @00000000
 defb    @00000000, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Rook mask
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Knight
 defb    @00000000, @00000000
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @00110000, @00000000
 defb    @00110000, @00000000
 defb    @00011000, @00000000
 defb    @00111100, @00000000
 defb    @00000000, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Knight mask
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111000, @00000000
 defb    @00111110, @00000000
 defb    @01111110, @00000000
 defb    @01111110, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Bishop
 defb    @00000000, @00000000
 defb    @00001000, @00000000
 defb    @00011100, @00000000
 defb    @00111000, @00000000
 defb    @00110010, @00000000
 defb    @00110110, @00000000
 defb    @00111110, @00000000
 defb    @00000000, @00000000
 defb    @00000000, @00000000
 
 defb    9,10
 defb    @00000000, @00000000 ; Bishop mask
 defb    @00001000, @00000000
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @01111110, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; King
 defb    @00000000, @00000000
 defb    @00001000, @00000000
 defb    @00001000, @00000000
 defb    @00111110, @00000000
 defb    @00001000, @00000000
 defb    @00001000, @00000000
 defb    @00011100, @00000000
 defb    @00000000, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; King mask
 defb    @00011100, @00000000
 defb    @00011100, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @01111111, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Queen
 defb    @00000000, @00000000
 defb    @01001001, @00000000
 defb    @01101011, @00000000
 defb    @01110111, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00000000, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Queen mask
 defb    @01001001, @00000000
 defb    @11111111, @10000000
 defb    @11111111, @10000000
 defb    @11111111, @10000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00111110, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Pawn
 defb    @00000000, @00000000
 defb    @00011100, @00000000
 defb    @00011100, @00000000
 defb    @00001000, @00000000
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00000000, @00000000
 defb    @00000000, @00000000

 defb    9,10
 defb    @00000000, @00000000 ; Pawn mask
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @00111110, @00000000
 defb    @00011100, @00000000
 defb    @00111110, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @01111111, @00000000
 defb    @00000000, @00000000

#endasm

PutPiece (int x, int y, int piece,int b_w)
{
  putsprite(spr_and,6+10*x+5*y,5+5*y,pieces+264*b_w+piece*44 + 22);
  putsprite(spr_or,6+10*x+5*y,5+5*y,pieces+264*b_w+piece*44);
}


DrawBoard()
{

  int     x,y,z,a,b;

  clg ();

  for (x=1 ; x!=43; x++)
  {
    draw(x,x+9,81+x,x+9);
  }

  for (x=0 ; x!=8; x++)
  {
    for (y=0 ; y!=8; y++)
    {
      if (!((x+y) & 1))
      {
	  for (z=0 ; z!=5; z++)
	  {
	    a=3+10*x+5*y+z;
	    b=11+5*y+z;
	    undraw(a,b,a+9,b);
	  }
      }
    }
  }
}

