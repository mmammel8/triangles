// AI.h : header file
#if !defined(AI_H_ED0AF7A2_D99FB5A08F5D_INCLUDED)
#define AI_H_ED0AF7A2_D99FB5A08F5D_INCLUDED

#define MAXMOVE 256

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

class Node 
{
// Attributes
public:
	int move; //move made to get here
	double N, Q;
	Node *parent;
	Node *child[MAXMOVE];
	int nchildren, nmoves;
	int depth;
	int moveQueue[MAXMOVE];
	
// Construction
public: 
	Node();
	virtual ~Node();	
};

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
	Tboard *aiBoard, *mcBoard;
	double bscr;
	int bmove;
	bool halt;
		
protected:
	const int MAXNODE = 8388608/4; //32768; //max # nodes	
	int MAXDEPTH1 = 12; //limit depth of search in threat
	int MAXDEPTH2 = 6; //regular depth of search
	const int MAXBREADTH = 512;	
	const int MAXTREE = 16; //depth of UCT tree
	const double UTIMELIMIT = 10.0;	
	bool threat[MAXMOVE]; //(MAXLINES)
	int maxDepth,nthreat,count;
	unsigned int xr,yr,zr,cr;
	std::unordered_map<unsigned int, double> zmap; 
	Node *root, *narray;
	int nodeCt, depth, bnodes;
	double Cp;	

// Operations
public:
	unsigned int JKISS();
	double MinValue(double alpha, double beta, int depth);
	double MaxValue(double alpha, double beta, int depth);
	void ABSearch(int nmove);
	double get_playout();
	int go(Tboard *board0);
	int DefaultPolicy();
	void Backup(Node *v, double delta);
	Node* SelectChild(Node *v, double c);
	Node* Expand(Node *v, int l1);
	Node* TreePolicy(Node *v);
	int UCTSearch();
	
};
#endif // !defined
