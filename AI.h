// AI.h : header file
#if !defined(AI_H_ED0AF7A2_D99FB5A08F5D_INCLUDED)
#define AI_H_ED0AF7A2_D99FB5A08F5D_INCLUDED

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <cstddef>
#include <cstring>
#include <chrono>
#include <algorithm>
#include "Tboard.h"

class AI
{
// Construction
public: 
	AI();
	virtual ~AI();
	
// Attributes
public:
	const double PINF = 1.0e+15;
	const double NINF = -1.0e+15;
	const double EPS = 0.00001;
	const int MAXDEPTH1 = 12; //limit depth of search in threat
	const int MAXDEPTH2 = 2; //regular depth of search
	const int MAXBREADTH = 24;		
	Tboard *aiBoard, *mcBoard;
	double bscr;
	int bmove;
	
protected:
	bool threat[256]; //(MAXLINES)
	int maxDepth,nthreat,count;
	unsigned int xr,yr,zr,cr;

// Operations
public:
	unsigned int JKISS();
	double MinValue(double alpha, double beta, int depth);
	double MaxValue(double alpha, double beta, int depth);
	void ABSearch();
	int randMove();
	double get_playout();
	int go(Tboard *board0);
	
};
#endif // !defined
