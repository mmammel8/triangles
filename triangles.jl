using Gtk, Graphics

#constants
numpoints::UInt16 = 20 #number of points to place
cwidth::Int32 = 800 #width of board
cheight::Int32 = 600 #height of board
bufferdist::Int32 = 80 #minimum distance between points
lwidth::Int32 = 4 #line width
pradius::Int32 = 9 #point radius
clickrad::Int32 = 15 #within this of point to accept click
maxdepth1::UInt16 = 12 #limit depth of search in threat
maxdepth2::UInt16 = 3 #regular depth of search
maxbreadth::UInt16 = 512
pinf::Float64 = 1000000000.0
ninf::Float64 = -1000000000.0
eps::Float64 = 1000000.0

struct Point
	x::Int32
	y::Int32
end

struct Move
	l1::UInt16
	scr::Float64
end

mutable struct GraphPoints
	points::Vector{Point}
end

mutable struct GraphLines
	linect::Int #num_lines
	trict::Int #num_triangles
	lines::Vector{Vector{UInt16}} #list of lines in vector of pairs of point numbers
	triangles::Vector{Vector{UInt16}} #list of triangles in vector of triples of point numbers
	linenum::Array{UInt16} # array[p1][p2]  number of line between p1 and p2
	#trinum #[p1][p2][p3]  number of triangle with vert p1 p2 p3 - array
end


mutable struct GraphCross	
	line2triangles::Vector{Vector{UInt16}} #lists triangles including this line 
	crosslines::Vector{Vector{UInt16}} # lines that cross this one 
end
	
mutable struct Board
	currentPlayer::UInt16
	turn::UInt16
	winner::UInt16
	score::Vector{Float64}
	linecolor::Vector{UInt16} #vector[linect]
	trifilled::Vector{UInt16} #vector[trict]
	tricolor::Vector{UInt16} #vector[trict]
	trimade::Bool
	lastturn::UInt16
	history::Vector{UInt16} #vector[linect]
	zhash::UInt32
end

mutable struct Node
	#A node in the game tree. Note wins is always from the viewpoint of playerJustMoved.
	move::UInt16 # = move the move that got us to this node
	player::UInt16	
	parentNode::Node # = parent "nothing" for the root node
	childNodes::Vector{Node} 
	qscore::Float64
	nvisits::Float64
	untriedMoves::Vector{UInt16} # = state.GetMoves() # future child nodes
	depth::UInt16
	Node() = (x = new(); x.parentNode = x)
end

mutable struct AIstate
	bmove::UInt16
	bscr::Float64
	maxDepth::UInt16
	threat::Vector{Bool}
	count::UInt32
	zobrist1::Array{UInt32}
	zobrist2::Array{UInt32}
end

function ptdist(pa::Point, pb::Point)
	#return distance between two points
	dx::Int32 = pb.x - pa.x
	dy::Int32 = pb.y - pa.y
	return sqrt(dx*dx + dy*dy)
end

function pt_line_dist(pa::Point, pb::Point, pc::Point)
	#return distance of pt c from line pa-pb
	dx::Int32 = pb.x - pa.x
	dy::Int32 = pb.y - pa.y
	num::Float64 = dx*(pa.y-pc.y) - dy*(pa.x-pc.x)
	den::Float64 = sqrt(dx*dx + dy*dy)
	return num / den
end

function side(pa::Point, pb::Point, pc::Point)
	#returns 1 if point c is above line a-b
	#or -1 if below or 0 if on line.
	result::Int = 0
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

function intersect(pa::Point, pb::Point, pc::Point, pd::Point)
	#determines if segment pa-pb intersects pc-pd
	#is true if segments share endpoint
	if pa.x > pb.x
		p1x, p2x = pb, pa
	else
		p1x, p2x = pa, pb
	end
	if pc.x > pd.x
		p3x, p4x= pd, pc
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

function sign(pt1::Point, pt2::Point, pt3::Point)
	#returns number indicating side of p1 on segment p2-p3
	return (pt1.x - pt3.x) * (pt2.y - pt3.y) - (pt2.x - pt3.x) * (pt1.y - pt3.y)
end

function pointInTriangle(pt::Point, v1::Point, v2::Point, v3::Point)
	#returns if pt in triangle defined by three verts
	d1 = sign(pt, v1, v2)
	d2 = sign(pt, v2, v3)
	d3 = sign(pt, v3, v1)
	has_neg = (d1 < 0 || d2 < 0 || d3 < 0)
	has_pos = (d1 > 0 || d2 > 0 || d3 > 0)
	return !(has_neg && has_pos)
end

function lineLegal(pt1::Point, pt2::Point)
	#returns if pt1 to pt2 does not hit another point on board
	threshold::Int32 = pradius + 3#+ lwidth
	dx::Int32 = pt2.x - pt1.x
	dy::Int32 = pt2.y - pt1.y
	den::Int32 = dx*dx + dy*dy
	lox = min(pt1.x, pt2.x) - pradius
	hix = max(pt1.x, pt2.x) + pradius
	loy = min(pt1.y, pt2.y) - pradius
	hiy = max(pt1.y, pt2.y) + pradius
	threshold = threshold * threshold * den
	result = true
	for i = 1:numpoints
		if gpoint.points[i] != pt1 && gpoint.points[i] != pt2 && gpoint.points[i].x >= lox && gpoint.points[i].x <= hix && gpoint.points[i].y >= loy && gpoint.points[i].y <= hiy
			num::Int32 = dx*(pt1.y - gpoint.points[i].y) - dy*(pt1.x - gpoint.points[i].x)		
			if num * num < threshold
				#print(num*num, threshold, pt1, pt2, gpoint.points[i])
				result = false
			end
		end
	end
	return result
end

function trilegal(pt1::Point, pt2::Point, pt3::Point)
	#returns if triangle does not contain another point
	result = true
	for pt0 in gpoint.points
		if pt0 != pt1 && pt0 != pt2 && pt0 != pt3
			if pointInTriangle(pt0, pt1, pt2, pt3)
				result = false
			end
		end
	end
	return result
end

function randStart(gpoint)
	# rand gen lists of points
	for i = 1:numpoints
		ok = false
		while !ok
			x = rand(bufferdist: cwidth-bufferdist-1)
			y = rand(bufferdist: cheight-bufferdist-1)
			pt::Point = Point(x,y)
			ok = true
			for j = 1:i-1
				if ptdist(pt, gpoint.points[j]) < bufferdist
					ok = false
				end
			end
			if ok
				gpoint.points[i] = pt
			end
		end
	end
end

function testStart(gpoint)
	#points = [Point(15,45),Point(20,110),Point(90,130),Point(130,50),Point(75,20),Point(65,80)]
	gpoint.points = [Point(92,69),Point(169,49),Point(534,57),Point(248,79),Point(409,76),Point(150,160),Point(328,148),Point(516,161),Point(107,251),Point(185,287),Point(253,256),Point(384,225),Point(488,264),Point(331,313),Point(413,335),Point(68,400),Point(156,386),Point(266,398),Point(353,405),Point(507,394)]
end

function init1(gline)
	#precompute lists of lines, triangles
	gline.linect = 1
	for i::UInt16 = 1:numpoints
		for j::UInt16 = i + 1:numpoints
			if lineLegal(gpoint.points[i], gpoint.points[j])
				gline.linenum[i,j] = gline.linenum[j,i] = gline.linect
				push!(gline.lines, [i,j])
				gline.linect += 1
			end
		end
	end
	gline.trict = 1
	for i::UInt16 = 1:numpoints
		for j::UInt16 = i + 1:numpoints
			for k::UInt16 = j + 1:numpoints
				l1 = gline.linenum[i,j]
				l2 = gline.linenum[i,k]
				l3 = gline.linenum[j,k]
				if l1 > 0 && l2 > 0 && l3 > 0
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

function init2(gcross)
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

function clearBoard(b1::Board)
	#restore to beginning of game
	b1.currentPlayer = 1
	b1.turn = 1
	b1.winner = 0
	b1.score = [0.0,0.0]
	b1.linecolor = [0 for _ in 1:gline.linect]  #0, 1, 2 player
	b1.trifilled = [0 for _ in 1:gline.trict] #0,1,2,3 sides
	b1.tricolor = [0 for _ in 1:gline.trict] #0, 1, 2 player
	b1.trimade = false
	b1.lastturn = 0
	b1.history = [0 for _ in 1:gline.linect]
	b1.zhash = 0
end	

function copyBoard(b1::Board, b2::Board)
	#copy b2 to b1
	b1.currentPlayer = b2.currentPlayer
	b1.turn = b2.turn
	b1.winner = b2.winner
	b1.score = copy(b2.score)
	b1.linecolor = copy(b2.linecolor)
	b1.trifilled = copy(b2.trifilled)
	b1.tricolor = copy(b2.tricolor)
	b1.trimade = b2.trimade	
	b1.lastturn = b2.lastturn
	b1.history = copy(b2.history)
	b1.zhash = b2.zhash
end	

function legalLine(b1::Board, l1::UInt16)
	#returns if line is a valid move
	result = true	
	if l1 == 0
		println("-1")
		result = false #not valid line
	elseif b1.linecolor[l1] > 0
		#print(">0")
		result = false #already played
	else
		for l2::UInt16 in gcross.crosslines[l1]
			if b1.linecolor[l2] > 0
				#println("X")
				result =  false #intersects a line on board
			end
		end
	end
	return result
end	

function findWinner(b1::Board)
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

function getScore1(b1::Board)
	#evaluates the final board for player 1
	#returns score
	scr = b1.score[1] - b1.score[2] + rand(Float64) / 10.0
	return scr
end

function getScore2(b1::Board, line1::UInt16)
	#evaluates the line move
	#returns score
	value = [1., 6., 100.]
	scr = rand(Float64) / 10.0
	tf = 0.0
	for tr in gcross.line2triangles[line1]
		tf += value[b1.trifilled[tr]+1]
	end
	scr += tf
	return scr
end

function rewind(b1::Board)
	savelast = b1.lastturn
	if b1.turn < 2
		return false
	end	
	while b1.turn > 1
		line1 = b1.history[b1.turn - 1]
		remove(b1, line1)
	end
	b1.lastturn = savelast
	return true
end

function ffwd(b1::Board)
	if b1.turn > b1.lastturn
		return false
	end	
	while b1.turn <= b1.lastturn
		savelast = b1.lastturn	
		line1 = b1.history[b1.turn]
		makeMove(b1, line1)
		b1.lastturn = savelast
	end
	return true
end

function back(b1::Board)
	if b1.turn < 2
		return false
	end
	line1 = b1.history[b1.turn - 1]
	savelast = b1.lastturn
	remove(b1, line1)
	b1.lastturn = savelast
	return true
end

function ahead(b1::Board)
	if b1.turn > b1.lastturn
		return false
	end
	line1 = b1.history[b1.turn]
	savelast = b1.lastturn
	makeMove(b1, line1)
	b1.lastturn = savelast
	return true
end

function makeMove(b1::Board, line1::UInt16)
	#makes the single move line
	#updates triangles
	b1.linecolor[line1] = b1.currentPlayer
	b1.trimade = false
	b1.zhash ^= ai1.zobrist1[line1, b1.currentPlayer]
	for tr in gcross.line2triangles[line1]
		b1.trifilled[tr] += 1	
		if b1.trifilled[tr] == 3
			b1.tricolor[tr] = b1.currentPlayer
			b1.score[b1.currentPlayer] += 1
			b1.trimade = true
			b1.zhash ^= ai1.zobrist2[tr, b1.currentPlayer]
		end
	end	
	b1.lastturn = b1.turn
	b1.history[b1.turn] =  line1		
	b1.turn += 1
	b1.currentPlayer = 3 - b1.currentPlayer
end

function remove(b1::Board, line1::UInt16)
	#removes the line
	#updates triangles
	b1.currentPlayer = 3 - b1.currentPlayer
	b1.linecolor[line1] = 0
	b1.zhash ^= ai1.zobrist1[line1, b1.currentPlayer]
	for tr in gcross.line2triangles[line1]
		if b1.trifilled[tr] == 3
			b1.tricolor[tr] = 0
			b1.score[b1.currentPlayer] -= 1
			b1.zhash ^= ai1.zobrist2[tr, b1.currentPlayer]
		end			
		b1.trifilled[tr] -= 1
	end
	b1.turn -= 1
	b1.lastturn = b1.turn - 1
end

function generateMoves(b1::Board)
	#generates all legal moves for player
	smoves = Vector{Move}(undef, 0)
	for l1::UInt16 in 1:gline.linect
		if legalLine(b1, l1)
			scr = getScore2(b1, l1)
			push!(smoves, Move(l1, scr))
		end
	end
	return smoves
end

function randMove(b1::Board)
	#return a random legal move if any
	l1::UInt16 = 0
	smoves = generateMoves(b1)
	s = size(smoves,1)
	if s > 0
		l1 = smoves[rand(1:s)].l1
	end
	return l1
end

function playout(b1::Board)
	maxmv::UInt16 = 128
	mv::UInt16 = 0
	l1::UInt16 = 0
	while l1 != 0 && mv < maxmv
		l1 = randMove(b1)
		if l1 != 0
			makeMove(b1, l1)
		end
		mv += 1
	end
end

function nodeInit(onode, state)
	onode.untriedMoves = [ m.l1 for m in generateMoves(state) ]
	onode.player = state.currentPlayer
end
		
function uctSelectChild(onode, ck::Float64)
	maxchild::UInt16 = 0
	maxval = -100000.0
	logterm = 2.0 * log(onode.nvisits)
	for cindex = 1:length(onode.childNodes)
		c = onode.childNodes[cindex]
		val = c.qscore/c.nvisits + ck * sqrt(logterm/c.nvisits)
		if val > maxval
			maxchild = cindex
			maxval = val
		end
	end
	return onode.childNodes[maxchild]
end

function addChild(onode, m::UInt16, s)
	#Return child node
	#n = Node(m, 0, onode, Node[], 0, 0, UInt16[], onode.depth + 1)
	n::Node = Node()
	n.move = m
	n.player = 0
	n.parentNode = onode
	n.childNodes = Node[]
	n.qscore = 0.0
	n.nvisits = 0.0
	n.untriedMoves = UInt16[]
	n.depth = onode.depth + 1
	nodeInit(n, s)
	push!(onode.childNodes, n)
	if onode.depth + 1 > ai1.maxDepth
		ai1.maxDepth = onode.depth + 1
	end	
	return n
end
	
function nodeUpdate(onode, result::Float64)
	onode.nvisits += 1.0
	onode.qscore += result
end

function printnode(onode)
	return "[M:" * string(onode.move) * " W/V:" * string(onode.qscore) * "/" * string(onode.nvisits) * "]"
end

function uct(rootstate, itermax::Int)
	#rootnode = Node(0, 0, nothing, Node[], 0, 0, UInt16[], 0)
	rootnode::Node = Node()
	rootnode.move = 0
	rootnode.player = 0
	rootnode.childNodes = Node[]
	rootnode.qscore = 0.0
	rootnode.nvisits = 0.0
	rootnode.untriedMoves = UInt16[]
	rootnode.depth = 0
	
	state = Board(0,0,0,[0.0,0.0],Vector{UInt16}[],Vector{UInt16}[],Vector{UInt16}[],false,0,Vector{UInt16}[],0)	
	nodeInit(rootnode, rootstate)

	for i = 1:itermax
		node = rootnode
		copyBoard(state, rootstate)
		# Select while node is fully expanded and non-terminal
		while node.untriedMoves == [] && node.childNodes != []
			node = uctSelectChild(node, 1.4)	
			makeMove(state, node.move)
		end
		# Expand
		if node.untriedMoves != []
			mi = rand(1:length(node.untriedMoves))
			m = node.untriedMoves[mi]
			deleteat!(node.untriedMoves, findall(x -> x == m, node.untriedMoves))			
			makeMove(state, m)
			node = addChild(node, m, state)
		end
		playout(state)
		# Backpropagate
		scr = getScore1(state)
		while node != nothing
		#while node != node.parentNode
			psign = node.player * 2 - 3
			nodeUpdate(node, scr * psign)
			if node == node.parentNode
				node = nothing
			else
				node = node.parentNode
			end
		end
	end
	maxchild::UInt16 = 0
	maxvisit::Float64 = 0.0
	for cindex::UInt16 = 1:length(rootnode.childNodes)
		c = rootnode.childNodes[cindex]
		if c.nvisits > maxvisit
			maxchild = cindex
			maxvisit = c.nvisits
		end
	end
	# return the move that was most visited	
	return rootnode.childNodes[maxchild].move
end

getscr(m::Move) = m.scr
	
function minValue(alpha::Float64, beta::Float64, depth::UInt16)
	ai1.count += 1
	depth2::UInt16 = depth + 1	
	if depth > ai1.maxDepth
		ai1.maxDepth = depth
	end
	minV::Float64 = pinf
	v::Float64 = pinf
	movelist = generateMoves(aiBoard)
	sort!(movelist, by=getscr, rev=true)
	nmove::UInt16 = length(movelist)
	#print "minmv", nmove
	if nmove == 0
		#pass
		if depth == 1
			ai1.bmove = 0
			ai1.bscr = -1
		end
	else
		for i = 1:min(maxbreadth, length(movelist))
			mv::UInt16 = movelist[i].l1
			makeMove(aiBoard, mv) #make the move on the current board
			ai1.threat[depth] = aiBoard.trimade
			if depth < maxdepth1 && (depth < maxdepth2 || ai1.threat[depth] || 
				(depth == maxdepth2 && ai1.threat[depth-1]))
					#v = zobrist_dict[aiBoard.zhash]
				v = maxValue(alpha, beta, depth2)
				if v <= ninf + eps
					#no more moves from here
					v = getScore1(aiBoard)
				end
				#zobrist_dict[aiBoard.zhash] = v
			else
				v = getScore1(aiBoard)
			end
			remove(aiBoard, mv) #undo move
			#aiBoard.display()
			if v < minV
				minV = v
				if depth==1
					ai1.bmove = mv
					ai1.bscr = v
				end
			end
				#print(bmove, i, v)
			if minV <= alpha
				#print "mincu", minV, alpha, beta, depth
				return minV #cutoff
			end
			if minV < beta
				beta = minV
			end
		end
	end
	#print("minou", minV, alpha, beta, depth)
	return minV
end

function maxValue(alpha::Float64, beta::Float64, depth::UInt16)
	ai1.count += 1
	depth2::UInt16 = depth + 1
	if depth > ai1.maxDepth
		ai1.maxDepth = depth
	end
	maxV::Float64 = ninf
	v::Float64 = ninf
	movelist = generateMoves(aiBoard)
	sort!(movelist, by=getscr, rev=true)
	nmove::UInt16 = length(movelist)
	if nmove == 0
		#pass
		if depth == 1
			ai1.bmove = 0
			ai1.bscr = -1
		end
	else
		for i = 1:min(maxbreadth, length(movelist))
			mv::UInt16 = movelist[i].l1
			makeMove(aiBoard, mv) #make the move on the current board
			ai1.threat[depth] = aiBoard.trimade
			if depth < maxdepth1 && (depth < maxdepth2 || ai1.threat[depth] || 
				(depth == maxdepth2 && ai1.threat[depth-1]))
					#v = zobrist_dict[aiBoard.zhash]
				v = minValue(alpha, beta, depth2)
				if v >= pinf - eps
					#no more moves from here
					v = getScore1(aiBoard)
				end
				#zobrist_dict[aiBoard.zhash] = v
			else
				v = getScore1(aiBoard)
			end
			remove(aiBoard, mv) #undo move
			if v > maxV
				maxV = v
				if depth==1
					ai1.bmove = mv
					ai1.bscr = v
				end
			end
			if maxV >= beta
				return maxV #cutoff
			end
			if maxV > alpha
				alpha = maxV
			end
		end
	end
	return maxV
end
		
function abSearch(aiBoard)
	#alpha beta dfs search
	ai1.bscr = 0
	ai1.bmove = 0
	ai1.count = 0
	depth::UInt16 = 1
	#zobrist_dict.clear()
	if aiBoard.currentPlayer == 1
		ai1.bscr = maxValue(ninf,pinf,depth)
	else
		ai1.bscr = minValue(ninf,pinf,depth)
	end
	println("count: ", ai1.count)
	return ai1.bmove
end

function aigo(board0)
	ai1.bmove = 0
	ai1.bscr = 0
	t1 = time_ns()
	ai1.maxDepth = 0
	copyBoard(aiBoard, board0)
	if aiBoard.turn < 0
		randMove()
	else
		#ai1.bmove = uct(aiBoard, 8000)
		ai1.bmove = abSearch(aiBoard)
	end
	t2 = time_ns()
	elapsed = (t2-t1) / 1000000000.0
	println(ai1.bscr, ",", ai1.maxDepth, ",", elapsed)	
	return ai1.bmove
end

board1 = Board(0,0,0,[0.0,0.0],Vector{UInt16}[],Vector{UInt16}[],Vector{UInt16}[],false,0,Vector{UInt16}[],0)
aiBoard = Board(0,0,0,[0.0,0.0],Vector{UInt16}[],Vector{UInt16}[],Vector{UInt16}[],false,0,Vector{UInt16}[],0)
gpoint = GraphPoints(Vector{Point}(undef, numpoints))	
#randStart(gpoint)
testStart(gpoint)
	
gline = GraphLines(0,0,Vector{UInt16}[],Vector{UInt16}[],Array{UInt16}(undef,numpoints,numpoints) )
init1(gline)
println(gline.linect, " lines, ", gline.trict, " triangles")

gcross = GraphCross([Vector{UInt16}(undef,0) for _ in 1:gline.linect], [Vector{UInt16}(undef,0) for _ in 1:gline.linect])	
init2(gcross)

ai1 = AIstate(0, 0.0, 0, [false for _ in 1:gline.linect], 0, rand(UInt32, (gline.linect,2)), rand(UInt32, (gline.trict,2)))

#globals
players = [1,0]
pt1 = 0
clearBoard(board1)

function drawCircle(widget, pt)
    ctx = getgc(widget)
    set_source_rgb(ctx, 0.6, 0.6, 0.6)
    arc(ctx, pt.x, pt.y, pradius, 0, 2pi)
    #stroke(ctx)
    fill(ctx)
    reveal(widget)
end

function drawLine(widget, line, player)
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
	set_line_width(ctx, lwidth)  
    stroke(ctx)
end

function drawTriangle(widget, tr, player)
    ctx = getgc(widget)
	pt1 = gpoint.points[gline.triangles[tr][1]]
	pt2 = gpoint.points[gline.triangles[tr][2]]
	pt3 = gpoint.points[gline.triangles[tr][3]] 
	move_to(ctx, pt1.x, pt1.y)
	line_to(ctx, pt2.x, pt2.y)
	line_to(ctx, pt3.x, pt3.y)
	line_to(ctx, pt1.x, pt1.y)
	set_line_width(ctx, lwidth)
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

function clear_surface()
end

function aiprocess()
	l1 = aigo(board1)
	return l1
end

function displayMove(widget, l1)
	player = 3 - board1.currentPlayer #already incremented player
	drawLine(widget, l1, player)		
	for tr in gcross.line2triangles[l1]
		if board1.trifilled[tr] == 3
			p1 = gline.triangles[tr][1]
			p2 = gline.triangles[tr][2]
			p3 = gline.triangles[tr][3]
			l1 = gline.linenum[p1,p2]		
			l2 = gline.linenum[p1,p3] 
			l3 = gline.linenum[p2,p3]
			drawTriangle(widget, tr, player)
			drawLine(widget, l1, board1.linecolor[l1]) 
			drawLine(widget, l2, board1.linecolor[l2]) 
			drawLine(widget, l3, board1.linecolor[l3]) 
			#label1.configure(text=str(Board.score[1]))
			#label2.configure(text=str(Board.score[2]))
		end
	end
	reveal(widget)	
	smoves = generateMoves(board1)
	s = size(smoves,1)
	getscr(m::Move) = m.scr
	sort(smoves, by=getscr,  rev=true)	
	println("turn", board1.turn, " len", s)
	if s == 0
		winner = findWinner(board1)
		#label3.configure(text="Game Over", fg=lcolors[winner])		 
		#label3.update()		   
	elseif players[board1.currentPlayer] == 1
		#canvas.update()
		t1 = Task(aiprocess); schedule(t1)
		l1 = fetch(t1)
		#l1 = aiprocess()
		if l1 != 0
			makeMove(board1, l1) 
			displayMove(widget, l1)
		end
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
        #println(h," by ",w)
        
		for pt in gpoint.points
			drawCircle(widget, pt)  
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
				drawTriangle(widget, tr, p)
			end
		end
		for l1 = 1:gline.linect
			p = board1.linecolor[l1] 
			if p > 0
				drawLine(widget, l1, p)
			end
		end
    end
    
	id = signal_connect(button1, "clicked") do widget
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
		if rewind(board1)
			clear_surface()
			players[1] = 0
			players[2] = 0
		end
	end
	id = signal_connect(button8, "clicked") do widget
		if back(board1)
			clear_surface()	
			players[1] = 0
			players[2] = 0
		end
	end
	id = signal_connect(button9, "clicked") do widget
		if ahead(board1)	
			clear_surface()	
			players[1] = 0
			players[2] = 0
		end
	end
	id = signal_connect(button10, "clicked") do widget
		if ffwd(board1)
			clear_surface()
			players[1] = 0
			players[2] = 0
		end
	end									
        
    canv.mouse.button1press = @guarded (widget, event) -> begin
		ptc = Point(round(UInt16, event.x), round(UInt16, event.y))
		global pt1 = 0
		for i in 1:numpoints
			if ptdist(ptc, gpoint.points[i]) < clickrad
				pt1 = i
			end
		end		
	end
    
    canv.mouse.button1release = @guarded (widget, event) -> begin
		ptc = Point(round(UInt16, event.x), round(UInt16, event.y))
		pt2 = 0
		for i in 1:numpoints
			if ptdist(ptc, gpoint.points[i]) < clickrad
				pt2 = i
			end
		end
		if pt1 > 0 && pt2 > 0 && pt1 != pt2
			l1 = gline.linenum[pt1, pt2]
			if legalLine(board1, l1)
				makeMove(board1, l1)
				displayMove(widget, l1)
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

	println(pointInTriangle(p3, p2, p4, p6)) #t
	println(pointInTriangle(p1, p2, p4, p6)) #f
	println(pointInTriangle(p5, p2, p4, p6)) #t

	println(sign(p2, p1, p3)) #+
	println(sign(p4, p1, p3)) #-
	#returns number indicating side of p1 on segment p2-p3
	
	gpoint.points[1] = p5
	println(lineLegal(p4, p6))#f
	
end

hellogtkapp()
#testfunctions()


