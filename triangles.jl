using Gtk, Graphics

#constants
NUMPOINTS = 20 #number of points to place
CWIDTH = 800 #width of board
CHEIGHT = 600 #height of board
BUFFERDIST = 80 #minimum distance between points
LWIDTH = 4 #line width
PRADIUS = 9 #point radius
CLICKRAD = 15 #within this of point to accept click
MAXDEPTH1 = 2 #limit depth of search in threat
MAXDEPTH2 = 2 #regular depth of search
MAXBREADTH = 24

struct Point
	x::Int
	y::Int
end

struct Move
	l1::Int
	scr::Float64
end

mutable struct GraphPoints
	points #list of points on the board
	num_points
end

mutable struct GraphLines
	linect #num_lines
	trict #num_triangles
	lines #list of lines in vector of pairs of point numbers
	triangles #list of triangles in vector of triples of point numbers
	linenum # [p1][p2]  number of line between p1 and p2 - array
	#trinum #[p1][p2][p3]  number of triangle with vert p1 p2 p3 - array
end

mutable struct GraphCross	
	line2triangles #lists triangles including this line - vector of variable length vectors
	crosslines # lines that cross this one - vector of variable length vectors
end
	
mutable struct Board
	currentPlayer
	turn
	winner
	score
	linecolor
	trifilled
	tricolor
	trimade
	history
	zhash
end

function ptdist(pa, pb)
	#return distance between two points
	dx = pb.x - pa.x
	dy = pb.y - pa.y
	return sqrt(dx*dx + dy*dy)
end

function pt_line_dist(pa, pb, pc)
	#return distance of pt c from line pa-pb
	dx = pb.x - pa.x
	dy = pb.y - pa.y
	num = dx*(pa.y-pc.y) - dy*(pa.x-pc.x)
	den = sqrt(dx*dx + dy*dy)
	return num / den
end

function side(pa, pb, pc)
	#returns 1 if point c is above line a-b
	#or -1 if below or 0 if on line.
	result = nothing
	if pa.x == pb.x
	#segment is vertical
		if pa.x == pc.x
			result = 0
		elseif pa.x < pc.x
			result = 1
		else
			result = -1
		end
	elseif pa.x == pc.x
	#point is vertical
		if pc.y > pa.y
			result = 1
		else
			result =-1
		end
	else
		m1 = (pb.y - pa.y)*(pc.x - pa.x)
		m2 = (pc.y - pa.y)*(pb.x - pa.x)
		if m2 > m1
			result = 1
		elseif m1 > m2
			result = -1
		else
			result = 0
		end
	end
	return result
end

function intersect(pa, pb, pc, pd)
	#determines if segment pa-pb intersects pc-pd
	#is true if segments share endpoint
	if pa.x > pb.x
		p1x, p2x = pb, pa
	else
		p1x, p2x = pa, pb
	end
	if pc.x > pd.x
		p3x, p4x = pd, pc
	else
		p3x, p4x = pc, pd
	end		
	if pa.y > pb.y
		p1y, p2y = pb, pa
	else
		p1y, p2y = pa, pb
	end		
	if pc.y > pd.y
		p3y, p4y = pd, pc
	else
		p3y, p4y = pc, pd
	end		

	#check not overlapping
	#print(p1x.y, p2x.y, p3x.y, p4x.y)
	result = true
	if p3x.x > p2x.x || p1x.x > p4x.x || p3y.y > p2y.y || p1y.y > p4y.y
		result = false
	else
		s1 = side(p3x, p4x, p1x)
		s2 = side(p3x, p4x, p2x)
		s3 = side(p1x, p2x, p3x)
		s4 = side(p1x, p2x, p4x)
		if s1==0 || s2==0 || s3==0 || s4==0
			result = true #endpoint on segment
		else
			result = (s1 != s2 && s3 != s4)
		end
	end
	return result
end

function sign(p1, p2, p3)
	#returns number indicating side of p1 on segment p2-p3
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y)
end

function point_in_triangle(pt, v1, v2, v3)
	#returns if pt in triangle defined by three verts
	d1 = sign(pt, v1, v2)
	d2 = sign(pt, v2, v3)
	d3 = sign(pt, v3, v1)
	has_neg = (d1 < 0 || d2 < 0 || d3 < 0)
	has_pos = (d1 > 0 || d2 > 0 || d3 > 0)
	return !(has_neg && has_pos)
end

function linelegal(pt1, pt2)
	#returns if pt1 to pt2 does not hit another point on board
	threshold = PRADIUS + 3#+ LWIDTH
	dx = pt2.x - pt1.x
	dy = pt2.y - pt1.y
	den = dx*dx + dy*dy
	lox = min(pt1.x, pt2.x) - PRADIUS
	hix = max(pt1.x, pt2.x) + PRADIUS
	loy = min(pt1.y, pt2.y) - PRADIUS
	hiy = max(pt1.y, pt2.y) + PRADIUS
	threshold = threshold * threshold * den
	result = true
	for i = 1:NUMPOINTS
		if gpoint.points[i] != pt1 && gpoint.points[i] != pt2 && gpoint.points[i].x >= lox && gpoint.points[i].x <= hix && gpoint.points[i].y >= loy && gpoint.points[i].y <= hiy
			num = dx*(pt1.y - gpoint.points[i].y) - dy*(pt1.x - gpoint.points[i].x)		
			if num * num < threshold
				#print(num*num, threshold, pt1, pt2, gpoint.points[i])
				result = false
			end
		end
	end
	return result
end

function trilegal(pt1, pt2, pt3)
	#returns if triangle does not contain another point
	result = true
	for pt0 in gpoint.points
		if pt0 != pt1 && pt0 != pt2 && pt0 != pt3
			if point_in_triangle(pt0, pt1, pt2, pt3)
				result = false
			end
		end
	end
	return result
end

function rand_start()
	# rand gen lists of points
	for i in 1:NUMPOINTS
		ok = false
		while !ok
			x = rand(BUFFERDIST: CWIDTH-BUFFERDIST-1)
			y = rand(BUFFERDIST: CHEIGHT-BUFFERDIST-1)
			pt = Point(x,y)
			ok = true
			for j in 1:i-1
				if ptdist(pt, gpoint.points[j]) < BUFFERDIST
					ok = false
				end
			end
			if ok
				gpoint.points[i] = pt
			end
		end
	end
end

function test_start()
	#points = [Point(15,45),Point(20,110),Point(90,130),Point(130,50),Point(75,20),Point(65,80)]
	gpoint.points = [Point(92,69),Point(169,49),Point(534,57),Point(248,79),Point(409,76),Point(150,160),Point(328,148),Point(516,161),Point(107,251),Point(185,287),Point(253,256),Point(384,225),Point(488,264),Point(331,313),Point(413,335),Point(68,400),Point(156,386),Point(266,398),Point(353,405),Point(507,394)]
end

function init1()
	#precompute lists of lines, triangles
	gline.linect = 1
	for i = 1:NUMPOINTS
		for j = i + 1:NUMPOINTS
			if linelegal(gpoint.points[i], gpoint.points[j])
				gline.linenum[i,j] = gline.linenum[j,i] = gline.linect
				push!(gline.lines, [i,j])
				gline.linect += 1
			end
		end
	end
	gline.trict = 1
	for i = 1:NUMPOINTS
		for j = i + 1:NUMPOINTS
			for k = j + 1:NUMPOINTS
				l1 = gline.linenum[i,j]
				l2 = gline.linenum[i,k]
				l3 = gline.linenum[j,k]
				if l1 > -1 && l2 > -1 && l3 > -1
					if trilegal(gpoint.points[i], gpoint.points[j], gpoint.points[k])
						 #gline.trinum[i,j,k] = gline.trinum[j,i,k] = gline.trict
						 #gline.trinum[i,k,j] = gline.trinum[j,k,i] = gline.trict
						 #gline.trinum[k,i,j] = gline.trinum[k,j,i] = gline.trict
						 push!(gline.triangles, [i,j,k])
						 gline.trict += 1
					end
				end
			end
		end
	end
	gline.linect-=1
	gline.trict-=1
end

function init2()
	#determine line crosses
	line_cross = trues(gline.linect, gline.linect)
	for l1 = 1:gline.linect
		pa = gpoint.points[gline.lines[l1][1]]
		pb = gpoint.points[gline.lines[l1][2]]
		for l2 = l1 + 1:gline.linect
			pc = gpoint.points[gline.lines[l2][1]]
			pd = gpoint.points[gline.lines[l2][2]]
			if pa == pc || pa == pd || pb == pc || pb == pd
				line_cross[l1,l2] = line_cross[l2,l1] = false
			else
				line_cross[l1,l2] = line_cross[l2,l1] = intersect(pa, pb, pc, pd)
			end
		end
	end
	for l1 = 1:gline.linect
		for l2 = 1:gline.linect
			if line_cross[l1,l2]
				push!(gcross.crosslines[l1], l2)
			end
		end
	end
	for t1 = 1:gline.trict
		p1 = gline.triangles[t1][1]
		p2 = gline.triangles[t1][2]
		p3 = gline.triangles[t1][3]
		l1 = gline.linenum[p1,p2]
		l2 = gline.linenum[p1,p3]
		l3 = gline.linenum[p2,p3]
		push!(gcross.line2triangles[l1], t1)
		push!(gcross.line2triangles[l2], t1)
		push!(gcross.line2triangles[l3], t1)
	end	
end

function clear_board(b1)
	#restore to beginning of game
	b1.currentPlayer = 1
	b1.turn = 1
	b1.winner = -1
	b1.score = [0,0,0]
	b1.linecolor = [0 for _ in 1:gline.linect]  #0, 1, 2 player
	b1.trifilled = [0 for _ in 1:gline.trict] #0,1,2,3 sides
	b1.tricolor = [0 for _ in 1:gline.trict] #0, 1, 2 player
	b1.trimade = false
	b1.history = []
end	

function legalline(b1, l1)
	#returns if line is a valid move
	result = true	
	if l1 == -1
		#println("-1")
		result = false #not valid line
	elseif b1.linecolor[l1] > 0
		#print(">0")
		result = false #already played
	else
		for l2 in gcross.crosslines[l1]
			if b1.linecolor[l2] > 0
				#println("X")
				result =  false #intersects a line on board
			end
		end
	end
	return result
end	

function find_winner(b1)
	#finds winning player when no moves available
	#sets b1winner
	if b1.score[1] > b1.score[2]
		b1.winner = 1
	elseif b1.score[2] > b1.score[1]
		b1.winner = 2
	else
		b1.winner = 0
	end
	return b1.winner
end	

function get_score1(b1)
	#evaluates the final board for player 1
	#returns score
	scr = b1.score[1] - b1.score[2] + rand(Float64) / 10.0
	return scr
end

function get_score2(b1, line1)
	#evaluates the line move
	#returns score
	value  = [1, 6, 100]
	scr = rand(Float64) / 10.0
	tf = 0
	for tr in gcross.line2triangles[line1]
		tf += value[b1.trifilled[tr]+1]
	end
	scr += tf
	return scr
end

function make_move(b1, line1)
	#makes the single move line
	#updates triangles
	b1.linecolor[line1] = b1.currentPlayer
	b1.trimade = false
	#b1.zhash ^= zobrist1[line1][b1.currentPlayer-1]
	for tr in gcross.line2triangles[line1]
		b1.trifilled[tr] += 1	
		if b1.trifilled[tr] == 3
			b1.tricolor[tr] = b1.currentPlayer
			b1.score[b1.currentPlayer] += 1
			b1.trimade = true
			#b1.zhash ^= zobrist2[tr][b1.currentPlayer-1]
		end
	end			
	b1.turn += 1
	push!(b1.history, line1)
	b1.currentPlayer = 3 - b1.currentPlayer
end

function remove(b1, line1)
	#removes the line
	#updates triangles
	b1.currentPlayer = 3 - b1.currentPlayer
	b1.linecolor[line1] = 0
	#b1.zhash ^= zobrist1[line1][b1.currentPlayer-1]
	for tr in gcross.line2triangles[line1]
		if b1.trifilled[tr] == 3
			b1.tricolor[tr] = 0
			b1.score[b1.currentPlayer] -= 1
			#b1.zhash ^= zobrist2[tr][b1.currentPlayer-1]
		end			
		b1.trifilled[tr] -= 1
	end
	deleteat!(b1.history, turn)
	b1.turn -= 1		
end

function undo(b1)
	#removes the line
	#updates triangles
	deleteat!(b1.history, turn)
	b1.turn -= 1
	b1.currentPlayer = 3 - b1.currentPlayer
	b1.linecolor[line1] = 0
	#b1.zhash ^= zobrist1[line1][b1.currentPlayer-1]
	for tr in gcross.line2triangles[line1]
		if b1.trifilled[tr] == 3
			b1.tricolor[tr] = 0
			b1.score[b1.currentPlayer] -= 1
			#b1.zhash ^= zobrist2[tr][b1.currentPlayer-1]
		end
		b1.trifilled[tr] -= 1
	end
end

function generate_moves(b1)
	#generates all legal moves for player
	smoves = Vector{Move}(undef, 0)
	for l1 in 1:gline.linect
		if legalline(b1, l1)
			scr = get_score2(b1, l1)
			push!(smoves, Move(l1, scr))
		end
	end
	return smoves
end

function rand_move(b1)
	#return a random legal move if any
	l1 = nothing
	smoves = generate_moves(b1)
	s = size(smoves)
	if s > 0
		l1 = smoves[rand(1:s)].l1
	end
	return l1
end
	
board1 = Board(0,0,0,[0,0,0],Vector{Int},Vector{Int},Vector{Int},false,Vector{Int},0)
gpoint = GraphPoints(Vector{Point}(undef, NUMPOINTS), NUMPOINTS)	
#rand_start()
test_start()

gline = GraphLines(0,0,Vector{Int}[],Vector{Int}[],fill(-1, (NUMPOINTS, NUMPOINTS)))
init1()
println(gline.linect, " lines, ", gline.trict, " triangles")

gcross = GraphCross([Vector{Int}(undef,0) for _ in 1:gline.linect], [Vector{Int}(undef,0) for _ in 1:gline.linect])	
init2()

#zobrist1 = [[0 for _ in range(2)] for _ in range(gline.linect)]
#zobrist2 = [[0 for _ in range(2)] for _ in range(gline.trict)]
#globals
players = [0,0]
pt1 = -1
clear_board(board1)

function draw_circle(widget, pt)
    ctx = getgc(widget)
    set_source_rgb(ctx, 0.6, 0.6, 0.6)
    arc(ctx, pt.x, pt.y, PRADIUS, 0, 2pi)
    #stroke(ctx)
    fill(ctx)
    reveal(widget)
end

function draw_line(widget, line, player)
    ctx = getgc(widget)
	pt1 = gpoint.points[gline.lines[line][1]]
	pt2 = gpoint.points[gline.lines[line][2]]    
    move_to(ctx, pt1.x, pt1.y)
	if player == 1
		set_source_rgb(ctx, 0.2, 0.2, 0.8)
	end
	if player == 2
		set_source_rgb(ctx, 0.8, 0.2, 0.2)
	end
    line_to(ctx, pt2.x, pt2.y)
	set_line_width(ctx, LWIDTH)  
    stroke(ctx)
end

function draw_triangle(widget, tr, player)
    ctx = getgc(widget)
	pt1 = gpoint.points[gline.triangles[tr][1]]
	pt2 = gpoint.points[gline.triangles[tr][2]]
	pt3 = gpoint.points[gline.triangles[tr][3]] 
	move_to(ctx, pt1.x, pt1.y)
	line_to(ctx, pt2.x, pt2.y)
	line_to(ctx, pt3.x, pt3.y)
	line_to(ctx, pt1.x, pt1.y)
	set_line_width(ctx, LWIDTH)
	if (player == 1)
		set_source_rgb(ctx, 0.1, 0.1, 0.6)
	end
	if (player == 2)
		set_source_rgb(ctx, 0.6, 0.1, 0.1)
	end
	fill_preserve(ctx)			
	stroke(ctx)
	#close_path(ctx);
	#stroke_preserve(ctx);
	#fill(ctx);	
end

function display_move(widget, l1)
	player = 3 - board1.currentPlayer #already incremented player
	draw_line(widget, l1, player)		
	for tr in gcross.line2triangles[l1]
		if board1.trifilled[tr] == 3
			p1 = gline.triangles[tr][1]
			p2 = gline.triangles[tr][2]
			p3 = gline.triangles[tr][3]
			l1 = gline.linenum[p1,p2]		
			l2 = gline.linenum[p1,p3] 
			l3 = gline.linenum[p2,p3]
			draw_triangle(widget, tr, player)
			draw_line(widget, l1, board1.linecolor[l1]) 
			draw_line(widget, l2, board1.linecolor[l2]) 
			draw_line(widget, l3, board1.linecolor[l3]) 
			#label1.configure(text=str(self.Board.score[1]))
			#label2.configure(text=str(self.Board.score[2]))
		end
	end
	reveal(widget)	
	smoves = generate_moves(board1)
	s = size(smoves,1)
	getscr(m::Move) = m.scr
	sort(smoves, by=getscr,  rev=true)	
	println("turn", board1.turn, " len", s)
	if s == 0
		winner = board1.find_winner()
		#label3.configure(text="Game Over", fg=self.lcolors[winner])		 
		#label3.update()		   
	elseif players[board1.currentPlayer] == 1
		#canvas.update()
		#l1 = ai.go(board1)
		#if l1 != None:
		#	make_move(board1, l1) 
		#	display_move(widget, l1)
	end
end

function hellogtkapp()
    # design gui layout

    win = GtkWindow("Triangles", 800,700)
    grid = GtkGrid()
    label1 = GtkLabel("score 0")
    label2 = GtkLabel("score 0")
    GAccessor.justify(label1, Gtk.GConstants.GtkJustification.CENTER)
    grid[1,1] = label1
    grid[4,1] = label2
    button1 = GtkButton("Quit")
    button2 = GtkButton("New Game")
    grid[2,1] = button1
    grid[3,1] = button2    
    button3 = GtkButton("Save")
    button4 = GtkButton("Load")
    grid[1,2] = button3
    grid[2,2] = button4 
    button5 = GtkButton("Stop")
    button6 = GtkButton("Go")               
    grid[3,2] = button5
    grid[4,2] = button6     
    button7 = GtkButton("Rewind")
    button8 = GtkButton("Back")
    grid[1,3] = button7
    grid[2,3] = button8    
    button9 = GtkButton("Forward")
    button10 = GtkButton("FFwd")
    grid[3,3] = button9
    grid[4,3] = button10     

    
    canv = @GtkCanvas()
    set_gtk_property!(canv, :expand, true)
    grid[1:4,4] = canv  
	push!(win, grid)
    showall(win)
    
    @guarded draw(canv) do widget
        ctx = getgc(canv)
        h = height(canv)
        w = width(canv)
        println(h," by ",w)
        
		for pt in gpoint.points
			draw_circle(widget, pt)  
		end  
		for tr = 1:gline.trict
			p = board1.tricolor[tr] 
			if p > 0	
				p1 = gline.triangles[tr][1]
				p2 = gline.triangles[tr][2]
				p3 = gline.triangles[tr][3]
				l1 = gline.linenum[p1,p2]		
				l2 = gline.linenum[p1,p3] 
				l3 = gline.linenum[p2,p3]
				draw_triangle(widget, tr, p)
			end
		end
		for l1 = 1:gline.linect
			p = board1.linecolor[l1] 
			if p > 0
				draw_line(widget, l1, p)
			end
		end        
        # Paint red rectangle
        #rectangle(ctx, 0, 0, w, h/2)
        #set_source_rgb(ctx, 1, 0, 0)
        #fill(ctx)
    end
    
	id = signal_connect(button1, "clicked") do widget
		if board1.rewind()
			clear_surface()
			players[1] = 0
			players[2] = 0
		end
	end
	id = signal_connect(button2, "clicked") do widget
		 println(widget, " was clicked!")
	end
	id = signal_connect(button3, "clicked") do widget
		 println(widget, " was clicked!")
	end
	id = signal_connect(button4, "clicked") do widget
		 println(widget, " was clicked!")
	end
	id = signal_connect(button5, "clicked") do widget
		 println(widget, " was clicked!")
	end
	id = signal_connect(button6, "clicked") do widget
		 println(widget, " was clicked!")
	end
	id = signal_connect(button7, "clicked") do widget
		if board1.rewind()
			clear_surface()
			players[1] = 0
			players[2] = 0
		end
	end
	id = signal_connect(button8, "clicked") do widget
		if board1.ffw()
			clear_surface()
			players[1] = 0
			players[2] = 0
		end
	end
	id = signal_connect(button9, "clicked") do widget
		 println(widget, " was clicked!")
	end
	id = signal_connect(button10, "clicked") do widget
		 println(widget, " was clicked!")
	end									
        
    canv.mouse.button1press = @guarded (widget, event) -> begin
		ptc = Point(round(Int, event.x), round(Int, event.y))
		global pt1 = -1	
		for i in 1:NUMPOINTS
			if ptdist(ptc, gpoint.points[i]) < CLICKRAD
				pt1 = i
			end
		end		
	end
    
    canv.mouse.button1release = @guarded (widget, event) -> begin
		ptc = Point(round(Int, event.x), round(Int, event.y))
		pt2 = -1
		for i in 1:NUMPOINTS
			if ptdist(ptc, gpoint.points[i]) < CLICKRAD
				pt2 = i
			end
		end
		if pt1 > -1 && pt2 > -1 && pt1 != pt2
			l1 = gline.linenum[pt1, pt2]
			if legalline(board1, l1)
				make_move(board1, l1)
				display_move(widget, l1)
			end
		end		
	end    

    while true
        println("(hit Enter to end session)")
        input = readline()
        if input == ""
            break
        end
    end    

end

function testfunctions()
	p1 = Point(1,1)
	p2 = Point(2,6)
	p3 = Point(3,5)
	p4 = Point(4,1)
	p5 = Point(6,3)
	p6 = Point(8,5)
	println(ptdist(p1,p2)) #5.0990195135927845
	println(pt_line_dist(p1, p2, p3)) #1.1766968108291043

	println(side(p1, p2, p3)) #-1
	println(side(p1, p3, p2)) # 1
	println(side(p4, p5, p6)) # 0
	#returns 1 if point c is above line a-b
	#or -1 if below or 0 if on line.
	println(intersect(p1, p3, p2, p4)) #t
	println(intersect(p1, p3, p2, p6)) #f
	println(intersect(p4, p5, p5, p6)) #t
	println(intersect(p1, p4, p2, p5)) #f

	println(point_in_triangle(p3, p2, p4, p6)) #t
	println(point_in_triangle(p1, p2, p4, p6)) #f
	println(point_in_triangle(p5, p2, p4, p6)) #t

	println(sign(p2, p1, p3)) #+
	println(sign(p4, p1, p3)) #-
	#returns number indicating side of p1 on segment p2-p3
	
	gpoint.points[1] = p5
	println(linelegal(p4, p6))#f
	
end

hellogtkapp()
#testfunctions()


