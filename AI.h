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
	int MAXDEPTH1 = 10; //limit depth of search in threat
	int MAXDEPTH2 = 6; //regular depth of search
	const int MAXBREADTH = 640;		
	Tboard *aiBoard, *mcBoard;
	double bscr;
	int bmove;
	bool halt;
	
protected:
	bool threat[256]; //(MAXLINES)
	int maxDepth,nthreat,count;
	unsigned int xr,yr,zr,cr;
	std::unordered_map<unsigned int, double> zmap; 

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
