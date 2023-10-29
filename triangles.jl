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
	x
	y
end

#globals
points = Vector{Point}(undef, NUMPOINTS) #list of points on the board
#lines #list of lines in form of tuples of point numbers
#triangles #list of triangles in form of triples of point numbers
#linenum # [p1][p2]  number of line between p1 and p2
#trinum #[p1][p2][p3]  number of triangle with vert p1 p2 p3
#line2triangles #lists triangles including this line
#line_cross #[l1][l2] if these two intersect
#crosslines # lines that cross this one
	
mutable struct Board
	numlines
	numtri
	currentPlayer
	turn
	winner
	score
	linecolor
	trifilled
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
	if pa.x == pb.x
	#segment is vertical
		if pa.x == pc.x
			return 0
		elseif pa.x < pc.x
			return 1
		else
			return -1
		end
	elseif pa.x == pc.x
	#point is vertical
		if pc.y > pa.y
			return 1
		else
			return -1
		end
	else
		m1 = (pb.y - pa.y)*(pc.x - pa.x)
		m2 = (pc.y - pa.y)*(pb.x - pa.x)
		if m2 > m1
			return 1
		elseif m1 > m2
			return -1
		else
			return 0
		end
	end
end

function intersect(pa, pb, pc, pd)
	#determines if segment pa-pb intersects pc-pd
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
		end
		result = (s1 != s2 && s3 != s4)
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
		if points[i] != pt1 && points[i] != pt2 && points[i].x >= lox && points[i].x <= hix && points[i].y >= loy && points[i].y <= hiy
			num = dx*(pt1.y - points[i].y) - dy*(pt1.x - points[i].x)
			if num * num < threshold
				#print(num*num, threshold, pt1, pt2, points[i])
				result = false
			end
		end
	end
	return result
end

function trilegal(pt1, pt2, pt3)
	#returns if triangle does not contain another point
	result = true
	for i = 1:NUMPOINTS
		pt0 = points[i]
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
				if ptdist(pt, points[j]) < BUFFERDIST
					ok = false
				end
			end
			if ok
				points[i] = pt
			end
		end
	end
end

function init1()
	#precompute lists of lines, triangles
	global linect = 0
	for i = 1:NUMPOINTS
		for j = i + 1:NUMPOINTS
			if linelegal(points[i], points[j])
				linenum[i,j] = linenum[j,i] = linect
				push!(lines, [i,j])
				linect += 1
			end
		end
	end
	global trict = 0
	for i = 1:NUMPOINTS
		for j = i + 1:NUMPOINTS
			for k = j + 1:NUMPOINTS
				l1 = linenum[i,j]
				l2 = linenum[i,k]
				l3 = linenum[j,k]
				if l1 > -1 && l2 > -1 && l3 > -1
					if trilegal(points[i], points[j], points[k])
						 trinum[i,j,k] = trinum[j,i,k] = trict
						 trinum[i,k,j] = trinum[j,k,i] = trict
						 trinum[k,i,j] = trinum[k,j,i] = trict
						 push!(triangles, [i,j,k])
						 #line2triangles[l1].append(trict)
						 #line2triangles[l2].append(trict)
						 #line2triangles[l3].append(trict)
						 trict += 1
					end
				end
			end
		end
	end
	println(linect, " ", trict)	
end

function init2()
	#determine line crosses
	for l1 = 1:linect
		pa = points[lines[l1][1]]
		pb = points[lines[l1][2]]
		for l2 = l1 + 1:linect
			pc = points[lines[l2][1]]
			pd = points[lines[l2][2]]
			if pa == pc || pa == pd || pb == pc || pb == pd
				line_cross[l1,l2] = line_cross[l2,l1] = false
			else
				line_cross[l1,l2] = line_cross[l2,l1] = intersect(pa, pb, pc, pd)
			end
		end
	end
	for l1 = 1:linect
		for l2 = l1 + 1:linect
			if line_cross[l1,l2]
				push!(crosslines[l1], l2)
			end
		end
	end
	for t1 = 1:trict
		l1 = triangles[t1][1]
		l2 = triangles[t1][2]
		l3 = triangles[t1][3]
		push!(line2triangles[l1], t1)
		push!(line2triangles[l2], t1)
		push!(line2triangles[l3], t1)
	end	
end

board1 = Board(0,0,0,0,0,[0,0,0],Vector{Int},Vector{Int},false,Vector{Int},0)
rand_start()

#globals
linect = 0
lines = Vector{Int}[]
linenum = zeros(Int, NUMPOINTS, NUMPOINTS)
trict = 0
triangles = Vector{Int}[]
trinum = zeros(Int, NUMPOINTS, NUMPOINTS, NUMPOINTS)
init1()
println(linect, ",", trict)

line_cross = trues(linect, linect)
crosslines = [Vector{Int}(undef,0) for _ in 1:linect]
line2triangles = [Vector{Int}(undef,0) for _ in 1:linect]
init2()
#zobrist1 = [[0 for _ in range(2)] for _ in range(linect)]
#zobrist2 = [[0 for _ in range(2)] for _ in range(trict)]

