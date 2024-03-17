//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
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
// Nev    : Buga Péter
// Neptun : G50RDF
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
const char * const vertexSource = R"(
	#version 330
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";


class Camera2D {
	vec2 wCenter; // center in world coordinates
	vec2 wSize;   // width and height in world coordinates
public:
	Camera2D() : wCenter(0, 0), wSize(30, 30) { }

	mat4 V() { return TranslateMatrix(-wCenter); }
	mat4 P() { return ScaleMatrix(vec2(2 / wSize.x, 2 / wSize.y)); }

	mat4 Vinv() { return TranslateMatrix(wCenter); }
	mat4 Pinv() { return ScaleMatrix(vec2(wSize.x / 2, wSize.y / 2)); }

	void Zoom(float s) { wSize = wSize * s; }
	void Pan(vec2 t) { wCenter = wCenter + t; }
};

enum mode {
	Lagrange,
	Bezier,
	Catmull
};

Camera2D camera;		
GPUProgram gpuProgram;	
mode currentMode = Lagrange;
float tension = 0.0f;

class Curve {

protected:
	std::vector<vec2> cps; 
	unsigned int vao, vbo; 
	std::vector<vec2> vertices; 

public:

	Curve() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	virtual ~Curve() {
	}

	virtual vec2 r(float t) = 0;


	virtual void calcVertices() = 0;

	void Draw() {

		if (cps.size() > 0) {

			mat4 MVPTransform = camera.V() * camera.P();
			gpuProgram.setUniform(MVPTransform, "MVP");

			
			glBindVertexArray(vao);  // Draw call
			glBindBuffer(GL_ARRAY_BUFFER, vbo);


			gpuProgram.setUniform(vec3(1.0f, 1.0f, 0.0f), "color");
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec2), &vertices[0], GL_DYNAMIC_DRAW);
			glDrawArrays(GL_LINE_STRIP, 0, vertices.size());

			gpuProgram.setUniform(vec3(1.0f, 0.0f, 0.0f), "color");
			glBufferData(GL_ARRAY_BUFFER, cps.size() * sizeof(vec2), &cps[0], GL_DYNAMIC_DRAW);
			glDrawArrays(GL_POINTS, 0, cps.size());
		}
	}

	void AddControlPoint(float cX, float cY) { 
		
		vec4 mVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		cps.push_back(vec2(mVertex.x, mVertex.y)); 
		
	
	}

	float distance(vec2 p1, vec2 p2) {
		return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
	}

	vec2* pickPoint(float cX, float cY) {

		vec4 mPoint = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
		vec2 point = vec2(mPoint.x, mPoint.y);

		for (unsigned int i = 0; i < cps.size(); i++) {
		
			if (dot(cps[i] - point, cps[i] - point) < 0.1) {
				return &cps[i];
			}

		}
		return nullptr;
	}
};

class BezierCurve : public Curve {

	float B(int i, float t) {
		int n = cps.size() - 1; // n+1 pts!
		float choose = 1;
		for (int j = 1; j <= i; j++) choose *= (float)(n - j + 1) / j;
		return choose * pow(t, i) * pow(1 - t, n - i);
	}
public:

	vec2 r(float t) {
		vec2 rt(0, 0);
		for ( unsigned int i = 0; i < cps.size(); i++) rt = rt + (cps[i] * B(i, t));
		return rt;
	}

	void calcVertices() {
		vertices.clear();
		if (cps.size() > 1) {
			for (int i = 0; i < 100; i++) {
				float t = (float)i / 99;
				vec2 p = r(t);
				vertices.push_back(p);
			}
		}
	}
};

class CatmullRom : public Curve {
	std::vector<float> ts;
	std::vector<float> dst;

public:
	void AddControlPoint(float cX, float cY) {
		
		Curve::AddControlPoint(cX, cY);
		
		
		if (cps.size() > 2) {

			float dist2 = distance(cps[cps.size()-1], cps[cps.size()-2]);
			dst.push_back( dst[dst.size()-1] + dist2);
			ts.push_back(1);

			for (unsigned int i = 0; i < ts.size()-1; i++) {
;
				ts[i] = dst[i] / dst[dst.size()-1];
			}
		}
		else
		{
			if (cps.size() < 2) {
				ts.push_back(0);
				dst.push_back(0);
			}
			else
			{
				ts.push_back(1);
				dst.push_back(distance( cps[0], cps[1]));
			}
		}
	}
	vec2 Hermite(vec2 p0, vec2 v0, float t0, vec2 p1, vec2 v1, float t1,
		float t) {

		vec2 a0 = p0;
		vec2 a1 = v0;

		vec2 a2 = ((3 * (p1 - p0)) / pow((t1 - t0), 2)) - ((v1 + (2 * v0)) / (t1 - t0));
		vec2 a3 = (2 * (p0 - p1)) / (pow((t1 - t0), 3)) + ((v1 + v0) / (pow((t1 - t0), 2)));
		return (a3  * (pow((t - t0), 3))) + (a2 * (pow((t - t0), 2))) + (a1 * (t - t0)) + a0;

	}
	vec2 r(float t) {
		for (unsigned int i = 0; i < cps.size() - 1; i++) {
			if (ts[i] <= t && t <= ts[i + 1]) {
				
				vec2 old;

				if (i > 0) {
					old = (cps[i] - cps[i - 1]) / (ts[i] - ts[i - 1]);
				}
				else
				{
					old = vec2(0, 0);
				}

				vec2 current = (cps[i + 1] - cps[i]) / (ts[i + 1] - ts[i]);

				vec2 vNew;

				if( i < cps.size() -2){
					vNew = (cps[i + 2] - cps[i + 1]) / (ts[i + 2] - ts[i + 1]);
				}
				else
				{
					vNew = vec2(0, 0);
				}
	
				vec2 v0 = ((1 - tension) / 2) * (old + current);
				vec2 v1 = ((1 - tension) / 2) * (current + vNew);
				
				return Hermite(cps[i], v0, ts[i], cps[i + 1], v1, ts[i + 1], t);
			}
		}
		return cps[0];
	}

	void calcVertices() {
		vertices.clear();

		if (cps.size() >= 2 ) {
			for (int i = 0; i < 100; i++) {
				
				float nt = (float)i / 99;
				float t = nt * (ts[ts.size() - 1]);
				
				vec2 p = r(t);
				vertices.push_back(p);
			}
		}

	}
};

class LagrangeCurve : public Curve {
	std::vector<float> ts;
	std::vector<float> dst; 

	float L(unsigned int i, float t) {
		float Li = 1.0f;
		for (unsigned int j = 0; j < cps.size(); j++)
			if (j != i)
				Li *= (t - ts[j]) / (ts[i] - ts[j]);
		return Li;
	}
public:
	void AddControlPoint(float cX, float cY) {

	
		Curve::AddControlPoint(cX, cY);


		if (cps.size() > 2) {

			float dist2 = distance(cps[cps.size() - 1], cps[cps.size() - 2]);
			dst.push_back(dst[dst.size() - 1] + dist2);
			ts.push_back(1);

			for (unsigned int i = 0; i < ts.size() - 1; i++) {
				;
				ts[i] = dst[i] / dst[dst.size() - 1];
			}
		}
		else
		{
			if (cps.size() < 2) {
				ts.push_back(0);
				dst.push_back(0);
			}
			else
			{
				ts.push_back(1);
				dst.push_back(distance(cps[0], cps[1]));
			}
		}
		
		
	}

	vec2 r(float t) {
		vec2 rt(0, 0);
		for (unsigned int i = 0; i < cps.size(); i++) rt = rt + cps[i] * L(i, t);
		return rt;
	}

	void calcVertices() {
		vertices.clear();

		if (cps.size() > 1) {
			for (int i = 0; i < 100; i++) {
				float nt = (float)i / 99;
				
				float t = (ts[cps.size()-1]) * nt;
				vec2 p = r(t);
				vertices.push_back(p);
			}
		}
	}
};

LagrangeCurve * lc;
BezierCurve * bc;
CatmullRom * cm;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	glPointSize(10.0f);
	glLineWidth(2.0f);

	lc = new LagrangeCurve();
	bc = new BezierCurve();
	cm = new CatmullRom();

	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}


void onDisplay() {
	glClearColor(0, 0, 0, 0);     
	glClear(GL_COLOR_BUFFER_BIT); 

	switch (currentMode)
	{
	case Lagrange:
		lc->calcVertices();
		lc->Draw();
		break;
	case Bezier:
		bc->calcVertices();
		bc->Draw();
		break;
	case Catmull:
		cm->calcVertices();
		cm->Draw();
		break;
	default:
		break;
	}
	glutSwapBuffers(); 
}

void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 'P': camera.Pan(vec2(+1, 0));  break;
	case 'p': camera.Pan(vec2(-1, 0)); break;
	case 'z': camera.Zoom(1 / 1.1); break;
	case 'Z': camera.Zoom(1.1f); break;
	case 'l': currentMode = Lagrange; delete lc; lc = new LagrangeCurve();  break;
	case 'b': currentMode = Bezier; delete bc; bc = new BezierCurve(); break;
	case 'c': currentMode = Catmull; delete cm; cm = new CatmullRom(); break;
	case 'T': tension += 0.1f; break;
	case 't': tension -= 0.1f; break;
	}
	glutPostRedisplay();
}


void onKeyboardUp(unsigned char key, int pX, int pY) {
}

vec2* point = nullptr;


void onMouseMotion(int pX, int pY) {	
	
	float cX = 2.0f * pX / windowWidth - 1;	
	float cY = 1.0f - 2.0f * pY / windowHeight;

	if (point != nullptr) {

		vec4 hPoint = vec4(cX, cY, 0,1) * camera.Pinv() * camera.Vinv();
		point->x = hPoint.x;
		point->y = hPoint.y;
	}

	glutPostRedisplay();

	
}



void onMouse(int button, int state, int pX, int pY) { 
	
	float cX = 2.0f * pX / windowWidth - 1;	
	float cY = 1.0f - 2.0f * pY / windowHeight;



	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {

		vec2* testPoint = nullptr;

		switch (currentMode)
		{
		case Lagrange:
			testPoint = lc->pickPoint(cX, cY);

			if (testPoint != nullptr) {
				point = testPoint;
			}
			else {
				point = nullptr;
			}
			testPoint = nullptr;
			break;

			break;
		case Bezier:
			testPoint = bc->pickPoint(cX, cY);

			if (testPoint != nullptr) {
				point = testPoint;
			}
			else {
				point = nullptr;
			}
			testPoint = nullptr;
			break;

			break;
		case Catmull:
			testPoint = cm->pickPoint(cX, cY);

			if (testPoint != nullptr) {
				point = testPoint;
			}
			else {
				point = nullptr;
			}
			testPoint = nullptr;
			break;
		}

	}

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		switch (currentMode)
		{
		case Lagrange:
			lc->AddControlPoint(cX, cY);
			break;
		case Bezier:
			bc->AddControlPoint(cX, cY);
			break;
		case Catmull:
			cm->AddControlPoint(cX, cY);
			break;
		default:
			break;
		}
}

void onIdle() {
}
