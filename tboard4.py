"""
Board of Triangles
v3.0 1/15/2023
MKM
"""
import random
import math
import time
from collections import namedtuple

#constants
NUMPOINTS = 20 #number of points to place
CWIDTH = 800 #width of board
CHEIGHT = 600 #height of board
BUFFERDIST = 80 #minimum distance between points
LWIDTH = 4 #line width
PRADIUS = 10 #point radius
CLICKRAD = 15 #within this of point to accept click
MAXDEPTH1 = 18 #limit depth of search
MAXDEPTH2 = 3
MAXBREADTH = 24

PINF = 1000000000
NINF = -1000000000

inname = "Triangles_game.txt"
outname = "Triangles_game.txt"

Point = namedtuple('Point', 'x y')

#globals
points = [] #list of points on the board
lines = [] #list of lines in form of tuples of point numbers
triangles = [] #list of triangles in form of triples of point numbers
linenum = [] # [p1][p2]  number of line between p1 and p2
trinum = []  #[p1][p2][p3]  number of triangle with vert p1 p2 p3
line2triangles = []  #lists triangles including this line
line_cross = [] #[l1][l2] if these two intersect

def ptdist(pa, pb):
	"""
	return distance between two points
	"""
	dx = pb.x - pa.x
	dy = pb.y - pa.y
	return math.sqrt(dx*dx + dy*dy)

def pt_line_dist(pa, pb, pc):
	"""
	return distance of pt c from line pa-pb
	"""
	dx = pb.x - pa.x
	dy = pb.y - pa.y
	num = dx*(pa.y-pc.y) - dy*(pa.x-pc.x)
	den = math.sqrt(dx*dx + dy*dy)
	return num / den
	
def side(pa, pb, pc):
	"""
	returns 1 if point c is above line a-b
	or -1 if below or 0 if on line.
	"""
	if pa.x == pb.x:
	#segment is vertical
		if pa.x == pc.x:
			return 0
		elif pa.x < pc.x:
			return 1
		else:
			return -1
	elif pa.x == pc.x:
	#point is vertical
		if pc.y > pa.y:
			return 1
		else:
			return -1
	else:
		m1 = (pb.y - pa.y)*(pc.x - pa.x)
		m2 = (pc.y - pa.y)*(pb.x - pa.x)
		if m2 > m1:
			return 1
		elif m1 > m2:
			return -1
		else:
			return 0

def intersect(pa, pb, pc, pd):
	"""
	determines if segment pa-pb intersects pc-pd
	"""
	if pa.x > pb.x:
		p1x, p2x = pb, pa
	else:
		p1x, p2x = pa, pb
	if pc.x > pd.x:
		p3x, p4x = pd, pc
	else:
		p3x, p4x = pc, pd
	if pa.y > pb.y:
		p1y, p2y = pb, pa
	else:
		p1y, p2y = pa, pb
	if pc.y > pd.y:
		p3y, p4y = pd, pc
	else:
		p3y, p4y = pc, pd

	#check not overlapping
	#print(p1x.y, p2x.y, p3x.y, p4x.y)
	if p3x.x > p2x.x or p1x.x > p4x.x or p3y.y > p2y.y  or p1y.y > p4y.y:  
		return False

	s1 = side(p3x, p4x, p1x)
	s2 = side(p3x, p4x, p2x)
	s3 = side(p1x, p2x, p3x)
	s4 = side(p1x, p2x, p4x)
	if not s1 or not s2 or not s3 or not s4:
		return True #endpoint on segment
	return s1 != s2 and s3 != s4
	
def sign(p1, p2, p3):
	"""
	returns number indicating side of p1 on segment p2-p3 
	"""
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y)

def point_in_triangle(pt, v1, v2, v3):
	"""
	returns if pt in triangle defined by three verts
	"""
	d1 = sign(pt, v1, v2)
	d2 = sign(pt, v2, v3)
	d3 = sign(pt, v3, v1)
	has_neg = d1 < 0 or d2 < 0 or d3 < 0
	has_pos = d1 > 0 or d2 > 0 or d3 > 0
	return not (has_neg and has_pos)
	
def linelegal(pt1, pt2):
	"""
	returns if pt1 to pt2 does not hit another point on board
	"""
	threshold = PRADIUS #+ LWIDTH
	dx = pt2.x - pt1.x
	dy = pt2.y - pt1.y
	den = dx*dx + dy*dy
	lox = min(pt1.x, pt2.x) - PRADIUS
	hix = max(pt1.x, pt2.x) + PRADIUS
	loy = min(pt1.y, pt2.y) - PRADIUS
	hiy = max(pt1.y, pt2.y) + PRADIUS
	threshold = threshold * threshold * den	  
	for i in range(NUMPOINTS):
		if points[i] != pt1 and points[i] != pt2 and points[i].x >= lox and points[i].x <= hix and points[i].y >= loy and points[i].y <= hiy:
			num = dx*(pt1.y - points[i].y) - dy*(pt1.x - points[i].x)
			if num * num < threshold:
				#print(num*num, threshold, pt1, pt2, points[i])
				return False
	return True

def trilegal(pt1, pt2, pt3):
	"""
	returns if triangle does not contain another point
	"""
	for pt0 in points:
		if pt0 != pt1 and pt0 != pt2 and pt0 != pt3:
			if point_in_triangle(pt0, pt1, pt2, pt3):
				return False
	return True
			
def rand_start():
	"""
	rand gen lists of points	
	"""
	global points 
	points = []
	for i in range(NUMPOINTS):
		ok = False
		while not ok:
			x = random.randint(BUFFERDIST, CWIDTH-BUFFERDIST-1)
			y = random.randint(BUFFERDIST, CHEIGHT-BUFFERDIST-1)
			pt = Point(x,y)
			ok = True
			for j in range(i):
				if ptdist(pt, points[j]) < BUFFERDIST:
					ok = False
					break
		points.append(pt)

def test_start():
	"""
	gen lists of points	
	"""
	global points 
	points = [Point(15,45),Point(20,110),Point(90,130),Point(130,50),Point(75,20),Point(65,80)]


def init():
	"""
	precompute lists of lines, triangles	
	"""
	global lines, triangles, linenum, trinum, line2triangles, line_cross 
	linect = 0
	lines = []
	line2triangles = []	
	linenum = [[-1 for _ in range(NUMPOINTS)] for _ in range(NUMPOINTS)]
	for i in range(NUMPOINTS):
		for j in range(i + 1, NUMPOINTS):
			if linelegal(points[i], points[j]):
				linenum[i][j] = linenum[j][i] = linect
				lines.append((i,j))
				line2triangles.append([])
				linect += 1

	trict = 0	
	triangles = []
	trinum = [[[-1 for _ in range(NUMPOINTS)] for _ in range(NUMPOINTS)] for _ in range(NUMPOINTS)]
	for i in range(NUMPOINTS):
		for j in range(i + 1, NUMPOINTS):
			for k in range(j + 1, NUMPOINTS):
				l1 = linenum[i][j]
				l2 = linenum[i][k]
				l3 = linenum[j][k]
				if l1 > -1 and l2 > -1 and l3 > -1:
					if trilegal(points[i], points[j], points[k]):
						 trinum[i][j][k] = trinum[j][i][k] = trict
						 trinum[i][k][j] = trinum[j][k][i] = trict
						 trinum[k][i][j] = trinum[k][j][i] = trict
						 triangles.append((i,j,k))
						 line2triangles[l1].append(trict)
						 line2triangles[l2].append(trict)
						 line2triangles[l3].append(trict)						 
						 trict += 1
	line_cross = [[True for _ in range(linect)] for _ in range(linect) ]
	for l1 in range(linect):
		pa = points[lines[l1][0]]
		pb = points[lines[l1][1]]
		for l2 in range(l1 + 1, linect):
			pc = points[lines[l2][0]]
			pd = points[lines[l2][1]]
			if pa == pc or pa == pd or pb == pc or pb == pd:
				line_cross[l1][l2] = line_cross[l2][l1] = False
			else:
				line_cross[l1][l2] = line_cross[l2][l1] = intersect(pa, pb, pc, pd)
	print(linect, trict)
	
class Board:
	"""
	class to hold Triangles Board
	"""
	def __init__(self):
		self.numlines = 0
		self.numtri = 0
		self.currentPlayer = 1
		self.turn = 0
		self.winner = -1		
		self.score = [0,0,0]
		self.linecolor = []
		self.trifilled = []
		self.trimade = False
		self.history = []
		self.clear_board()

	def clear_board(self):
		"""
		restore to beginning of game
		"""
		self.numlines = len(lines)
		self.numtri = len(triangles)		
		self.currentPlayer = 1
		self.turn = 1
		self.winner = -1
		self.score = [0,0,0]		
		self.linecolor = [0 for _ in range(self.numlines)]  #0, 1, 2 player
		self.trifilled = [0 for _ in range(self.numtri)] #0,1,2,3 sides   
		self.tricolor = [0 for _ in range(self.numtri)] #0, 1, 2 player
		self.trimade = False
		self.history = []

	def clone(self, board0):
		"""
		copy game state from board0
		"""
		self.numlines = board0.numlines
		self.numtri = board0.numtri		 
		self.currentPlayer = board0.currentPlayer
		self.turn = board0.turn
		self.score = list(board0.score)
		self.linecolor = list(board0.linecolor)
		self.trifilled = list(board0.trifilled)
		self.tricolor = list(board0.tricolor)
		self.history = list(board0.history)

	def exportGame(self):
		out_file = open(outname, 'w')
		out_file.write("Triangles game\n")
		out_file.write(str(NUMPOINTS) + " points, " + str(self.numlines) + " lines, " + str(self.numtri) + " triangles\n")
		out_file.write(str(self.currentPlayer) + " currentPlayer, " + str(self.turn) + " turn, " + str(self.score[1]) + " score1, " + str(self.score[2]) + " score2\n")
		out_file.write("Points\n")
		for pt in points:
			out = str(pt.x) + ", " + str(pt.y) + "\n"
			out_file.write(out)
		#out_file.write("linecolor\n")
		#out_file.write(",".join(map(str,self.linecolor)) + "\n")
		#out_file.write("tricolor\n")
		#out_file.write(",".join(map(str,self.tricolor)) + "\n")
		#out_file.write("trifilled\n")
		#out_file.write(",".join(map(str,self.trifilled)) + "\n")
		nmoves = len(self.history)
		out = str(nmoves) + " moves\n"
		out_file.write(out)
		for i in range(len(self.history)):
			l1 = self.history[i]
			out = str(lines[l1][0]) + '-' + str(lines[l1][1]) + "\n"
			out_file.write(out)	 
		out_file.close()

	def importGame(self):
		global points
		data_file = open(inname, 'r')
		data = ''.join(data_file.readlines())
		data_file.close()
		lines = data.split('\n')
		points = []
		for ln in range(4, NUMPOINTS+4):
			row = lines[ln].strip().split(",")
			pt = Point(int(row[0]), int(row[1]))
			points.append(pt)
		init() #dont rerandom pts
		self.clear_board()
		row = lines[1].split(",")
		if len(row) != 3:
			print("error on points lines triangles line")
		pos = row[0].find(" points")
		if pos > -1:
			if int(row[0][:pos]) != NUMPOINTS:
				print("wrong number of points")
		else:
			print("can't find points")
		"""
		row = lines[2].split(",")
		if len(row) != 4:
			print("error on currentPlayer line")
		pos = row[0].find(" currentPlayer")
		if pos > -1:		 
			 self.currentPlayer = int(row[0][:pos])
		else:
			print("can't find currentPlayer")
		pos = row[1].find(" turn")
		if pos > -1:		 
			 self.turn = int(row[1][:pos])
		else:
			print("can't find turn")
		pos = row[2].find(" score1")
		if pos > -1:		 
			 self.score[1] = int(row[2][:pos])
		else:
			print("can't find score1")
		pos = row[3].find(" score2")
		if pos > -1:		 
			 self.score[2] = int(row[3][:pos])
		else:
			print("can't find score2")
		line1 = lines[NUMPOINTS + 5].strip()
		self.linecolor = list(map(int,line1.split(',')))
		line1 = lines[NUMPOINTS + 7].strip()
		self.tricolor = list(map(int,line1.split(',')))
		line1 = lines[NUMPOINTS + 9].strip()
		self.trifilled = list(map(int,line1.split(',')))
		"""
		
		line1 = lines[NUMPOINTS + 4].strip()
		nmoves = 0
		pos = line1.find(" moves")
		if pos > -1:		 
			 nmoves = int(line1[:pos])
		for ln in range(NUMPOINTS + 5, NUMPOINTS + 5 + nmoves):
			row = lines[ln].strip().split("-")
			pa, pb = int(row[0]), int(row[1])
			mv = linenum[pa][pb]
			self.make_move(mv)
			
	def legalline(self, l1):
		"""
		returns if line is a valid move
		"""
		if l1 == -1:
			#print("-1")
			return False #not valid line
		if self.linecolor[l1] > 0:
			#print(">0")		
			return False #already played
		for l2 in range(self.numlines):
			if self.linecolor[l2] > 0:
				if line_cross[l1][l2]:
					#print("X")
					return False #intersects a line on board
		return True

	def find_winner(self):
		"""
		finds winning player when no moves available
		sets self.winner
		"""
		if self.score[1] > self.score[2]:
			self.winner = 1
		elif self.score[2] > self.score[1]:
			self.winner = 2
		else:
			self.winner = 0
		return self.winner

	def get_score1(self):
		"""
		evaluates the final board for player 1
		returns score
		"""
		scr = self.score[1] - self.score[2] + random.randint(0,10) / 1000.0
		
		return scr

	def get_score2(self, line1):
		"""
		evaluates the line move
		returns score
		"""
		value  = [5, 1, 100] 
		scr = random.randint(0,10) / 1000.0
		tf = 0
		for tr in line2triangles[line1]:
			tf += value[self.trifilled[tr]]
		scr += tf
		
		return scr

	def make_move(self, line1):
		"""
		makes the single move line		
		updates triangles
		"""
		self.linecolor[line1] = self.currentPlayer
		self.trimade = False				
		for tr in line2triangles[line1]:
			self.trifilled[tr] += 1
			if self.trifilled[tr] == 3:
				self.tricolor[tr] = self.currentPlayer
				self.score[self.currentPlayer] += 1
				self.trimade = True
		self.turn += 1
		self.history.append(line1)
		self.currentPlayer = 3 - self.currentPlayer

	def remove(self, line1):
		"""
		removes the line
		updates triangles
		"""
		self.turn -= 1
		self.currentPlayer = 3 - self.currentPlayer		
		self.linecolor[line1] = 0		
		for tr in line2triangles[line1]:
			if self.trifilled[tr] == 3:
				self.tricolor[tr] = 0
				self.score[self.currentPlayer] -= 1	 
			self.trifilled[tr] -= 1
		self.history.pop()
		
	def undo(self):
		"""
		removes the line
		updates triangles
		"""
		line1 = self.history.pop()
		self.turn -= 1
		self.currentPlayer = 3 - self.currentPlayer		
		self.linecolor[line1] = 0		
		for tr in line2triangles[line1]:
			if self.trifilled[tr] == 3:
				self.tricolor[tr] = 0
				self.score[self.currentPlayer] -= 1	 
			self.trifilled[tr] -= 1
				
	def generate_moves(self):
		"""
		generates all legal moves for player
		"""
		smoves = []
		for l1 in range(self.numlines):
			if self.legalline(l1):
				scr = self.get_score2(l1)
				smoves.append((scr,l1))
		return smoves

	def rand_move(self):
		"""
		return a random legal move if any
		"""
		l1 = None
		smoves = self.generate_moves()
		if len(smoves) > 0:
			l1 = random.choice(smoves)
		return l1
			 
	def playout(self):
		"""
		random playout till end of game
		"""
		MAXMV = 128
		mv = 0
		l1 = 0
		
		while l1 != None and mv < MAXMV:
			l1 = self.rand_move()
			if l1 != None:
				self.make_move(l1)
			mv += 1

	def playout2(self):
		"""
		random playout until triangle made or maxmoves
		"""
		MAXMV = 12
		mv = 0
		l1 = 0
		self.trimade = False
		mades = 0
		while l1 != None and mv < MAXMV and mades < 2:
			l1 = self.rand_move()
			if l1 != None:
				self.make_move(l1)
			mv += 1
			if self.trimade:
				mades += 1
			else:
				mades = 0

class AI:
	"""
	class to run the AI process
	"""
	def __init__(self):
		self.aiBoard = Board()
		self.bmove = None
		self.bscr = 0
		self.maxDepth = 0
		self.threat = []
		self.count = 0
		 
	def MinValue(self, alpha, beta, depth):
		#print "minin", alpha, beta, depth
		self.count += 1
		if depth > self.maxDepth:
			self.maxDepth = depth
		minV = PINF
		movelist = self.aiBoard.generate_moves()
		movelist.sort(reverse=True)
		nmove = len(movelist)
		#print "minmv", nmove
		if nmove == 0:
			#pass
			if depth==1:
				self.bmove = None
				self.bscr = -1
		else: 
			#for mv in movelist:
			for i in range(min(MAXBREADTH, len(movelist))):
				mv = movelist[i][1]
				self.aiBoard.make_move(mv) #make the move on the current board
				self.threat[depth] = self.aiBoard.trimade
				if depth < MAXDEPTH1 and (depth < MAXDEPTH2 or self.threat[depth]):# or self.threat[depth-1]): 
					v = self.MaxValue(alpha, beta, depth+1)
					if v == NINF:
						#no more moves from here
						v = self.aiBoard.get_score1()
				else:
					v = self.aiBoard.get_score1()
				self.aiBoard.remove(mv) #undo move  
				#self.aiBoard.display()
				if v < minV: 
					minV = v
					if depth==1: 
						self.bmove = mv
						self.bscr = v
				if minV <= alpha:
					#print "mincu", minV, alpha, beta, depth
					return minV #cutoff
				if minV < beta:
					beta = minV
		#print("minou", minV, alpha, beta, depth)
		return minV

	def MaxValue(self, alpha, beta, depth):
		#print "maxin", alpha, beta, depth
		self.count += 1
		if depth > self.maxDepth:
			self.maxDepth = depth
		maxV = NINF
		movelist = self.aiBoard.generate_moves()
		movelist.sort(reverse=True)
		nmove = len(movelist)
		if nmove == 0:
			#pass
			if depth==1:
				self.bmove = None
				self.bscr = -1
		else: 
			#for mv in movelist:
			for i in range(min(MAXBREADTH, len(movelist))):
				mv = movelist[i][1]			
				self.aiBoard.make_move(mv) #make the move on the current board
				self.threat[depth] = self.aiBoard.trimade
				if depth < MAXDEPTH1 and (depth < MAXDEPTH2 or self.threat[depth]): #or self.threat[depth-1]):				 
					v = self.MinValue(alpha, beta, depth+1)
					if v == PINF:
						#no more moves from here
						v = self.aiBoard.get_score1()
				else:
					v = self.aiBoard.get_score1()
				self.aiBoard.remove(mv) #undo move
				if v > maxV: 
					maxV = v
					if depth==1: 
						self.bmove = mv
						self.bscr = v
				if maxV >= beta:
					return maxV #cutoff
				if maxV > alpha:
					alpha = maxV
		#print("maxou", maxV, alpha, beta, depth)					
		return maxV

	def ABSearch(self):
		#alpha beta dfs search
		self.bscr = 0
		self.bmove = None
		self.count = 0
		time1 = time.time()
		if self.aiBoard.currentPlayer == 1: 
			self.bscr = self.MaxValue(NINF,PINF,1)
		else:
			self.bscr = self.MinValue(NINF,PINF,1)
		time2 = time.time()
		laptime = round((time2 - time1), 2)
		print("time: ", str(laptime))
		print("count: ", self.count)
			
	def randMove(self):
		self.bmove = self.aiBoard.rand_move()
		self.bscr = 0
		return self.bmove
		
	def get_playout(self):
		mcboard = Board()	
		mcboard.clone(self.aiBoard)
		mcboard.playout2()
		return mcboard.get_score()
		
	def MonteCarloMove(self):
		"""
		monte carlo search 
		"""
		NREPS = 100
		self.bmove = None
		self.bscr = 0		
		mcboard = Board()
		smoves = self.aiBoard.generate_moves()
		fr = self.aiBoard.currentPlayer
		en = 3 - fr
		if len(smoves) == 0:
			return None
		elif len(smoves) == 1:
			self.bmove = smoves[0]
			return smoves[0]
		self.bscr = NINF
		for l1 in smoves:
			scr = 0
			for _ in range(NREPS):
				mcboard.clone(self.aiBoard)
				mcboard.make_move(l1)
				mcboard.playout2()
				scr += mcboard.score[fr] - mcboard.score[en]
			if scr > self.bscr:
				#print(self.bscr, scr, l1)
				self.bscr = scr
				self.bmove = l1
		return self.bmove

	def go(self,board0):
		#calc
		self.bmove = None
		self.bscr = 0
		self.maxDepth = 0
		self.threat = [False for _ in range(128)]
		self.threat[0] = True		
		self.aiBoard.clone(board0)
		#if self.aiBoard.turn < 0:
		#	self.randMove()
		#if self.aiBoard.currentPlayer == 1:
		self.ABSearch()
		#else:
		#self.MonteCarloMove()
		print (self.bscr, self.maxDepth)
		return self.bmove


