#ifndef RME_SHADE_DRAWER_H_
#define RME_SHADE_DRAWER_H_

struct DrawContext;

class ShadeDrawer {
public:
	ShadeDrawer();
	~ShadeDrawer();

	void draw(const DrawContext& ctx);
};

#endif
