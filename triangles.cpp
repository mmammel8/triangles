#include <gtk/gtk.h>
#include <cstddef>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "Tboard.h"
#include "AI.h"
//g++ `pkg-config --cflags gtk+-3.0` -o h AI.cpp Tboard.cpp triangles.cpp `pkg-config --libs gtk+-3.0`

//globals
Tboard *bd1;
AI *ai1;
double px, py;
int players[3], winner;

static void print_hello(GtkWidget *widget, gpointer data)
{
	g_print("Hello World\n");

}

static cairo_surface_t *surface = NULL;

static void clear_surface(void)
{
	cairo_t *cr;

	cr = cairo_create(surface);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	cairo_destroy(cr);
}

static gboolean configure_event_cb(GtkWidget *widget,
	GdkEventConfigure *event, gpointer data)
{
	if (surface)
		cairo_surface_destroy(surface);

	surface = gdk_window_create_similar_surface(gtk_widget_get_window(widget),
		CAIRO_CONTENT_COLOR,
		gtk_widget_get_allocated_width(widget),
		gtk_widget_get_allocated_height(widget));

	/* Initialize the surface to white */
	clear_surface();

	return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static void draw_circle(GtkWidget *widget, gdouble x, gdouble y)
{
	cairo_t *cr;

	cr = cairo_create(surface);
	//cr->save();
	//cairo_set_source_rgb(cr, 0.3, 0.4, 0.6);
	cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
	cairo_arc(cr, x, y, (double)bd1->PRADIUS, 0.0, 2.0 * M_PI); // full circle
	//cr->set_source_rgba(0.0, 0.0, 0.8, 0.6);    // partially translucent
	cairo_fill_preserve(cr);
	//cr->restore();  // back to opaque black
	cairo_stroke(cr);
}

static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	double cx, cy;
	cairo_set_source_surface(cr, surface, 0, 0);
	for (int i = 0; i < bd1->NUMPOINTS; ++i)
	{	
		cx = (double)bd1->points[i].x;
		cy = (double)bd1->points[i].y;		
	    draw_circle(widget, cx, cy);
	}
	
	cairo_paint(cr);

	return FALSE;
}

static void draw_line(GtkWidget *widget, Point pt1, Point pt2, int player)
{
	cairo_t *cr;
	/* Paint to the surface, where we store our state */
	cr = cairo_create(surface);
	
	double x1 = pt1.x, x2 = pt2.x, y1 = pt1.y, y2 = pt2.y;
	cairo_move_to(cr, x1, y1);
	cairo_line_to(cr, x2, y2);
	cairo_set_line_width(cr, bd1->LWIDTH);
	if (player == 1)
		cairo_set_source_rgb(cr, 0.2, 0.2, 0.8);
	if (player == 2)
		cairo_set_source_rgb(cr, 0.8, 0.2, 0.2);		
	cairo_stroke(cr);
	
	double minx, miny, maxx, maxy, xwid, ywid;
	minx = std::min(x1,x2);
	miny = std::min(y1,y2);
	maxx = std::max(x1,x2);
	maxy = std::max(y1,y2);
	xwid = std::max(maxx-minx, (double)bd1->LWIDTH);
	ywid = std::max(maxy-miny, (double)bd1->LWIDTH);

	cairo_destroy(cr);

	/* invalidate the affected region of the drawing area. */
	gtk_widget_queue_draw_area(widget, minx, miny, xwid, ywid);
}

static void draw_triangle(GtkWidget *widget, Point pt1, Point pt2, Point pt3, int player)
{
	cairo_t *cr;
	/* Paint to the surface, where we store our state */
	cr = cairo_create(surface);
	
	double x1 = pt1.x, x2 = pt2.x, x3 = pt3.x, y1 = pt1.y, y2 = pt2.y, y3 = pt3.y;
	cairo_move_to(cr, x1, y1);
	cairo_line_to(cr, x2, y2);
	cairo_line_to(cr, x3, y3);
	cairo_line_to(cr, x1, y1);
	cairo_set_line_width(cr, bd1->LWIDTH);
	if (player == 1)
		cairo_set_source_rgb(cr, 0.1, 0.1, 0.6);
	if (player == 2)
		cairo_set_source_rgb(cr, 0.6, 0.1, 0.1);
	cairo_fill_preserve(cr);				
	cairo_stroke(cr);
	double minx, miny, maxx, maxy, xwid, ywid;
	minx = std::min(x1,x2);
	minx = std::min(minx,x3);
	miny = std::min(y1,y2);
	miny = std::min(miny,y3);	
	maxx = std::max(x1,x2);
	maxx = std::max(maxx,x3);	
	maxy = std::max(y1,y2);
	maxy = std::max(maxy,y3);	
	xwid = std::max(maxx-minx, (double)bd1->LWIDTH);
	ywid = std::max(maxy-miny, (double)bd1->LWIDTH);
	cairo_destroy(cr);

	/* invalidate the affected region of the drawing area. */
	gtk_widget_queue_draw_area(widget, minx, miny, xwid, ywid);
}

void display_move(GtkWidget *widget, int line1)
{
	int movelist[bd1->MAXLINES], nmoves;
	int player = 3 - bd1->currentPlayer; //already incremented player
	Point pt1 = bd1->points[bd1->lines[line1][0]];
	Point pt2 = bd1->points[bd1->lines[line1][1]];
	draw_line(widget, pt1, pt2, player);		
	for (int i = 0; i < bd1->line_tri_ct[line1]; ++i)
	{ 
		int tr = bd1->line2triangles[line1][i];
		if (bd1->trifilled[tr] == 3)
		{
			int p1 = bd1->triangles[tr][0];
			int p2 = bd1->triangles[tr][1];
			int p3 = bd1->triangles[tr][2];
			int l1 = bd1->linenum[p1][p2];	
			int l2 = bd1->linenum[p1][p3];
			int l3 = bd1->linenum[p2][p3];
			Point pt1 = bd1->points[p1];
			Point pt2 = bd1->points[p2];
			Point pt3 = bd1->points[p3];
			draw_triangle(widget, pt1, pt2, pt3, player);
			draw_line(widget, pt1, pt2, bd1->linecolor[l1]);
			draw_line(widget, pt1, pt3, bd1->linecolor[l2]);
			draw_line(widget, pt2, pt3, bd1->linecolor[l3]);
			//label1.configure(text=str(self.Board.score[1]));
			//label2.configure(text=str(self.Board.score[2]));
		}
	}

	nmoves = bd1->generate_moves(movelist);
	std::cout << "turn " << bd1->turn << " len " << nmoves << std::endl;
	if (nmoves == 0)
	{
		winner = bd1->get_winner();
		//self.label3.configure(text="Game Over", fg=self.lcolors[winner])		 
		//self.label3.update()
	}
	else if (players[bd1->currentPlayer] == 1)
	{
		//canvas.update()
		int line2 = ai1->go(bd1);
		//self.l1 = int(input("l1"))
		std::cout << "ai " << line2 << std::endl;
		if (line2 > -1)
		{
			bd1->make_move(line2);
			display_move(widget, line2);
		}
	}
}
			
/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */
static gboolean button_press_event_cb(GtkWidget *widget,
	GdkEventButton *event, gpointer data)
{
	if (surface == NULL)
		return FALSE;

	if (event->button == GDK_BUTTON_PRIMARY)
		{
			px = event->x;
			py = event->y;
		}
	else if (event->button == GDK_BUTTON_SECONDARY)
		{
			//clear_surface();
			//gtk_widget_queue_draw(widget);
		}

	return TRUE;
}

static gboolean button_release_event_cb(GtkWidget *widget,
	GdkEventButton *event, gpointer data)
{
	if (surface == NULL)
		return FALSE;

    double qx = event->x, qy = event->y;
    Point p1, p2;
    p1.x = (int)px;
    p1.y = (int)py;
    p2.x = (int)qx;
    p2.y = (int)qy;
    //std::cout << p1.x << " " << p1.y << " " << p2.x << " " << p2.y << std::endl;
    int pindex1 = -1, pindex2 = -1;
	for (int i = 0; i < bd1->NUMPOINTS; ++i)
	{
		if (bd1->pt_pt_dist(p1, bd1->points[i]) < bd1->CLICKRAD)
			pindex1 = i;		
		if (bd1->pt_pt_dist(p2, bd1->points[i]) < bd1->CLICKRAD)
			pindex2 = i;
	}
	if (pindex1 > -1 && pindex2 > -1 && pindex1 != pindex2)
	{
		int l1 = bd1->linenum[pindex1][pindex2];
		if (bd1->legalline(l1))
		{
			bd1->make_move(l1);
			display_move(widget, l1);
			//std::cout << "mark move " << l1 << std::endl; 
		}
	}

	return TRUE;
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
static gboolean motion_notify_event_cb(GtkWidget *widget,
	GdkEventMotion *event, gpointer data)
{
	if (surface == NULL)
		return FALSE;

	//if (event->state & GDK_BUTTON1_MASK)
	//	draw_line(widget, event->x, event->y);

	return TRUE;
}

static void close_window(void)
{
	if (surface)
		cairo_surface_destroy(surface);
}

static void activate(GtkApplication *app, gpointer user_data)
{
	GtkWidget *window;
	GtkWidget *grid;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *drawing_area; 

	/* create a new window,*/
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Window");
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);

	/* container that pack our buttons */
	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), grid);

	button = gtk_button_new_with_label("Button 1");
	g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);
	/* Place the first button in the grid cell (0, 0), and make it fill
	 * just 1 cell horizontally and vertically (ie no spanning)
*/
	gtk_grid_attach(GTK_GRID(grid), button, 0, 0, 1, 1);

	button = gtk_button_new_with_label("Button 2");
	g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);
	gtk_grid_attach(GTK_GRID(grid), button, 1, 0, 1, 1);

	button = gtk_button_new_with_label("Quit");
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_widget_destroy), window);
	// Place the Quit button in the grid cell (0, 1), and make it span 2 columns.
	gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 2, 1);
	
	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawing_area, bd1->CWIDTH, bd1->CHEIGHT);
	gtk_grid_attach(GTK_GRID(grid), drawing_area, 0, 2, 2, 2);
	
	/* Signals used to handle the backing surface */
	g_signal_connect(drawing_area, "draw",
		G_CALLBACK(draw_cb), NULL);
	g_signal_connect(drawing_area,"configure-event",
		G_CALLBACK(configure_event_cb), NULL);

	/* Event signals */
	g_signal_connect(drawing_area, "motion-notify-event",
		G_CALLBACK(motion_notify_event_cb), NULL);
	g_signal_connect(drawing_area, "button-press-event",
		G_CALLBACK(button_press_event_cb), NULL);
	g_signal_connect(drawing_area, "button-release-event",
		G_CALLBACK(button_release_event_cb), NULL);									
			 
	gtk_widget_set_events(drawing_area, gtk_widget_get_events(drawing_area)
		| GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
																													 
	gtk_widget_show_all(window);
}

int main(int argc, char **argv)
{
	GtkApplication *app;
	int status;
	
	bd1 = new Tboard();
	bd1->rand_start();
	//bd1->test_start();
	bd1->init();
	bd1->clear();	
	ai1 = new AI();	
	players[1] = 0;
	players[2] = 1;

	app = gtk_application_new("org.gtk.triangles", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	
	delete bd1;

	return status;
}

