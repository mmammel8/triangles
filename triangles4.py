"""
Game of Triangles
v1.0 1/15/2023
MKM
"""
import random
import math
import time
from collections import namedtuple
from tkinter import *
import tboard4 as tb

Point = namedtuple('Point', 'x y')

#constants
NUMPOINTS = 20 #number of points to place
CWIDTH = 800 #width of board
CHEIGHT = 600 #height of board
BUFFERDIST = 80 #minimum distance between points
LWIDTH = 4 #line width
PRADIUS = 10 #point radius
CLICKRAD = 15 #within this of point to accept click

PINF = 1000000000
NINF = -1000000000

class TrianglesApp:
	"""
	application to play Triangles
	"""
	def __init__(self, parent):
		self.Parent = parent
		self.ai = tb.AI()
		self.Container1 = Frame(parent)
		self.Container1.pack()
		self.Board = tb.Board()
		self.players = [0,0,0] # 1 for computer
		self.label1 = None
		self.label2 = None
		self.players = [0,0,0]
		self.colors = ["gray", "yellow", "RoyalBlue1", "red", "green", "orange", "purple" ]
		self.lcolors = ["gray", "dark orange", "blue"]
		self.p1 = None
		self.p2 = None
		self.l1 = None
		self.drawing = BooleanVar()
 
		#new game button
		self.button1 = Button(self.Container1)
		self.button1["text"]= "New Game"
		self.button1["background"] = "yellow"
		self.button1.pack()
		self.button1.bind("<Button-1>", self.button1_click)

		#start AI button
		self.button2 = Button(self.Container1)
		self.button2.configure(text="AI", background="green")
		self.button2.pack(side=RIGHT)
		self.button2.bind("<Button-1>", self.button2_click)

		#quit button
		self.button3 = Button(self.Container1)
		self.button3.configure(text="Quit", background="red")
		self.button3.pack(side=RIGHT)
		self.button3.bind("<Button-1>", self.button3_click)

		#save button
		self.button4 = Button(self.Container1)
		self.button4.configure(text="Save", background="orange")
		self.button4.pack()
		self.button4.bind("<Button-1>", self.button4_click)

		#load button
		self.button5 = Button(self.Container1)
		self.button5.configure(text="Load", background="blue")
		self.button5.pack(side=RIGHT)
		self.button5.bind("<Button-1>", self.button5_click)
		
		#undo button
		self.button6 = Button(self.Container1)
		self.button6.configure(text="Undo", background="purple")
		self.button6.pack()
		self.button6.bind("<Button-1>", self.button6_click)		

		#canvas
		self.canvas = Canvas(parent, width=CWIDTH, height=CHEIGHT)
		self.canvas.pack()
		self.canvas.bind("<Button-1>", self.canvas_click)
		self.canvas.bind("<ButtonRelease-1>", self.canvas_release)

	def button1_click(self, event):
		tb.rand_start()
		tb.init()
		self.Board.clear_board()
		self.players = [0,0,0]
		self.draw()

	def button2_click(self, event):
		self.players[self.Board.currentPlayer] = 1 #auto move next time
		self.l1 = self.ai.go(self.Board)
		if self.l1 != None:
			self.Board.make_move(self.l1)
			self.display_move(self.l1)

	def button3_click(self, event):
		self.Parent.destroy()

	def button4_click(self, event):
		self.Board.exportGame()

	def button5_click(self, event):
		self.Board.importGame()
		self.draw()

	def button6_click(self, event):
		if self.Board.turn > 1:
			self.players = [0,0,0]
			self.Board.undo()
			self.draw()

	def canvas_click(self, event):
		ptc = Point(event.x, event.y)
		self.p1 = None
		for i in range(NUMPOINTS):
			if tb.ptdist(ptc, tb.points[i]) < CLICKRAD:
				self.p1 = i
				
	def canvas_release(self, event):
		ptc = Point(event.x, event.y)
		print(event.x, event.y)
		self.p2 = None
		for i in range(NUMPOINTS):
			if tb.ptdist(ptc, tb.points[i]) < CLICKRAD:
				self.p2 = i
		if self.p1 != None and self.p2 != None and self.p1 != self.p2:
			self.l1 = tb.linenum[self.p1][self.p2]
			if self.Board.legalline(self.l1):
				self.Board.make_move(self.l1)
				self.display_move(self.l1)

	def draw_line(self, line, color):
		pt1 = tb.points[tb.lines[line][0]]
		pt2 = tb.points[tb.lines[line][1]]
		self.canvas.create_line(pt1.x,pt1.y,pt2.x,pt2.y, width = LWIDTH, fill = self.lcolors[color])
		#redraw vertex
		self.canvas.create_oval(pt1.x+PRADIUS,pt1.y+PRADIUS,pt1.x-PRADIUS,pt1.y-PRADIUS, outline="", fill=self.colors[0])
		self.canvas.create_oval(pt2.x+PRADIUS,pt2.y+PRADIUS,pt2.x-PRADIUS,pt2.y-PRADIUS, outline="", fill=self.colors[0])
	
	def draw_triangle(self, triangle, color):
		pt1 = tb.points[tb.triangles[triangle][0]]
		pt2 = tb.points[tb.triangles[triangle][1]]
		pt3 = tb.points[tb.triangles[triangle][2]]
		self.canvas.create_polygon(pt1.x,pt1.y,pt2.x,pt2.y,pt3.x,pt3.y,pt1.x,pt1.y, outline="", fill=self.colors[color])
	
	def display_move(self, l1):
		player = 3 - self.Board.currentPlayer #already incremented player
		self.draw_line(l1, player)		
		for tr in tb.line2triangles[l1]:
			if self.Board.trifilled[tr] == 3:
				p1 = tb.triangles[tr][0]
				p2 = tb.triangles[tr][1]
				p3 = tb.triangles[tr][2]
				l1 = tb.linenum[p1][p2]		
				l2 = tb.linenum[p1][p3] 
				l3 = tb.linenum[p2][p3]
				self.draw_triangle(tr, player)
				self.draw_line(l1, self.Board.linecolor[l1]) 
				self.draw_line(l2, self.Board.linecolor[l2]) 
				self.draw_line(l3, self.Board.linecolor[l3]) 
				self.label1.configure(text=str(self.Board.score[1]))
				self.label2.configure(text=str(self.Board.score[2]))
		movelist = self.Board.generate_moves()
		print("turn", self.Board.turn, "len",len(movelist))
		if len(movelist) == 0:
			winner = self.Board.find_winner()
			self.label3.configure(text="Game Over", fg=self.lcolors[winner])		 
			self.label3.update()		   
		elif self.players[self.Board.currentPlayer] == 1:
			self.canvas.update()
			self.l1 = self.ai.go(self.Board)
			#self.l1 = int(input("l1"))
			if self.l1 != None:
				self.Board.make_move(self.l1) 
				self.display_move(self.l1)
				
	def draw(self):
		self.canvas.delete('all')
		for pt in tb.points:
			#self.canvas.create_rectangle(x1,y1,x2,y2, fill=colors[c1])
			self.canvas.create_oval(pt.x+PRADIUS,pt.y+PRADIUS,pt.x-PRADIUS,pt.y-PRADIUS, outline="", fill=self.colors[0])
		self.label1 = Label(text="0 ", fg=self.lcolors[1], font=("Helvetica", 18))
		self.label2 = Label(text="0 ", fg=self.lcolors[2], font=("Helvetica", 18))
		self.label3 = Label(text="		  ", font=("Helvetica", 18))
		self.label1.place(x=50,y=50)
		self.label2.place(x=CWIDTH-80,y=50)
		self.label3.place(x=150,y=70) 
		
		for tr in range(self.Board.numtri):
			p = self.Board.tricolor[tr] 
			if p > 0:		
				p1 = tb.triangles[tr][0]
				p2 = tb.triangles[tr][1]
				p3 = tb.triangles[tr][2]
				l1 = tb.linenum[p1][p2]		
				l2 = tb.linenum[p1][p3] 
				l3 = tb.linenum[p2][p3]
				self.draw_triangle(tr, p)
		for l1 in range(self.Board.numlines):
			p = self.Board.linecolor[l1] 
			if p > 0:
				self.draw_line(l1, p)		
		self.label1.configure(text=str(self.Board.score[1]))
		self.label2.configure(text=str(self.Board.score[2])) 
			
#main
#random.seed(42)
tb.rand_start()
tb.init()
root = Tk()
Trianglesapp = TrianglesApp(root)
Trianglesapp.draw()
root.mainloop()

