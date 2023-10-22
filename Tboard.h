// Board.h : header file
#if !defined(BOARD_H_186B_4855_83C5_INCLUDED)
#define BOARD_H_186B_4855_83C5_INCLUDED

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <cstddef>
#include <cstring>

typedef struct
{
	int x, y;
} Point;

bool operator==(const Point& lhs, const Point& rhs);
bool operator!=(const Point& lhs, const Point& rhs);

class Tboard
{
// Construction
public: 
	Tboard();
	virtual ~Tboard();
	
// Attributes
public:
	//constants
	static const int NUMPOINTS = 20; //number of points to place
	static const int CWIDTH = 800; //width of board
	static const int CHEIGHT = 600; //height of board
	static const int BUFFERDIST = 80; //minimum distance between points
	static const int LWIDTH = 3; //line width
	static const int PRADIUS = 8; //point radius
	static const int CLICKRAD = 15; //within this of point to accept click
	static const int MAXLINES = NUMPOINTS * (NUMPOINTS-1) / 2;
	static const int MAXTRIANGLES = NUMPOINTS * (NUMPOINTS-1)  * (NUMPOINTS-2) / 6;	
	
	static Point points[NUMPOINTS]; //list of points on the board
	static int lines[MAXLINES][2]; //list of lines in form of two point #s
	static int triangles[MAXTRIANGLES][3]; //list of triangles in form of triples of point numbers
	static int linenum[NUMPOINTS][NUMPOINTS]; // [p1][p2]  number of line between p1 and p2
	static int trinum[NUMPOINTS][NUMPOINTS][NUMPOINTS]; //[p1][p2][p3]  number of triangle with vert p1 p2 p3
	static int line2triangles[MAXLINES][NUMPOINTS-2]; //lists triangles including this line
	static int line_tri_ct[MAXLINES];
	static bool line_cross[MAXLINES][MAXLINES]; //[l1][l2] if these two intersect
	static int crosslines[MAXLINES][MAXLINES]; // lines that cross this one
	static int line_cross_ct[MAXLINES]; //num cross lines
	static unsigned int zobrist1[MAXLINES][2]; //zobrist hash [lines][2]
	static unsigned int zobrist2[MAXTRIANGLES][2]; //zobrist hash [triangles][2]	
	
	int linect, trict, turn, lastturn;
	bool trimade;
	int score[3];
	unsigned int zhash;
	std::string fnameIn = "Triangles_game.txt";
	int currentPlayer, winner; 
	char linecolor[MAXLINES]; //0, 1, 2 player
	char trifilled[MAXTRIANGLES]; //0,1,2,3 sides
	char tricolor[MAXTRIANGLES]; //0, 1, 2 player		
protected:
	unsigned int xr,yr,zr,cr;
	int numlines, numtri;
	int history[MAXLINES]; //list of lines	

// Operations
public:
	unsigned int JKISS();
	double pt_pt_dist(Point pa, Point pb);
	double pt_line_dist(Point pa, Point pb, Point pc);
	int side(Point pa, Point pb, Point pc);
	bool intersect(Point pa, Point pb, Point pc, Point pd);
	int sign(Point p1, Point p2, Point p3);
	bool point_in_triangle(Point pt, Point v1, Point v2, Point v3);
	bool linelegal(Point pt1, Point pt2);
	bool trilegal(Point pt1, Point pt2, Point pt3);
	void init();
	void clear();
	void rand_start();
	void test_start();
	void copyBd(Tboard* mainBd);
	double get_score();
	int get_winner();
	bool legalline(int l1);
	double get_score1();
	double get_score2(int line1);
	bool rewind();
	bool ffwd();
	bool back();
	bool ahead();
	void make_move(int line1);
	void remove(int line1);
	int generate_moves(int *linelist, double *scorelist);
	int generate_moves(int *linelist);
	int rand_move();
	void playout2();	
	void playout();
	bool Read();
	bool savetodisk();

};
#endif // !defined
