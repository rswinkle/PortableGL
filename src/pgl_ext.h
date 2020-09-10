
void clear_screen();

//TODO
//pglDrawRect(x, y, w, h)
//pglDrawPoint(x, y)
void pglDrawFrame();

void put_pixel(Color color, int x, int y);

//Should I have it take a glFramebuffer as paramater?
void put_line(Color the_color, float x1, float y1, float x2, float y2);

void put_triangle(Color c1, Color c2, Color c3, vec2 p1, vec2 p2, vec2 p3);

