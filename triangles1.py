"""
Game of Triangles
v1.0 1/15/2023
MKM
"""
import random
import math
import time
from tkinter import *
from collections import namedtuple

#constants
NUMPOINTS = 20 #number of points to place
CWIDTH = 800 #width of board
CHEIGHT = 600 #height of board
BUFFERDIST = 80 #minimum distance between points
LWIDTH = 4 #line width
PRADIUS = 10 #point radius
CLICKRAD = 15 #within this of point to accept click
MAXDEPTH = 128 #limit depth of search

PINF = 1000000
NINF = -1000000

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
    return SQUARE of distance of pt c from line pa-pb
    """
    dx = pb.x - pa.x
    dy = pb.y - pa.y
    num = dx*(pa.y-pc.y) - dy*(pa.x-pc.x)
    den = dx*dx + dy*dy
    return num*num / den
    
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
    threshold = PRADIUS + LWIDTH
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
            
def init():
    """
    precompute lists of points, lines, triangles    
    """
    global points, lines, triangles, linenum, trinum, line2triangles, line_cross 
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
        self.linefilled = []
        self.trifilled = []
        self.trimade = False
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
        self.linefilled = [0 for _ in range(self.numlines)]  #0, 1, 2 player
        self.trifilled = [0 for _ in range(self.numtri)] #0,1,2,3 sides   
        self.trimade = False

    def clone(self, board0):
        """
        copy game state from board0
        """
        self.numlines = board0.numlines
        self.numtri = board0.numtri         
        self.currentPlayer = board0.currentPlayer
        self.turn = board0.turn
        self.winner = board0.winner
        self.score = list(board0.score)
        self.linefilled = list(board0.linefilled)
        self.trifilled = list(board0.trifilled)

    def legalline(self, l1):
        """
        returns if line is a valid move
        """
        if l1 == -1:
            #print("-1")
            return False #not valid line
        if self.linefilled[l1] > 0:
            #print(">0")        
            return False #already played
        for l2 in range(self.numlines):
            if self.linefilled[l2]:
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

    def get_score(self):
        """
        evaluates the board for player 1
        returns score
        """
        scr = 0.0
        #vals = np.array([pawn, king, cent, kcent, adv, back, thret, mob, deny, mobil])
        #vals = [pawn, king, cent, kcent, adv, back, thret, mob, deny, mobil]
        #score = np.dot(self.theta, vals)
        #score = 0.0
        #for idx in range(len(vals)):
        scr = self.score[1] - self.score[2]
        return scr

    def make_move(self, line1):
        """
        makes the single move line
        updates triangles
        """
        self.linefilled[line1] = self.currentPlayer        
        for tr in line2triangles[line1]:
            self.trifilled[tr] += 1
            if self.trifilled[tr] == 3:
                self.score[self.currentPlayer] += 1
                self.trimade = True
            else:
                self.trimade = False
        self.turn += 1
        self.currentPlayer = 3 - self.currentPlayer

    def remove(self, line1):
        """
        makes the single move line
        updates triangles
        """
        self.turn -= 1
        self.currentPlayer = 3 - self.currentPlayer        
        self.linefilled[line1] = 0        
        for tr in line2triangles[line1]:
            if self.trifilled[tr] == 3:
                self.score[self.currentPlayer] -= 1     
            self.trifilled[tr] -= 1
   
        
    def generate_moves(self):
        """
        generates all legal moves for player
        """
        smoves = []
        for l1 in range(self.numlines):
            if self.legalline(l1):
                smoves.append(l1)
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
        MAXMV = 12
        mv = 0
        l1 = 0
        while l1 != None and mv < MAXMV:
            l1 = self.rand_move()
            if l1 != None:
                self.make_move(l1)
            mv += 1
           
 
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
 
    def MinValue(self, alpha, beta, depth):
        #print "minin", alpha, beta, depth
        if depth > self.maxDepth:
            self.maxDepth = depth
        minV = PINF
        movelist = self.aiBoard.generate_moves()
        nmove = len(movelist)
        #print "minmv", nmove
        if nmove == 0:
            #pass
            if depth==1:
                self.bmove = None
                self.bscr = -1
        else: 
            for mv in movelist:
                self.aiBoard.make_move(mv) #make the move on the current board
                self.threat[depth] = self.aiBoard.trimade
                if depth < MAXDEPTH and (self.threat[depth] or self.threat[depth-1]): 
                    v = self.MaxValue(alpha, beta, depth+1)
                    if v == NINF:
                        #no more moves from here
                        v = self.aiBoard.get_score()
                else:
                    v = self.aiBoard.get_score() + self.get_playout() / 20.0
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
        #print "minou", minV, alpha, beta, depth
        return minV

    def MaxValue(self, alpha, beta, depth):
        #print "maxin", alpha, beta, depth
        if depth > self.maxDepth:
            self.maxDepth = depth
        maxV = NINF
        movelist = self.aiBoard.generate_moves()
        nmove = len(movelist)
        if nmove == 0:
            #pass
            if depth==1:
                self.bmove = None
                self.bscr = -1
        else: 
            for mv in movelist:
                self.aiBoard.make_move(mv) #make the move on the current board
                self.threat[depth] = self.aiBoard.trimade
                if depth < MAXDEPTH and (self.threat[depth] or self.threat[depth-1]): 
                    v = self.MinValue(alpha, beta, depth+1)
                    if v == PINF:
                        #no more moves from here
                        v = self.aiBoard.get_score()
                else:
                    v = self.aiBoard.get_score() + self.get_playout() / 20.0
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
        return maxV

    def ABSearch(self):
        #alpha beta dfs search
        self.bscr = 0
        self.bmove = None
        if self.aiBoard.currentPlayer == 1: 
            self.bscr = self.MaxValue(NINF,PINF,1)
        else:
            self.bscr = self.MinValue(NINF,PINF,1)
            
    def randMove(self):
        self.bmove = self.aiBoard.rand_move()
        self.bscr = 0
        return self.bmove
        
    def get_playout(self):
        mcboard = Board()    
        mcboard.clone(self.aiBoard)
        mcboard.playout()
        return mcboard.get_score()
        
    def MonteCarloMove(self):
        """
        monte carlo search 
        """
        NREPS = 20
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
                mcboard.playout()
                scr += mcboard.score[fr] - mcboard.score[en]
                print(mcboard.score)
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
        if self.aiBoard.turn < 3:
            self.randMove()
        else:
            self.ABSearch()
        #
        #self.MonteCarloMove()
        print (self.bscr, self.maxDepth)
        return self.bmove

class TrianglesApp:
    """
    application to play Triangles
    """
    def __init__(self, parent):
        self.Parent = parent
        self.ai = AI()
        self.Container1 = Frame(parent)
        self.Container1.pack()
        self.Board = Board()
        self.players = [0,0,0] # 1 for computer
        self.label1 = None
        self.label2 = None
        self.players = [0,0,0]
        self.colors = ["gray", "RoyalBlue1", "yellow", "red", "green", "orange", "purple" ]
        self.lcolors = ["gray", "blue", "dark orange"]
        self.p1 = None
        self.p2 = None
        self.l1 = None
 
        #new game button
        self.button1 = Button(self.Container1)
        self.button1["text"]= "New Game"
        self.button1["background"] = "yellow"
        self.button1.pack()
        self.button1.bind("<Button-1>", self.button1_click)

        #start AI button
        self.button2 = Button(self.Container1)
        self.button2.configure(text="AI", background="green")
        self.button2.pack()
        self.button2.bind("<Button-1>", self.button2_click)

        #quit button
        self.button3 = Button(self.Container1)
        self.button3.configure(text="Quit", background="red")
        self.button3.pack()
        self.button3.bind("<Button-1>", self.button3_click)

        #canvas
        self.canvas = Canvas(parent, width=CWIDTH, height=CHEIGHT)
        self.canvas.pack()
        self.canvas.bind("<Button-1>", self.canvas_click)
        self.canvas.bind("<ButtonRelease-1>", self.canvas_release)

    def button1_click(self, event):
        init()
        self.Board.clear_board()
        self.players = [0,0,0]
        self.draw()

    def button2_click(self, event):
        self.players[self.Board.currentPlayer] = 1 #auto move next time
        self.l1 = ai.go(self.Board)
        if self.l1 != None:
            self.Board.make_move(self.l1)
            self.display_move(self.l1)

    def button3_click(self, event):
        self.Parent.destroy()

    def canvas_click(self, event):
        ptc = Point(event.x, event.y)
        self.p1 = None
        for i in range(NUMPOINTS):
            if ptdist(ptc, points[i]) < CLICKRAD:
                self.p1 = i
                
    def canvas_release(self, event):
        ptc = Point(event.x, event.y)
        self.p2 = None
        for i in range(NUMPOINTS):
            if ptdist(ptc, points[i]) < CLICKRAD:
                self.p2 = i
        if self.p1 != None and self.p2 != None and self.p1 != self.p2:
            self.l1 = linenum[self.p1][self.p2]
            if self.Board.legalline(self.l1):
                self.Board.make_move(self.l1)
                self.display_move(self.l1)

    def draw_line(self, line, color):
        pt1 = points[lines[line][0]]
        pt2 = points[lines[line][1]]
        self.canvas.create_line(pt1.x,pt1.y,pt2.x,pt2.y, width = LWIDTH, fill = self.lcolors[color])
        #redraw vertex
        self.canvas.create_oval(pt1.x+PRADIUS,pt1.y+PRADIUS,pt1.x-PRADIUS,pt1.y-PRADIUS, outline="", fill=self.colors[0])
        self.canvas.create_oval(pt2.x+PRADIUS,pt2.y+PRADIUS,pt2.x-PRADIUS,pt2.y-PRADIUS, outline="", fill=self.colors[0])
    
    def draw_triangle(self, triangle, color):
        pt1 = points[triangles[triangle][0]]
        pt2 = points[triangles[triangle][1]]
        pt3 = points[triangles[triangle][2]]
        self.canvas.create_polygon(pt1.x,pt1.y,pt2.x,pt2.y,pt3.x,pt3.y,pt1.x,pt1.y, outline="", fill=self.colors[color])
    
    def display_move(self, l1):
        player = 3 - self.Board.currentPlayer #already incremented player
        self.draw_line(l1, player)        
        for tr in line2triangles[l1]:
            if self.Board.trifilled[tr] == 3:
                p1 = triangles[tr][0]
                p2 = triangles[tr][1]
                p3 = triangles[tr][2]
                l1 = linenum[p1][p2]        
                l2 = linenum[p1][p3] 
                l3 = linenum[p2][p3]
                self.draw_triangle(tr, player)
                self.draw_line(l1, self.Board.linefilled[l1]) 
                self.draw_line(l2, self.Board.linefilled[l2]) 
                self.draw_line(l3, self.Board.linefilled[l3]) 
                self.label1.configure(text=str(self.Board.score[1]))
                self.label2.configure(text=str(self.Board.score[2]))
        movelist = self.Board.generate_moves()
        if len(movelist) == 0:
            winner = self.Board.find_winner()
            self.label3 = Label(text="Game Over", fg=self.lcolors[winner], font=("Helvetica", 18))
            self.label3.place(x=150,y=70)            
        elif self.players[self.Board.currentPlayer] == 1:
            self.l1 = self.ai.go(self.Board)
            if self.l1 != None:
                self.Board.make_move(self.l1) 
                self.display_move(self.l1)
                
    def draw(self):
        self.canvas.delete('all')
        for pt in points:
            #self.canvas.create_rectangle(x1,y1,x2,y2, fill=colors[c1])
            self.canvas.create_oval(pt.x+PRADIUS,pt.y+PRADIUS,pt.x-PRADIUS,pt.y-PRADIUS, outline="", fill=self.colors[0])
        self.label1 = Label(text="0 ", fg=self.lcolors[1], font=("Helvetica", 18))
        self.label2 = Label(text="0 ", fg=self.lcolors[2], font=("Helvetica", 18))
        self.label1.place(x=50,y=50)
        self.label2.place(x=CWIDTH-80,y=50)
            
#main
#random.seed(42)
init()
root = Tk()
ai = AI()
Trianglesapp = TrianglesApp(root)
Trianglesapp.draw()
root.mainloop()

