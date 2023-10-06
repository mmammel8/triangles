#include <gtk/gtk.h>
#include <cstddef>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include "Tboard.h"
#include "AI.h"
//g++ `pkg-config --cflags gtk+-3.0` -o h AI.cpp Tboard.cpp triangles.cpp `pkg-config --libs gtk+-3.0`

//globals
Tboard *bd1;
AI *ai1;
double px, py;
int players[3], winner;
GtkWidget *window;
GtkWidget *grid;
GtkWidget *button1, *button2, *button3, *button4, *button5, *button6, *button7, *button8, *button9, *button10, *button11, *button12;
GtkWidget *frame;
GtkWidget *drawing_area;
	
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
static void draw_circle(GtkWidget *widget, gdouble x, gdouble y, int clr)
{
	cairo_t *cr;

	cr = cairo_create(surface);
	//cr->save();
	if (clr == 1)
		cairo_set_source_rgb(cr, 0.2, 0.2, 0.8);
	else if (clr == 2)
		cairo_set_source_rgb(cr, 0.8, 0.2, 0.2);
	else
		cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
	cairo_arc(cr, x, y, (double)bd1->PRADIUS, 0.0, 2.0 * M_PI); // full circle
	//cr->set_source_rgba(0.0, 0.0, 0.8, 0.6);    // partially translucent
	cairo_fill_preserve(cr);
	//cr->restore();  // back to opaque black
	cairo_stroke(cr);
	cairo_destroy(cr);	
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

	//gtk_widget_queue_draw_area(widget, minx, miny, xwid, ywid);
}

static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	double cx, cy;
	cairo_set_source_surface(cr, surface, 0, 0);
	for (int i = 0; i < bd1->NUMPOINTS; ++i)
	{	
		cx = (double)bd1->points[i].x;
		cy = (double)bd1->points[i].y;		
	    draw_circle(widget, cx, cy, 0);
	}
	for (int line1 = 0; line1 < bd1->linect; ++line1)
	{	
		if (bd1->linecolor[line1] > 0)
		{
			Point pt1 = bd1->points[bd1->lines[line1][0]];
			Point pt2 = bd1->points[bd1->lines[line1][1]];
			draw_line(widget, pt1, pt2, bd1->linecolor[line1]);
		}
	}
	for (int tr = 0; tr < bd1->trict; ++tr)
	{	
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
			draw_triangle(widget, pt1, pt2, pt3, bd1->tricolor[tr]);
			draw_line(widget, pt1, pt2, bd1->linecolor[l1]);
			draw_line(widget, pt1, pt3, bd1->linecolor[l2]);
			draw_line(widget, pt2, pt3, bd1->linecolor[l3]);
		}
	}		
	cairo_paint(cr);
	gtk_widget_queue_draw_area(widget, 0, 0, bd1->CWIDTH, bd1->CWIDTH);	
	char labscr[4];
	sprintf(labscr,"%d",bd1->score[1]);
	gtk_label_set_text(GTK_LABEL (button11), labscr);
	sprintf(labscr,"%d",bd1->score[2]);
	gtk_label_set_text(GTK_LABEL (button12), labscr);

	return FALSE;
}

void *CUCT( void *threadid )
{
	ai1->go(bd1);
	pthread_exit(NULL);	
}

void display_move(GtkWidget *widget, int line1)
{
	const int NUM_THREADS = 1;
	long t;
	int rc;
	void *status;
	pthread_t thread[NUM_THREADS];
	char labscr[16];
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
		}
	}
	nmoves = bd1->generate_moves(movelist);		
	sprintf(labscr,"%d",bd1->score[1]);
	gtk_label_set_text(GTK_LABEL (button11), labscr);
	sprintf(labscr,"%d",bd1->score[2]);
	gtk_label_set_text(GTK_LABEL (button12), labscr);
	if (bd1->currentPlayer == 1)
	{
		draw_circle(widget, 100, 10, 1);
		draw_circle(widget, 700, 10, 0);		
	}
	else
	{
		draw_circle(widget, 100, 10, 0);
		draw_circle(widget, 700, 10, 2);
	}
	gtk_widget_queue_draw(widget);
	while (gtk_events_pending())
  		gtk_main_iteration();	
	std::cout << "turn: " << bd1->turn << ", moves left: " << nmoves << std::endl;

	/* invalidate the affected region of the drawing area. */
	//gtk_widget_queue_draw_area(widget, 0, 0, bd1->CWIDTH, bd1->CWIDTH);	
	if (nmoves == 0)
	{
		winner = bd1->get_winner();
		std::cout << "winner " << winner << std::endl;
		if (winner == 1)
		{
			sprintf(labscr,"%d wins",bd1->score[1]);
			gtk_label_set_text(GTK_LABEL (button11), labscr);
		}
		else if (winner == 2)
		{
			sprintf(labscr,"%d wins",bd1->score[2]);
			gtk_label_set_text(GTK_LABEL (button12), labscr);		
		}
		else
		{
			sprintf(labscr,"%d ties",bd1->score[1]);
			gtk_label_set_text(GTK_LABEL (button11), labscr);		
		}
	}
	else if (players[bd1->currentPlayer] == 1)
	{
		//canvas.update()
		//gtk_widget_set_sensitive(button9, false);	
		//gtk_widget_set_sensitive(button10, true);
		//gtk_widget_queue_draw(widget);
		usleep(2000);	
		//int line2 = ai1->go(bd1);
		
		//std::cout << "ai " << line2 << std::endl;
		rc = pthread_create(&thread[0], NULL, CUCT, (void *)t);
		if (rc) 
		{
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}		
		rc = pthread_join(thread[0], &status);
		if (rc) 
		{
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}		
		int line2 = ai1->bmove;
		
		if (line2 > -1)
		{
			bd1->make_move(line2);
			display_move(widget, line2);
		}
		//gtk_widget_set_sensitive(button9, true);	
		//gtk_widget_set_sensitive(button10, false);	
	}
	gtk_widget_set_sensitive(button5, true);
	gtk_widget_set_sensitive(button6, true);
	gtk_widget_set_sensitive(button7, false);
	gtk_widget_set_sensitive(button8, false);

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

static void xrewind(GtkWidget *widget, gpointer data)
{
	if (bd1->rewind())
	{
		clear_surface();	
		gtk_widget_queue_draw_area(widget, 0, 0, bd1->CWIDTH, bd1->CHEIGHT);
		players[1] = 0;
		players[2] = 0;	
		gtk_widget_set_sensitive(button5, false);
		gtk_widget_set_sensitive(button6, false);
		gtk_widget_set_sensitive(button7, true);
		gtk_widget_set_sensitive(button8, true);
	}

}

static void xffwd(GtkWidget *widget, gpointer data)
{
	if (bd1->ffwd())
	{
		clear_surface();	
		gtk_widget_queue_draw_area(widget, 0, 0, bd1->CWIDTH, bd1->CHEIGHT);
		players[1] = 0;
		players[2] = 0;	
		gtk_widget_set_sensitive(button5, true);
		gtk_widget_set_sensitive(button6, true);
		gtk_widget_set_sensitive(button7, false);
		gtk_widget_set_sensitive(button8, false);		
	}

}

static void xback(GtkWidget *widget, gpointer data)
{
	if (bd1->back())
	{
		clear_surface();	
		gtk_widget_queue_draw_area(widget, 0, 0, bd1->CWIDTH, bd1->CHEIGHT);
		players[1] = 0;
		players[2] = 0;	
		if (bd1->turn == 0)
		{
			gtk_widget_set_sensitive(button5, false);
			gtk_widget_set_sensitive(button6, false);		
		}
		gtk_widget_set_sensitive(button7, true);
		gtk_widget_set_sensitive(button8, true);		
	}

}

static void xahead(GtkWidget *widget, gpointer data)
{
	if (bd1->ahead())
	{
		clear_surface();	
		gtk_widget_queue_draw_area(widget, 0, 0, bd1->CWIDTH, bd1->CHEIGHT);
		players[1] = 0;
		players[2] = 0;
		gtk_widget_set_sensitive(button5, true);
		gtk_widget_set_sensitive(button6, true);
		if (bd1->turn > bd1->lastturn)
		{
			gtk_widget_set_sensitive(button7, false);
			gtk_widget_set_sensitive(button8, false);		
		}
	}

}

static void new_game(GtkWidget *widget, gpointer data)
{
	bd1->rand_start();
	bd1->init();
	bd1->clear();
	players[1] = 0;
	players[2] = 0;	
	gtk_widget_set_sensitive(button5, false);
	gtk_widget_set_sensitive(button6, false);
	gtk_widget_set_sensitive(button7, false);
	gtk_widget_set_sensitive(button8, false);		
	clear_surface();
	gtk_widget_queue_draw_area(widget, 0, 0, bd1->CWIDTH, bd1->CHEIGHT);
}

static void xload(GtkWidget *widget, gpointer data)
{
	if (bd1->Read())
	{
		players[1] = 0;
		players[2] = 0;		
		gtk_widget_set_sensitive(button5, true);
		gtk_widget_set_sensitive(button6, true);
		gtk_widget_set_sensitive(button7, false);
		gtk_widget_set_sensitive(button8, false);	
		clear_surface();
		gtk_widget_queue_draw_area(widget, 0, 0, bd1->CWIDTH, bd1->CHEIGHT);			
	}
}

static void xsave(GtkWidget *widget, gpointer data)
{
	bd1->savetodisk();
}

static void ai_go(GtkWidget *widget, gpointer data)
{
	players[bd1->currentPlayer] = 1; //auto move next time
	int line1 = ai1->go(bd1);
	if (line1 > -1)
	{
		bd1->make_move(line1);
		display_move(widget, line1);
	}
}

static void ai_stop(GtkWidget *widget, gpointer data)
{
	ai1->halt = true;
	players[1] = 0;
	players[2] = 0;
}


static void activate(GtkApplication *app, gpointer user_data)
{
	/* create a new window,*/
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Triangles");
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);

	/* container that pack our buttons */
	grid = gtk_grid_new();
	gtk_container_add(GTK_CONTAINER(window), grid);

	button1 = gtk_button_new_with_label("Quit");
	//button1 = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
	g_signal_connect_swapped(button1, "clicked", G_CALLBACK(gtk_widget_destroy), window);
	// Place the Quit button in the grid cell(0, 1), and make it span 2 columns.
	gtk_grid_attach(GTK_GRID(grid), button1, 0, 0, 1, 1);
	
	button2 = gtk_button_new_with_label("New Game");
	g_signal_connect_swapped(button2, "clicked", G_CALLBACK(new_game), window);
	// Place the Quit button in the grid cell(0, 1), and make it span 2 columns.
	gtk_grid_attach(GTK_GRID(grid), button2, 1, 0, 1, 1);

	button3 = gtk_button_new_with_label("Load");
	//button = gtk_image_new_from_icon_name("media-seek-forward", GTK_ICON_SIZE_BUTTON);
	g_signal_connect(button3, "clicked", G_CALLBACK(xload), NULL);
	gtk_grid_attach(GTK_GRID(grid), button3, 2, 0, 1, 1);

	button4 = gtk_button_new_with_label("Save");
	//button = gtk_image_new_from_icon_name("media-seek-forward", GTK_ICON_SIZE_BUTTON);
	g_signal_connect(button4, "clicked", G_CALLBACK(xsave), NULL);
	gtk_grid_attach(GTK_GRID(grid), button4, 3, 0, 1, 1);


	button5 = gtk_button_new_with_label("Rewind");
	//button = gtk_image_new_from_icon_name("media-seek-backward", GTK_ICON_SIZE_BUTTON);
	g_signal_connect(button5, "clicked", G_CALLBACK(xrewind), NULL);
	// Place the first button in the grid cell(0, 0)
	// fill just 1 cell horizontally and vertically(ie no spanning)
	gtk_grid_attach(GTK_GRID(grid), button5, 0, 1, 1, 1);
	gtk_widget_set_sensitive(button5, false);

	button6 = gtk_button_new_with_label("Back");
	g_signal_connect(button6, "clicked", G_CALLBACK(xback), NULL);
	gtk_grid_attach(GTK_GRID(grid), button6, 1, 1, 1, 1);
	gtk_widget_set_sensitive(button6, false);
	
	button7 = gtk_button_new_with_label("Ahead");
	g_signal_connect(button7, "clicked", G_CALLBACK(xahead), NULL);
	gtk_grid_attach(GTK_GRID(grid), button7, 2, 1, 1, 1);
	gtk_widget_set_sensitive(button7, false);
	
	button8 = gtk_button_new_with_label("FForward");
	//button = gtk_image_new_from_icon_name("media-seek-forward", GTK_ICON_SIZE_BUTTON);
	//media-skip-backward media-playback-start
	g_signal_connect(button8, "clicked", G_CALLBACK(xffwd), NULL);
	gtk_grid_attach(GTK_GRID(grid), button8, 3, 1, 1, 1);
	gtk_widget_set_sensitive(button8, false);
	
	button9 = gtk_button_new_with_label("AI go");
	g_signal_connect(button9, "clicked", G_CALLBACK(ai_go), NULL);
	gtk_grid_attach(GTK_GRID(grid), button9, 1, 2, 1, 1);
	
	button10 = gtk_button_new_with_label("AI stop");
	//button = gtk_image_new_from_icon_name("media-seek-forward", GTK_ICON_SIZE_BUTTON);
	g_signal_connect(button10, "clicked", G_CALLBACK(ai_stop), NULL);
	gtk_grid_attach(GTK_GRID(grid), button10, 2, 2, 1, 1);
	//gtk_widget_set_sensitive(button10, false);
	
	button11 = gtk_label_new("0");
	gtk_grid_attach(GTK_GRID(grid), button11, 0, 2, 1, 1);
	//GdkColor color;
	//gdkrgba* rgba = gdk_rgb_new(.8, .1, .1, .4);
	//gdk_rgba_parse (rgba, &color);
	//gtk_widget_override_background_color ( GTK_WIDGET(button11), GTK_STATE_NORMAL, &color);
	button12 = gtk_label_new("0");
	gtk_grid_attach(GTK_GRID(grid), button12, 3, 2, 1, 1);
  
	//drawing canvas
	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawing_area, bd1->CWIDTH, bd1->CHEIGHT);
	gtk_grid_attach(GTK_GRID(grid), drawing_area, 0, 3, 4, 4);
	
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
	players[2] = 0;

	app = gtk_application_new("org.gtk.triangles", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
	
	delete bd1;

	//pthread_exit(NULL);
	return status;
}

