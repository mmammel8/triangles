// Tboard.cpp : implementation file
#include "Tboard.h"
using namespace std;

bool operator==(const Point& lhs, const Point& rhs)
{
	return (lhs.x == rhs.x && lhs.y == rhs.y);
}

bool operator!=(const Point& lhs, const Point& rhs)
{
	return (lhs.x != rhs.x || lhs.y != rhs.y);
}

Tboard::Tboard()
{
	unsigned int seed = (unsigned)time( NULL );
	srand( seed );
	//srand(1); //**********************
	xr = rand();
	yr = rand();
	cr = rand();
	zr = rand();
	clear();
}

Tboard::~Tboard()
{
}

unsigned int Tboard::JKISS()
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

double Tboard::pt_pt_dist(Point pa, Point pb)
{
	double dx, dy;
	dx = pa.x - pb.x;
	dy = pa.y - pb.y;
	return sqrt(dx*dx + dy*dy);
}

double Tboard::pt_line_dist(Point pa, Point pb, Point pc)
{  //distance of point pc from line pa-pb
	double dx, dy, num, den;
	dx = pa.x - pb.x;
	dy = pa.y - pb.y;
	num = abs(dx*(pa.y - pc.y) - dy*(pa.x - pc.x));
	den = sqrt(dx*dx + dy*dy);
	return num / den;
}

int Tboard::side(Point pa, Point pb, Point pc)
{
	//determines if point c is above line pa-pb  -> 1
	// or -1 if below or 0 if on line.
	int m1, m2;

	if (pa.x == pb.x)
	{ //segment is vertical
		if (pa.x == pc.x) return 0;
		else if (pa.x < pc.x) return 1;
		else return -1;
	}
	else if (pa.x == pc.x)
	{ //point is vertical
		if (pc.y > pa.y) return 1;
		else return -1;
	}
	else
	{
		m1 = (pb.y - pa.y)*(pc.x - pa.x);
		m2 = (pc.y - pa.y)*(pb.x - pa.x);
		if (m2 > m1) return 1;
		else if (m1 > m2) return -1;
		else return 0;
	}
}

bool Tboard::intersect(Point pa, Point pb, Point pc, Point pd)
{
	//determines if segment a-b intersects c-d
	Point p1x, p2x, p3x, p4x; //order pairs by x
	Point p1y, p2y, p3y, p4y; //by y
	int s1, s2, s3, s4;

	if (pa.x > pb.x)
	{ p1x = pb; p2x = pa; }
	else
	{ p1x = pa; p2x = pb; }
	if (pc.x > pd.x)
	{ p3x = pd; p4x = pc; }
	else
	{ p3x = pc; p4x = pd; }
	if (pa.y > pb.y)
	{ p1y = pb; p2y = pa; }
	else
	{ p1y = pa; p2y = pb; }
	if (pc.y > pd.y)
	{ p3y = pd; p4y = pc; }
	else
	{ p3y = pc; p4y = pd; }

	//check not overlapping
	if (p3x.x > p2x.x || p1x.x > p4x.x || p3y.y > p2y.y || p1y.y > p4y.y)
		return false;

	s1 = side(p3x,p4x,p1x);
	s2 = side(p3x,p4x,p2x);
	s3 = side(p1x,p2x,p3x);
	s4 = side(p1x,p2x,p4x);
	if (s1==0 || s2==0 || s3==0 || s4==0)
		return true; //endpoint on segment
	return (s1!=s2 && s3!=s4);
}

int Tboard::sign(Point p1, Point p2, Point p3)
{
	//returns number indicating side of p1 on segment p2-p3
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool Tboard::point_in_triangle(Point pt, Point v1, Point v2, Point v3)
{
	//returns if pt in triangle defined by three verts
		int d1,d2,d3;
		bool has_neg, has_pos;

	d1 = sign(pt, v1, v2);
	d2 = sign(pt, v2, v3);
	d3 = sign(pt, v3, v1);
	has_neg = (d1 < 0 || d2 < 0 || d3 < 0);
	has_pos = (d1 > 0 || d2 > 0 || d3 > 0);
	return !(has_neg && has_pos);
}

bool Tboard::linelegal(Point pt1, Point pt2)
{
	//returns if pt1 to pt2 does not hit another point on board
	double threshold, dx, dy, num, den;
	int lox, hix, loy, hiy;
	threshold = PRADIUS + 1; //LWIDTH/2;
	dx = pt2.x - pt1.x;
	dy = pt2.y - pt1.y;
	den = dx*dx + dy*dy;
	lox = min(pt1.x, pt2.x) - PRADIUS;
	hix = max(pt1.x, pt2.x) + PRADIUS;
	loy = min(pt1.y, pt2.y) - PRADIUS;
	hiy = max(pt1.y, pt2.y) + PRADIUS;
	threshold = threshold * threshold * den;
	for (int i = 0; i < NUMPOINTS; ++i)
	{
		if (points[i] != pt1 && points[i] != pt2 &&
			points[i].x >= lox && points[i].x <= hix && points[i].y >= loy && points[i].y <= hiy)
		{
			//num = abs(dx*(pt1.y - points[i].y) - dy*(pt1.x - points[i].x));
			num = dx*(pt1.y - points[i].y) - dy*(pt1.x - points[i].x);
			//cout << num*num << "," << threshold << "," << i << endl;
			if (num*num < threshold)
			{
				//cout << num*num << "," << threshold << "," << i << endl;
				return false;
			}
		}
	}
	return true;
}

bool Tboard::trilegal(Point pt1, Point pt2, Point pt3)
{
	//returns if triangle does not contain another point
	for (int i = 0; i < NUMPOINTS; ++i)
	{
		if (points[i] != pt1 && points[i] != pt2 && points[i] != pt3)
			if (point_in_triangle(points[i], pt1, pt2, pt3))
				return false;
	}
	return true;
}

void Tboard::rand_start()
{
	bool ok;
	int x, y, width, height;
	width = CWIDTH - 2*BUFFERDIST;
	height = CHEIGHT - 2*BUFFERDIST;
	for (int i = 0; i < NUMPOINTS; ++i)
	{
		ok = false;
		while (!ok)
		{
			x = rand()%width + BUFFERDIST;
			y = rand()%height + BUFFERDIST;
			points[i].x = x;
			points[i].y = y;
			ok = true;
			for (int j = 0; j < i; ++j)
			{
				if (pt_pt_dist(points[i], points[j]) < BUFFERDIST)
				{
					ok = false;
					break;
				}
			}
		}
	}
}

void Tboard::test_start()
{
	//int data[12] = {15,45,20,110,90,130,130,50,75,20,65,80};
	int data[40] = {92,69,169,49,534,57,248,79,409,76,150,160,328,148,516,161,107,251,185,287,253,256,384,225,488,264,331,313,413,335,68,400,156,386,266,398,353,405,507,394};
	for (int i = 0; i < 40; i+=2)
	{
		points[i/2].x = data[i];
		points[i/2].y = data[i+1];		
	}
}

void Tboard::init()
{
	Point pa, pb, pc, pd;
	int l1, l2, l3;
	
	for (int i = 0; i < MAXLINES; ++i)
	{
		line_tri_ct[i] = 0;
		for (int j = 0; j < MAXLINES; ++j)
			line_cross[i][j] = true;
	}
	for (int i = 0; i < NUMPOINTS; ++i)
		for (int j = 0; j < NUMPOINTS; ++j)
		{
			linenum[i][j] = -1;
			for (int k = 0; k < NUMPOINTS; ++k)
				trinum[i][j][k] = -1;
		} 
	linect = 0;		
	for (int i = 0; i < NUMPOINTS; ++i)
		for (int j = i + 1; j < NUMPOINTS; ++j)
			if (linelegal(points[i], points[j]) )
			{
				linenum[i][j] = linenum[j][i] = linect;
				lines[linect][0] = i;
				lines[linect][1] = j;
				//cout << i << "," << j << ":" << linect << "::" << points[i].x << ","  << points[i].y << ","  << points[j].x << ","  << points[j].y << endl;
				linect++;
			}
	trict = 0;
	for (int i = 0; i < NUMPOINTS; ++i)
		for (int j = i + 1; j < NUMPOINTS; ++j)
			for (int k = j + 1; k < NUMPOINTS; ++k)
			{
				l1 = linenum[i][j];
				l2 = linenum[i][k];
				l3 = linenum[j][k];
				if (l1 > -1 && l2 > -1 && l3 > -1)
					if (trilegal(points[i], points[j], points[k]))
					{
						trinum[i][j][k] = trinum[j][i][k] = trict;
						trinum[i][k][j] = trinum[j][k][i] = trict;
						trinum[k][i][j] = trinum[k][j][i] = trict;
						triangles[trict][0] = i;
						triangles[trict][1] = j;
						triangles[trict][2] = k;
						line2triangles[l1][line_tri_ct[l1]++] = trict;
						line2triangles[l2][line_tri_ct[l2]++] = trict;
						line2triangles[l3][line_tri_ct[l3]++] = trict;
						trict++;
					}
			}

	for (int l1 = 0; l1 < linect; ++l1)
	{	
		pa = points[lines[l1][0]];
		pb = points[lines[l1][1]];
		for (int l2 = l1 + 1; l2 < linect; ++l2)
		{
			pc = points[lines[l2][0]];
			pd = points[lines[l2][1]];
			if (pa == pc || pa == pd || pb == pc || pb == pd)
				line_cross[l1][l2] = line_cross[l2][l1] = false;
			else
			{
				line_cross[l1][l2] = line_cross[l2][l1] = intersect(pa, pb, pc, pd);
			}
		}
	}
	
	for (int l1 = 0; l1 < linect; ++l1)
	{
		line_cross_ct[l1] = 0;
		zobrist1[l1][0]	= JKISS();
		zobrist1[l1][1]	= JKISS();		
		for (int l2 = 0; l2 < linect; ++l2)
		{	
			if (line_cross[l1][l2] && l1 != l2)
			{
				crosslines[l1][line_cross_ct[l1]++] = l2;
			}	
		}			
	}
	for (int t1 = 0; t1 < trict; ++t1)
	{
		zobrist2[t1][0]	= JKISS();
		zobrist2[t1][1]	= JKISS();	
	}
			
	cout << linect << "," << trict << endl;

}

void Tboard::clear()
{
 	numlines = linect;
	numtri = trict;		
 	currentPlayer = 1;
 	turn = 0;
 	winner = 0;
 	score[0] = score[1] = score[2] = 0;
 	for (int i = 0; i < numlines; ++i)
 		linecolor[i] = 0;
 	for (int i = 0; i < numtri; ++i)
 	{
 		trifilled[i] = 0;	
 		tricolor[i] = 0;
 	}
 	trimade = false;
 	lastturn = 0;
 	zhash = 0;
}

void Tboard::copyBd(Tboard* mainBd)
{
 	currentPlayer = mainBd->currentPlayer;
 	turn = mainBd->turn;
 	winner = mainBd->winner;
 	score[1] = mainBd->score[1];
 	score[2] = mainBd->score[2];
  	numlines = mainBd->numlines;
 	numtri = mainBd->numtri;	
 	for (int i = 0; i < numlines; ++i)
 		linecolor[i] = mainBd->linecolor[i];
 	for (int i = 0; i < numtri; ++i)
 	{
 		trifilled[i] = mainBd->trifilled[i];	
 		tricolor[i] = mainBd->tricolor[i];
 	}
 	for (int i = 0; i < turn; ++i)
 		history[i] = mainBd->history[i];
 	zhash = mainBd->zhash;
	lastturn = mainBd->lastturn;
	trimade = mainBd->trimade;
}

int Tboard::get_winner()
{
	/*
	finds winning player when no moves available
	*/
	if (score[1] > score[2])
		winner = 1;
	else if (score[2] > score[1])
		winner = 2;
	else
		winner = 0;
	return winner;
}

bool Tboard::legalline(int l1)
{
	//returns if line is a valid move
	//std::cout << l1 << " ";
	if (l1 == -1)
	{
		//std::cout << "-1" << std::endl;
		return false; //not valid line
	}
	if (linecolor[l1] > 0)
	{
		//std::cout << ">0" << std::endl;		
		return false; //already played
	}
	for (int i = 0; i < line_cross_ct[l1]; ++i)
	{
		if (linecolor[crosslines[l1][i]] > 0)
		{
			//std::cout << "X" << std::endl;
			return false; //intersects a line on board
		}
	}
	//std::cout << "ok" << std::endl;
	return true;
}

double Tboard::get_score0()
{
	double scr;
	scr = (double)score[1] - (double)score[2];
	return scr;
}

double Tboard::get_score1()
{
	/*
	evaluates the final board for player 1
	returns score
	*/
	double scr = score[1] - score[2];
	scr += (double)(JKISS()%10) / 1000.0;

	return scr;
}

double Tboard::get_score2(int line1)
{
	/*
	evaluates the line move
	returns score
	*/
	double value[3]  = {1.0, 6.0, 100.0};
	double scr = (double)(JKISS()%10) / 1000.0;
	double tf = 0.0;
	for (int i = 0; i < line_tri_ct[line1]; ++i)
	{
		int tr = line2triangles[line1][i];
		tf += value[trifilled[tr]];
	}
	scr += tf;

	return scr;
}
	
bool Tboard::rewind()
{
	int line1;
	int savelast = lastturn;
	
	if (turn < 1)
		return false;	
	while (turn > 0)
	{
		line1 = history[turn - 1];
		remove(line1);
	}
	lastturn = savelast;
	return true;
	
}

bool Tboard::ffwd()
{
	int line1, savelast;

	if (turn > lastturn)
		return false;	
	while (turn <= lastturn)
	{
		savelast = lastturn;	
		line1 = history[turn];
		make_move(line1);
		lastturn = savelast;
	}
	return true;

}

bool Tboard::back()
{
	if (turn < 1)
		return false;
	int line1 = history[turn - 1];
	int savelast = lastturn;
	remove(line1);
	lastturn = savelast;
	return true;
	
}

bool Tboard::ahead()
{
	if (turn > lastturn)
		return false;
	int line1 = history[turn];
	int savelast = lastturn;
	make_move(line1);
	lastturn = savelast;
	return true;
	
}
		
void Tboard::make_move(int line1)
{
	//makes the single move line
	//updates triangles
	int tr;
	linecolor[line1] = currentPlayer;
	zhash ^= zobrist1[line1][currentPlayer-1];
	trimade = false;
	for (int i = 0; i < line_tri_ct[line1]; ++i)
	{
		tr = line2triangles[line1][i];
		trifilled[tr] += 1;
		if (trifilled[tr] == 3)
		{
			tricolor[tr] = currentPlayer;
			score[currentPlayer] += 1;
			trimade = true;
			zhash ^= zobrist2[tr][currentPlayer-1];
		}
	}
	lastturn = turn;	
	history[turn++] = line1;
	currentPlayer = 3 - currentPlayer;
}

void Tboard::remove(int line1)
{
	//removes the line
	//updates triangles
	int tr;
	turn--;
	lastturn = turn - 1;
	currentPlayer = 3 - currentPlayer;	
	linecolor[line1] = 0;
	zhash ^= zobrist1[line1][currentPlayer-1];
	for (int i = 0; i < line_tri_ct[line1]; ++i)
	{
		tr = line2triangles[line1][i];
		if (trifilled[tr] == 3)
		{
			tricolor[tr] = 0;
			score[currentPlayer] -= 1;
			zhash ^= zobrist2[tr][currentPlayer-1];
		}
		trifilled[tr] -= 1;
	}
}

int Tboard::generate_moves(int *linelist, double *scorelist)
{
	//generates all legal moves for player & score
	int nmoves = 0;
	for (int l1 = 0; l1 < numlines; ++l1)
		if (legalline(l1))
		{
			linelist[nmoves] = l1;
			scorelist[nmoves++] = get_score2(l1);
		}
	return nmoves;
}

int Tboard::generate_moves(int *linelist)
{
	//generates all legal moves for player
	int nmoves = 0;
	for (int l1 = 0; l1 < numlines; ++l1)
		if (legalline(l1))
		{
			linelist[nmoves++] = l1;
		}
	//std::cout << "pip " << nmoves << " " << numlines << std::endl;	
	return nmoves;
}

int Tboard::rand_move()
{
	//return a random legal move if any
	int linelist[MAXLINES], i, nmove, l1;
	l1 = -1;
	nmove = generate_moves(linelist);
	if (nmove > 0)
	{
		i = JKISS()%nmove;
		l1 = linelist[i];
	}
	return l1;
}

void Tboard::playout2()
{
	//random playout until maxmoves
	const int MAXMV = 8;
	int mv = 0;
	int l1 = 0;
	trimade = false;
	bool primade = false;
	while (l1 > -1 && (mv < MAXMV || trimade || primade))
	{
		primade = trimade;	
		l1 = rand_move();
		if (l1 > -1)
			make_move(l1);
		mv++;		
	}
}

void Tboard::playout()
{
	//random playout
	int l1 = 0;
	while (l1 > -1)
	{
		l1 = rand_move();
		if (l1 > -1)
			make_move(l1);	
	}
}


bool Tboard::Read()
{
	ifstream fin;
	string line1;
	int x, y, nmoves;
	char ch;
	
	fin.open(fnameIn);
	if (!fin.is_open())
	{
		cout << "could not find " << fnameIn << endl;
		return false;
	}
	
	getline(fin, line1);
	getline(fin, line1);
	getline(fin, line1);
	stringstream stream0(line1);
	stream0 >> x;
	currentPlayer = x;	
	getline(fin, line1); //unused
	for (int i = 0; i < NUMPOINTS; ++i)
	{
		getline(fin, line1);
		stringstream stream1(line1);
		stream1 >> x >> ch >> y;
		//printf("pt %d %d\n", x, y);		
		points[i].x = x;
		points[i].y = y;
	}
    init();
    clear();
	
	getline(fin, line1);
	stringstream stream2(line1);
	stream2 >> x;
	nmoves = x;
	
	for (int i = 0; i < nmoves; ++i)
	{
		getline(fin, line1);
		stringstream stream3(line1);
		stream3 >> x >> ch >> y;
		//printf("ll %d %d\n", x, y);
		int line1 = linenum[x][y];
		make_move(line1);
	}				
	fin.close();
	return true;
}

bool Tboard::savetodisk()
{
	int col,row;
	int leading, xs, os;
	string out;
	ofstream outfile;
	//string fnameOut = "out" + to_string((long long)N) + "uct.txt";

	outfile.open(fnameIn);
	outfile << "Triangles game" << endl;
	out = to_string(NUMPOINTS) + " points, " + to_string(numlines) + " lines, " + to_string(numtri) + " triangles";
	outfile << out << endl;
	out = to_string(currentPlayer) + " currentPlayer, " + to_string(turn) + " turn, " + to_string(score[1]) + " score1, " + to_string(score[2]) + " score2";
	outfile << out << endl;
	outfile << "Points" << endl;
	for (int i = 0; i < NUMPOINTS; ++i)
	{
		out = to_string(points[i].x) + ", " + to_string(points[i].y);
		outfile << out << endl;
	}
	out = to_string(turn) + " moves";
	outfile << out << endl;
	for (int i = 0; i < turn; ++i)
	{
		int l1 = history[i];
		out = to_string(lines[l1][0]) + '-' + to_string(lines[l1][1]);
		outfile << out << endl;
	}
	outfile.close();
	
	return true;
}

//static varibles
Point Tboard::points[NUMPOINTS]; //list of points on the board
int Tboard::lines[MAXLINES][2]; //list of lines in form of two point #s
int Tboard::triangles[MAXTRIANGLES][3]; //list of triangles in form of triples of point numbers
int Tboard::linenum[NUMPOINTS][NUMPOINTS]; // [p1][p2]  number of line between p1 and p2
int Tboard::trinum[NUMPOINTS][NUMPOINTS][NUMPOINTS]; //[p1][p2][p3]  number of triangle with vert p1 p2 p3
int Tboard::line2triangles[MAXLINES][NUMPOINTS-2]; //lists triangles including this line
int Tboard::line_tri_ct[MAXLINES];
bool Tboard::line_cross[MAXLINES][MAXLINES]; //[l1][l2] if these two intersect
int Tboard::crosslines[MAXLINES][MAXLINES]; // lines that cross this one
int Tboard::line_cross_ct[MAXLINES]; //num cross lines
unsigned int Tboard::zobrist1[MAXLINES][2]; //zobrist hash [lines][2]
unsigned int Tboard::zobrist2[MAXTRIANGLES][2]; //zobrist hash [triangles][2]	

/*
int main(int argc, char* argv[])
{

	Tboard *bd1;
	bd1 = new Tboard();
	//bd1->rand_start();
	bd1->test_start();
	bd1->init();
	bd1->clear();
	int linelist[bd1->MAXLINES];
	double scorelist[bd1->MAXLINES];	
	bd1->make_move(13);	
	int nmoves = bd1->generate_moves(linelist, scorelist);
	cout << nmoves << endl;
	delete bd1;
	return 0;
}
*/
