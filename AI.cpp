#include "AI.h"
#include <unordered_map>
#include <sys/time.h>
using namespace std;

/*
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned long long tmpres = 0;
  static int tzflag;
 
  if (NULL != tv)
  {
	GetSystemTimeAsFileTime(&ft);
 
	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;
 
	//converting file time to unix epoch
	tmpres -= DELTA_EPOCH_IN_MICROSECS; 
	tmpres /= 10;  //convert into microseconds
	tv->tv_sec = (long)(tmpres / 1000000UL);
	tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  return 0;
}
*/

Node::Node()
{
	nchildren = 0;
	nchildex = 0;
	N = 0.0;
	Q = 0.0;
	parent = NULL;
	child[0] = NULL;
	filled = false;
}
Node::~Node()
{
	nchildren = 0;
}

AI::AI()
{
	aiBoard = new Tboard();
	mcBoard = new Tboard();
	bmove = -1;
	bscr = 0.0;
	maxDepth = 0;
	nthreat = 0;
	count = 0;
	unsigned int seed = (unsigned)time( NULL );
	srand( seed );
	//srand(1); //**********************
	xr = rand();
	yr = rand();
	cr = rand();
	zr = rand();	
	
}

AI::~AI()
{
	delete aiBoard;
	delete mcBoard;
}

unsigned int AI::JKISS()
{
	unsigned long long tr;
	xr = 314527869 * xr + 1234567;
	yr ^= yr << 5;
	yr ^= yr >> 7;
	yr ^= yr << 22;
	tr = 4294584393ULL * zr + cr;
	cr = tr >> 32;
	zr = tr;

	return xr + yr + zr;
}

double AI::MinValue(double alpha, double beta, int depth)
{
	double minV, v;
	int nmove, mv, lastmv, mvi;
	int movelist[aiBoard->MAXLINES], idx[aiBoard->MAXLINES];
	double scorelist[aiBoard->MAXLINES];
	unordered_map<unsigned int,double>::const_iterator zptr;
	
	count++;
	if (depth > maxDepth)
		maxDepth = depth;
	minV = PINF;
	nmove = aiBoard->generate_moves(movelist, scorelist);
	lastmv = min(nmove, MAXBREADTH);
	for (int k = 0; k < nmove; ++k)
		idx[k] = k;	
	std::sort(idx, idx + nmove, [&scorelist](int i, int j) {return scorelist[i] > scorelist[j];});	
	if (nmove == 0)
	{
		//pass
		if (depth == 1)
		{
			bmove = -1;
			bscr = 0.0;
		}
	}
	else
	{ 
		for (mvi = 0; mvi < lastmv; ++mvi)
		{	
			//if (depth == 1)
			mv = movelist[idx[mvi]];
			//std::cout << "AAA " << mvi << "," << movelist[idx[mvi]] << "," << scorelist[idx[mvi]] << std::endl;			
			aiBoard->make_move(mv); //make the move on the current board
			threat[depth] = aiBoard->trimade;
			if (!halt && depth < MAXDEPTH1 && (depth < MAXDEPTH2 || threat[depth] || threat[depth-1]))
			{ 
				zptr = zmap.find(aiBoard->zhash);
				if (zptr != zmap.end())
				{
					v = zptr->second;
				}
				else
				{
					v = MaxValue(alpha, beta, depth+1);	
					zmap[aiBoard->zhash] = v;
				}
				//std::cout << depth << "::" << v << std::endl;		
				if (v <= NINF + EPS)
				{
					//no more moves from here
					v = aiBoard->get_score();
				}
			}
			else
			{
				v = aiBoard->get_score1(); // + get_playout() / 20.0;
				//
				//std::cout << depth << "::" << v << std::endl;
			}
			aiBoard->remove(mv); //undo move  
			//aiBoard.display()

			if (v < minV)
			{
				minV = v;
				if (depth == 1)
				{ 
					bmove = mv;
					bscr = v;
					//std::cout << bmove << ";" << mvi << ";"  << bscr << std::endl;			
				}
			}
			if (minV <= alpha)
			{
				//std::cout <<  "mincu " << minV << "," << alpha<< "," << beta<< "," << depth << std::endl;
				return minV; //cutoff
			}
			if (minV < beta)
				beta = minV;
			//std::cout << depth << ":v " << v << ",a " << alpha << ",b " << beta << ",m " << minV << std::endl;					
		}
	}
	//print "minou", minV, alpha, beta, depth
	return minV;
}

double AI::MaxValue(double alpha, double beta, int depth)
{
	double maxV, v; 
	int nmove, mv, lastmv, mvi;
	int movelist[aiBoard->MAXLINES], idx[aiBoard->MAXLINES];
	double scorelist[aiBoard->MAXLINES];
	unordered_map<unsigned int,double>::const_iterator zptr;

	count++;
	if (depth > maxDepth)
		maxDepth = depth;
	maxV = NINF;
	nmove = aiBoard->generate_moves(movelist, scorelist);
	for (int k = 0; k < nmove; ++k)
		idx[k] = k;		
	std::sort(idx, idx + nmove, [&scorelist](int i, int j) {return scorelist[i] > scorelist[j];});
	lastmv = min(nmove, MAXBREADTH);
					
	if (nmove == 0)
	{
		//pass
		if (depth == 1)
		{
			bmove = -1;
			bscr = 0.0;
		}
	}
	else
	{ 	
		for (mvi = 0; mvi < lastmv; ++mvi)
		{
			mv = movelist[idx[mvi]];	
			//std::cout << "BBB " << mvi << "," << movelist[idx[mvi]] << "," << scorelist[idx[mvi]] << std::endl;		
			aiBoard->make_move(mv); //make the move on the current board
			threat[depth] = aiBoard->trimade;
			if (!halt && depth < MAXDEPTH1 && (depth < MAXDEPTH2 || threat[depth] || threat[depth-1]))
			{ 
				zptr = zmap.find(aiBoard->zhash);			
				if (zptr != zmap.end())
				{
					v = zptr->second;
				}
				else
				{			
					v = MinValue(alpha, beta, depth+1);
					zmap[aiBoard->zhash] = v;
				}
				//std::cout << depth << ":" << v << std::endl;
				if (v >= PINF - EPS)
				{
					//no more moves from here
					v = aiBoard->get_score();
				}
			}
			else
			{
				v = aiBoard->get_score1(); //+ get_playout() / 20.0;
				////std::cout << depth << ":m:" << mv << "::V::" << v << std::endl;
			}
			aiBoard->remove(mv); //undo move  
			//aiBoard.display()
			if (v > maxV)
			{
				maxV = v;
				if (depth == 1)
				{ 
					bmove = mv;
					bscr = v;
				}
			}
			if (maxV >= beta)
			{
				return maxV; //cutoff
			}
			if (maxV > alpha)
				alpha = maxV;
		}
	}
	//print "maxou", maxV, alpha, beta, depth
	return maxV;
}                  
					
void AI::ABSearch(int nmove)
{
	//alpha beta dfs search

	bscr = NINF;
	bmove = -1;
	count = 0;
	zmap.clear();

	if (nmove > 100)
	{
		MAXDEPTH1 = 4; //limit depth of search in threat
		MAXDEPTH2 = 4; //regular depth of search
	}	
	else if (nmove < 18)
	{
		MAXDEPTH1 = 16; //limit depth of search in threat
		MAXDEPTH2 = 6; //regular depth of search
	}
	else 
	{
		MAXDEPTH1 = (int)(-0.09 * (double)nmove + 13.0); 
		MAXDEPTH2 = 6; 
	}

	
	auto start = chrono::high_resolution_clock::now();
	if (aiBoard->currentPlayer == 1) 
	{	
		bscr = MaxValue(NINF,PINF,1);
	}
	else
	{
		//MAXDEPTH1 = 24; //limit depth of search in threat
		//MAXDEPTH2 = 6; //regular depth of search		
		bscr = MinValue(NINF,PINF,1);
	}
	auto stop = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	printf("score: %f, depth: %d, nodes: %d,",bscr, maxDepth, count);
	std::cout << " time: " << duration.count() << std::endl;	
}
		
int AI::randMove()
{
	bmove = aiBoard->rand_move();
	bscr = 0.0;
	return bmove;
}
	
int AI::DefaultPolicy()
{
	mcBoard->copyBd(aiBoard);
	mcBoard->playout();
	return mcBoard->get_score();

}

void AI::Backup(Node *v, double delta)
{
	while (v != NULL)
	{
		v->N += 1.0;
		v->Q += delta;
		if (v->move > -1 && v->parent != NULL)
			aiBoard->remove(v->move);
		v = v->parent;
	}

}

Node* AI::MoreChild(Node *v, double c)
{
	double val, logterm, maxv = -1.0E12;
	Node *ch;
	int maxi = 0;

	logterm = 2.0 * log(v->N);
	for (int i = 0; i < v->nchildren; ++i)
	{
		ch = v->child[i];
		if (ch->N > 0.0)
		{
			val = ch->Q / ch->N + c * sqrt(logterm / ch->N);
			if (val > maxv)
			{
				maxv = val;
				maxi = i;
			}
		}
		else
		{
			std::cout << "a" << ch->Q << "," << ch->N << std::endl;
		}
	}
	return v->child[maxi];

}

Node* AI::FewChild(Node *v, double c)
{
	double val,logterm, minv = 1.0E12;
	Node *ch;
	int mini = 0;

	logterm = 2.0 * log(v->N);
	for (int i = 0; i < v->nchildren; ++i)
	{
		ch = v->child[i];
		if (ch->N > 0.0)
		{
			val= ch->Q / ch->N - c*sqrt(logterm / ch->N);
			if (val < minv)
			{
				minv = val;
				mini = i;
			}
		}
		else
		{
			std::cout << "b" << ch->Q << "," << ch->N << std::endl;
		}		
	}
	return v->child[mini];

}

Node* AI::Expand(Node *v)
{
	int i, m, nmove, mv; 
	int movelist[aiBoard->MAXLINES];
	Node *v2;

	if (v->nchildex == 0)
	{
		//genMoves
		int nmove = aiBoard->generate_moves(movelist);
		if (nmove > 0)
		{
			v->filled = false;
			for (int i = 0; i < nmove; ++i)
			{
				v2 = &narray[nodeCt++];
				v->child[v->nchildren++] = v2;
				v2->parent = v;
				v2->move = movelist[i];
				v2->depth = depth;
			}
		}
		else
			v->filled = true;
	} 
	if (!v->filled && v->nchildren > 0)
	{
		mv = JKISS() % v->nchildren; //pick untried action
		v2 = v->child[mv];
		while (v2->N > 0.0)
		{
			mv++;
			if (mv >= v->nchildren)
				mv = 0;
			v2 = v->child[mv];
		}
		v->nchildex++;
		if (v2->move > -1)
			aiBoard->make_move(v2->move);
		depth++;
		if (depth > maxDepth)
			maxDepth = depth;
		v = v2;
	} 

	return v;

}

Node* AI::TreePolicy(Node *v)
{
	while (!v->filled)
	{
		if (v->nchildex < v->nchildren || v->nchildex == 0)
		{ //not expanded
			if (nodeCt > MAXNODE - 256) //getting too close to limit to expand all possible moves
				return v;
			else
				return Expand(v);
		}
		else
		{
		
			if ((aiBoard->currentPlayer + depth) % 2 == 0)
				v = MoreChild(v,Cp);
			else
				v = FewChild(v,Cp);
			if (v->move > -1)
				aiBoard->make_move(v->move);
		}
	}

	return v;
}

int AI::UCTSearch()
{
	int ct, nm; 
	Node *v1;
	double delta;
	int movelist[aiBoard->MAXLINES];
	double time0, time1, timeLimit;
	struct timeval tv;
	
	Cp = 1.0 / sqrt(2.0);
	root = new Node();
	narray = new Node[MAXNODE];
	nodeCt = 0;
	maxDepth = 0;
	root->depth = 0;
	timeLimit = 16.000;
	bnodes = 0;
	depth = 1;
	gettimeofday(&tv, NULL);
	time1 = time0 = tv.tv_sec + 1e-6 * tv.tv_usec;	
	nm = aiBoard->generate_moves(movelist);
	if (nm == 0)
	{ //pass
		bscr = NINF;
		bmove = -1;
	}
	else
	{
		if (aiBoard->turn == 0)
		{ //make random first move
			randMove();
		}
		else
		{ //mcsearch
			ct = 0;
			do
			{
				//filled=false;
				depth = 1;
				v1 = TreePolicy(root);
				delta = (double)DefaultPolicy(); 
				Backup(v1, delta / 2.0); //gives Q range 0-1

				if (++ct%100 == 0)
				{
					gettimeofday(&tv, NULL);
					time1 = tv.tv_sec + 1e-6 * tv.tv_usec;
					//std::cout << delta << ":" << time1 - time0 << std::endl;
				}
			} while (time1 - time0 < timeLimit);

			if (aiBoard->currentPlayer == 1)
				v1 = MoreChild(root,0.0);
			else
				v1 = FewChild(root,0.0);
			bmove = v1->move;
			if (v1->N > 0)
				bscr = (int)(v1->Q * 200.0 / v1->N + 0.5);
			else
			{
				bscr = 0;
				bmove = -1;
				std::cout << delta << ":" << time1 - time0 << std::endl;
			}
		} //mcsearch
	}
	depth=0;

	//cleanup tree
	delete [] narray;
	root->nchildren = 0;
	root->nchildex = 0;
	root->N = 0.0;
	root->Q = 0.0;
	printf("score: %f, depth: %d, nodes: %d,",bscr, maxDepth, nodeCt);
	std::cout << " time: " << time1 - time0 << std::endl;

	return 0;
}

	
int AI::go(Tboard *board0)
{
	int movelist[aiBoard->MAXLINES];
	int nmove = aiBoard->generate_moves(movelist);		
	bmove = -1;
	bscr = NINF;
	maxDepth = 0;
	halt = false;
	for (int i = 0; i < aiBoard->MAXLINES; ++i)
		threat[i] = false;
	threat[0] = true;		
	aiBoard->copyBd(board0);

	//if board0->turn < 1:
	//randMove()

	if (nmove == 0)
		return -1;
	if (nmove == 1)
		return movelist[0];
	if (nmove < 17)
		ABSearch(nmove);
	else
		UCTSearch();
	
	return bmove;
}
/*
int main(int argc, char* argv[])
{
	AI *ai1;
	ai1 = new AI();
	
	delete ai1;
	return 0;
}
*/

