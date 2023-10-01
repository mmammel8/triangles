#include "AI.h"
using namespace std;

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
			if (depth < MAXDEPTH1 && (depth < MAXDEPTH2 || threat[depth])) // || (depth == MAXDEPTH2 && threat[depth-1])))
			{ 
				v = MaxValue(alpha, beta, depth+1);	
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
			if (depth < MAXDEPTH1 && (depth < MAXDEPTH2 || threat[depth]))// || (depth == MAXDEPTH2 && threat[depth-1])))
			{ 
				v = MinValue(alpha, beta, depth+1);
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
                    
void AI::ABSearch()
{
	//alpha beta dfs search
	bscr = NINF;
	bmove = -1;
	count = 0;
	auto start = chrono::high_resolution_clock::now();
	if (aiBoard->currentPlayer == 1) 
		bscr = MaxValue(NINF,PINF,1);
	else
		bscr = MinValue(NINF,PINF,1);
	auto stop = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
	std::cout << duration.count() << std::endl;
	printf("count: %d\n", count);
}
		
int AI::randMove()
{
	bmove = aiBoard->rand_move();
	bscr = 0.0;
	return bmove;
}
	
double AI::get_playout()
{	
	mcBoard->copyBd(aiBoard);
	mcBoard->playout();
	return mcBoard->get_score();
}
	
int AI::go(Tboard *board0)
{
	bmove = -1;
	bscr = NINF;
	maxDepth = 0;
	for (int i = 0; i < aiBoard->MAXLINES; ++i)
		threat[i] = false;
	threat[0] = true;		
	aiBoard->copyBd(board0);
	//if self.aiBoard.turn < 0:
	//self.randMove()
	//if self.aiBoard.currentPlayer == 1:

	/*
	int linelist[board0->MAXLINES];
	double scorelist[board0->MAXLINES];	
	int nmoves = board0->generate_moves(linelist, scorelist);
	std::cout << nmoves << std::endl;
	*/
	
	ABSearch();
	//else:
	//self.MonteCarloMove()
	printf("%f, %d\n",bscr, maxDepth);

	
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

