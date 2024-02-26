//=============================================================================================
// Mintaprogram: Z�ld h�romsz�g. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : 
// Neptun : 
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char* const vertexSource = R"(
	#version 330
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char* const fragmentSource = R"(
	#version 330
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

GPUProgram gpuProgram; // vertex and fragment shaders

class Object {
protected:
	unsigned int vao, vbo; // GPU
	std::vector<vec3> vtx; // CPU

public:

	//Constructor
	Object() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}


	std::vector<vec3>& getVtx() { return vtx; }

	// CPU -> GPU
	void updateGPU() {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);
	}


	// Draw
	void Draw(int type, vec3 color) {
		if (vtx.size() > 0) {
			glBindVertexArray(vao);
			gpuProgram.setUniform(color, "color");
			glDrawArrays(type, 0, vtx.size());
		}
	}
};

class PointCollection : public Object {


public:
	PointCollection() : Object() {}

	void addPoint(vec3 p) {
		vtx.push_back(p);
		printf("Point: %f, %f added \n", p.x, p.y );
		updateGPU();
	}

	void Draw(vec3 color) {
		Object::Draw(GL_POINTS, color);
	}

	vec3 findNearestPoint(vec3 p) {
		float minDist = 1000;
		vec3 nearest = vec3(0, 0, 0);

		for (size_t i = 0; i < vtx.size(); i++) {
				float dist = (float) sqrt(pow(p.x - vtx[i].x, 2) + pow(p.y - vtx[i].y, 2));
				if (dist < minDist) {
					minDist = dist;
					nearest = vtx[i];
				}
		}

		float dist = (float) sqrt(pow(p.x - nearest.x, 2) + pow(p.y - nearest.y, 2));

		if (dist < 0.01) {

		return nearest;
		}
		else
		{
			return vec3(0, 0, -1);
		}
	}

};


bool cmpVec3(vec3 v1, vec3 v2) {
	if (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z) {
		return true;
	}
	else {
		return false;
	}
}

class Line {

	vec3 p1, p2;

public:
	Line() : p1(vec3(0, 0, -1)), p2(vec3(0, 0, -1)) {}

	Line(vec3 p1, vec3 p2) : p1(p1), p2(p2) {
	
		printf("Line added\n ");
		printf("\tImplicit: %f x + %f y + %f = 0 \n",(p2.y - p1.y), (p1.x - p2.x), (p2.x * p1.y) - (p1.x * p2.y));
		printf("\tParametric: r(t) = (%f, %f) x + (%f, %f)t\n", p1.x, p2.y, p2.x - p1.x, p2.y - p1.y );
	}


	vec3 getP1() const { return p1; }
	vec3 getP2() const { return p2; }
	void setP1(vec3 p) { p1 = p; }
	void setP2(vec3 p) { p2 = p; }

	// Metszéspont meghatározása egy másik egyenessel
	vec3 getIntersection(Line other) {
	
		float m1 = (p2.y - p1.y) / (p2.x - p1.x);
		float m2 = (other.p2.y - other.p1.y) / (other.p2.x - other.p1.x);

		if (m1 == m2) {
			return vec3(0, 0, -1);
		}

		float b1 = p1.y - m1 * p1.x;
		float b2 = other.p1.y - m2 * other.p1.x;

		float x = (b2 - b1) / (m1 - m2);
		float y = m1 * x + b1;


		if (x < 1 && x > -1 && y > -1 && y < 1) {
			return vec3(x, y, 1);
		}
		else {
			return vec3(0, 0, -1);
		}

		

	}

	bool isPointOnLine(const vec3& point) const {
		// Az egyenes egyenlete
		float distance = (float) fabs((p2.y - p1.y) * point.x - (p2.x - p1.x) * point.y + p2.x * p1.y - p2.y * p1.x) /
			sqrt((p2.y - p1.y) * (p2.y - p1.y) + (p2.x - p1.x) * (p2.x - p1.x));

		// Távolság összehasonlítása a küszöbértékkel
		return distance < 0.01;
	}


	void setEndPoints() {
		vec3 newP1 = vec3(0, 0, -1); // Initialize with default values
		vec3 newP2 = vec3(0, 0, -1);

		// Calculate intersection points with each side of the box
		float topX = (p2.x - p1.x) * (1 - p1.y) / (p2.y - p1.y) + p1.x;
		vec3 top = vec3(topX, 1, 0);

		float bottomX = (p2.x - p1.x) * (-1 - p1.y) / (p2.y - p1.y) + p1.x;
		vec3 bottom = vec3(bottomX, -1, 0);

		float leftY = (p2.y - p1.y) * (-1 - p1.x) / (p2.x - p1.x) + p1.y;
		vec3 left = vec3(-1, leftY, 0);

		float rightY = (p2.y - p1.y) * (1 - p1.x) / (p2.x - p1.x) + p1.y;
		vec3 right = vec3(1, rightY, 0);

		// Update endpoints if intersection points are within the box
		if (topX >= -1 && topX <= 1) {
			if (cmpVec3(newP1, vec3(0,0,-1))) {
				newP1 = top;
			}
			else {
				newP2 = top;
			}
		}

		if (bottomX >= -1 && bottomX <= 1) {
			if (cmpVec3(newP1, vec3(0, 0, -1)) ){
				newP1 = bottom;
			}
			else {
				newP2 = bottom;
			}
		}

		if (leftY >= -1 && leftY <= 1) {
			if (cmpVec3(newP1, vec3(0, 0, -1))) {
				newP1 = left;
			}
			else {
				newP2 = left;
			}
		}

		if (rightY >= -1 && rightY <= 1) {
			if (cmpVec3(newP1, vec3(0, 0, -1))) {
				newP1 = right;
			}
			else {
				newP2 = right;
			}
		}

		p1 = newP1;
		p2 = newP2;
	}



};

bool cmpLine(const Line& l1, const Line& l2) {
	if (l1.getP1().x == l2.getP1().x && l1.getP1().y == l2.getP1().y && l1.getP1().z == l2.getP1().z &&
		l1.getP2().x == l2.getP2().x && l1.getP2().y == l2.getP2().y && l1.getP2().z == l2.getP2().z) {
		return true;
	}
	else {
		return false;
	}
}


// Function to calculate the distance between a point and a line defined by two points
float distancePointToLine(const vec3& point, const vec3& linePoint1, const vec3& linePoint2) {
	// Vector along the line
	vec3 lineVec = linePoint2 - linePoint1;

	// Vector from linePoint1 to the point
	vec3 pointVec = point - linePoint1;

	// Calculate the projection of pointVec onto lineVec
	float projection = dot(pointVec, lineVec) / dot(lineVec, lineVec);

	// Calculate the point on the line closest to the point
	vec3 closestPoint = linePoint1 + lineVec * projection;

	// Calculate the distance between the point and the closest point on the line
	float dist = length(point - closestPoint);

	return dist;
}


Line* cpyLine(Line l) {

	Line* newLine = new Line();
	newLine->setP1(l.getP1());
	newLine->setP2(l.getP2());
	return newLine;

}


class LineCollection : public Object {


	public:
	LineCollection() : Object() {}

	void addLine(vec3 p1, vec3 p2) {
		Line l = Line(p1, p2);
		l.setEndPoints();
		vtx.push_back(l.getP1());
		vtx.push_back(l.getP2());
		updateGPU();
		
	}

	void Draw(vec3 color) {
		Object::Draw(GL_LINES, color);
	}

	Line& findNearestLine(vec3 point) {

		vec3* p1 = nullptr;
		vec3* p2 = nullptr;
		Line l;

		float minDist = 1000; // Initialize with a large value
		Line* nearest = nullptr; // Initialize with nullptr

		for (size_t i = 0; i < vtx.size(); i += 2) { // Iterate over pairs of vertices
			p1 = &vtx[i];
			p2 = &vtx[i + 1];

			l.setP1(*p1);
			l.setP2(*p2);
		
			// Calculate distance from point to line
			float dist = distancePointToLine(point, *p1, *p2);

			if (dist < minDist) {
				minDist = dist;
				nearest = cpyLine(l); // Update nearest pointer
			}
		}

		if (nearest && minDist < 0.02) // Check if nearest is not nullptr
			return *nearest;
		else
			return Line(); // Return a default line
	}
	
	
};



enum mode { LINE, DOT, MOVE, INTERSECTION };
mode currentMode = mode::DOT;

PointCollection * pc;
LineCollection * lc;





// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	pc = new PointCollection();
	lc = new LineCollection();


	glPointSize(10.0f);
	glLineWidth(3.0f);


	Line l1 = Line(vec3(0.5, 0.5, 0), vec3(-0.5, -0.75, 0));
	Line l2 = Line(vec3(-0.5, 0.5, 0), vec3(0.5, -0.5, 0));

	printf("Intersection: %f, %f\n", l1.getIntersection(l2).x, l1.getIntersection(l2).y);

	// create program for the GPU
	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0.32f, 0.32f, 0.32f, 1.0f);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	float MVPtransf[4][4] = { 1, 0, 0, 0,    // MVP matrix, 
							  0, 1, 0, 0,    // row-major!
							  0, 0, 1, 0,
							  0, 0, 0, 1 };

	int location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	// Load a 4x4 row-major float matrix to the specified location


	lc->Draw(vec3(0.0f, 1.0f, 1.0f));
	pc->Draw(vec3(1.0f, 0.0f, 0.0f));
	


	glutSwapBuffers(); // exchange buffers for double buffering

}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key)
	{
	case 'l':
		currentMode = mode::LINE;
		printf("Line mode\n");
		break;
	case'm':
		currentMode = mode::MOVE;
		printf("Move mode\n");
		break;
	case'i':
		currentMode = mode::INTERSECTION;
		printf("Intersection mode\n");
		break;
	case 'p':
		currentMode = mode::DOT;
		printf("Point mode\n");
		break;
	}
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}


Line * nearestLine;
// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	


	switch (currentMode)
	{
	case LINE:
		break;
	case DOT:
		break;
	case MOVE:
		nearestLine = &lc->findNearestLine(vec3(cX, cY, 0));

		if (nearestLine->getP2().z != -1) {


			float dummyX = 0.5f;

			float m = (nearestLine->getP2().y - nearestLine->getP1().y) / (nearestLine->getP2().x - nearestLine->getP1().x);
			float b = nearestLine->getP1().y - m * nearestLine->getP1().x;

			float m2 = -1 / m;
			float b2 = cY - m2 * cX;

			vec3 midPoint = vec3((nearestLine->getP1().x + nearestLine->getP2().x) / 2, (nearestLine->getP1().y + nearestLine->getP2().y) / 2, 0);
			vec3 translationVector = vec3(cX - midPoint.x, cY - midPoint.y, 0);

			vec3 translatePoint1 = vec3(nearestLine->getP1().x + translationVector.x, nearestLine->getP1().y + translationVector.y, 0);
			vec3 translatePoint2 = vec3(nearestLine->getP2().x + translationVector.x, nearestLine->getP2().y + translationVector.y, 0);
	

			std::vector <vec3> * newVtx = &lc->getVtx();
			

			for (size_t i = 0; i < newVtx->size(); i += 2)
			{
				if (cmpVec3(newVtx->at(i), nearestLine->getP1()) && cmpVec3(newVtx->at(i + 1), nearestLine->getP2()) || 
					cmpVec3(newVtx->at(i), nearestLine->getP2()) && cmpVec3(newVtx->at(i + 1), nearestLine->getP1())) {

					Line l;
					l.setP1(translatePoint1);
					l.setP2(translatePoint2);
					l.setEndPoints();


					newVtx->at(i) = l.getP1();
					newVtx->at(i + 1) = l.getP2();
				}
			}


			lc->updateGPU();
			glutPostRedisplay();
			
		}
		
		break;
	case INTERSECTION:
		break;
	default:
		break;
	}


	
}


vec3 lp1 = NULL;
vec3 lp2 = NULL;

Line* l1 = nullptr;
Line* l2 = nullptr;


// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	vec3 nearestPoint;
	vec3* intersection;
	
	Line * tmp = nullptr;

	char* buttonStat;
	switch (state) {
	case GLUT_DOWN: 
		buttonStat = "pressed";
		break;
	case GLUT_UP:  
		buttonStat = "released";
		break;
	}

	if (buttonStat == "pressed" && button == GLUT_LEFT_BUTTON) {
	
		switch (currentMode)
		{
		case LINE:

		 nearestPoint = pc->findNearestPoint(vec3(cX, cY, 1));

			if (nearestPoint.z != -1) {
				if (cmpVec3(lp1, NULL)) {
					lp1 = vec3(nearestPoint.x, nearestPoint.y, 0);
				}
				else {
					if (!cmpVec3(lp1, vec3(nearestPoint.x, nearestPoint.y, 0))) {
					lp2 = vec3(nearestPoint.x, nearestPoint.y, 0);
					lc->addLine(lp1, lp2);

					lp1 = NULL;
					lp2 = NULL;
				}
				}
			}
			break;
		case DOT:
			pc->addPoint(vec3(cX, cY, 1));
			break;
		case MOVE:
			break;
		case INTERSECTION:

			 tmp = &lc->findNearestLine(vec3(cX, cY, 0));

			 if (tmp->getP2().z != -1) {
				 
				 if (l1 == nullptr) {
					 l1 = tmp;
					 tmp = nullptr;
				 }
				 else {

					 if (!cmpLine(*l1, *tmp)) {
						 l2 = tmp;
						 tmp = nullptr;

						 intersection = new vec3(l1->getIntersection(*l2));
						 if (intersection->z != -1) {
							 pc->addPoint(*intersection);
						 }

						 delete intersection;
						 delete l1;
						 delete l2;

						 l1 = nullptr;
						 l2 = nullptr;
						 intersection = nullptr;
					 
					 }
				 }
			 }

			

			break;
		default:
			break;
		}

	}

	glutPostRedisplay();
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}


