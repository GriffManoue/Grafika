//=============================================================================================
// Mintaprogram: Zold haromszog. Ervenyes 2019. osztol.
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
// Nev    : Buga Peter
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

// vertex shader in GLSL
const char* vertexSource = R"(
	#version 330				
    precision highp float;

	uniform mat4 MVP;			// Model-View-Projection matrix in row-major format

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0
	layout(location = 1) in vec3 vertexColor;	    // Attrib Array 1
	
	out vec3 color;									// output attribute

	void main() {
		color = vertexColor;														// copy color from input to output
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char* fragmentSource = R"(
	#version 330
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

// 2D camera
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

Camera2D camera;		// 2D camera
GPUProgram gpuProgram;	// vertex and fragment shaders
mode currentMode = Lagrange;
float tension = 0.0f;	

class Curve {

protected:
	std::vector<vec3> cps; // control pts
	unsigned int vao, vbo; // vertex array object, vertex buffer object
	std::vector<vec3> vertices; // vertices to be drawn

public:

	Curve() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	virtual vec3 r(float t) = 0;


	void calcVertecies() {
		vertices.clear();
		for (int i = 0; i < 100; i++) {
			float t = (float)i / 100;
			vec3 p = r(t);
			vertices.push_back(p);
		}
	}

	void updateGPU() {
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), &vertices[0], GL_DYNAMIC_DRAW);
	}


	void Draw() {


		glBindVertexArray(vao);
		gpuProgram.setUniform(vec3(1.0f, 0.0f, 0.0f), "color");


		glBufferData(GL_ARRAY_BUFFER, cps.size() * sizeof(vec3), &cps[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_POINTS, 0, cps.size());
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

	void AddControlPoint(vec3 cp) { cps.push_back(cp); }
	vec3 r(float t) {
		vec3 rt(0, 0, 0);
		for (int i = 0; i < cps.size(); i++) rt = rt + (cps[i] * B(i, t));
		return rt;
	}
};

class CatmullRom: public Curve {
	std::vector<float> ts; // parameter (knot) values

public:
	void AddControlPoint(vec3 cp, float t) {
		cps.push_back(cp);
		ts.push_back(t);
	}
	vec3 Hermite(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1,
		float t) {

		vec3 a0 = p0;
		vec3 a1 = v0;

		vec3 a2 = ((3 * (p1 - p0)) / pow((t1 - t0), 2)) - ((v1 + (2 * v0)) / (t1 - t0));
		vec3 a3 = (2 * (p0 - p1)) / (pow((t1 - t0), 3)) + ((v1 + v0) / (pow((t1 - t0), 2)));

		return (a3 * (pow((t - t0), 3))) + (a2 * (pow((t - t0), 2))) + (a1 * (t - t0)) + a0;

	}
	vec3 r(float t) {
		for (int i = 0; i < cps.size() - 1; i++)
			if (ts[i] <= t && t <= ts[i + 1]) {
				vec3 v0 = 0.5 * ((cps[i + 1] - cps[i]) / (ts[i + 1] - ts[i])) + (cps[i] - cps[i - 1]) / (ts[i] - ts[i - 1]);
				vec3 v1 = 0.5 * ((cps[i + 2] - cps[i + 1]) / (ts[i + 2] - ts[i + 1])) + (cps[i + 1] - cps[i]) / (ts[i + 1] - ts[i]);

				return Hermite(cps[i], v0, ts[i], cps[i + 1], v1, ts[i + 1], t);
			}
	}
};

class LagrangeCurve : public Curve {
	std::vector<float> ts; // knots

	float L(int i, float t) {
		float Li = 1.0f;
		for (int j = 0; j < cps.size(); j++)
			if (j != i)
				Li *= (t - ts[j]) / (ts[i] - ts[j]);
		return Li;
	}
public:
	void AddControlPoint(vec3 cp) {
		float ti = cps.size(); // or something better
		cps.push_back(cp); ts.push_back(ti);
	}
	vec3 r(float t) {
		vec3 rt(0, 0, 0);
		for (int i = 0; i < cps.size(); i++) rt = rt + cps[i] * L(i, t);
		return rt;
	}
};

	



// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight); 	// Position and size of the photograph on screen

	glPointSize(10.0f);
	glLineWidth(2.0f);


	LagrangeCurve lagrange;
	lagrange.AddControlPoint(vec3(0, 0, 0));
	lagrange.Draw();


	
	// create program for the GPU
	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}


// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);							// background color 
	glClear(GL_COLOR_BUFFER_BIT); // clear the screen



	mat4 MVPTransform = camera.V() * camera.P();
	gpuProgram.setUniform(MVPTransform, "MVP");

	glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 's': camera.Pan(vec2(-1, 0)); break;
	case 'd': camera.Pan(vec2(+1, 0)); break;
	case 'e': camera.Pan(vec2(0, 1)); break;
	case 'x': camera.Pan(vec2(0, -1)); break;
	case 'z': camera.Zoom(1 / 1.1); break;
	case 'Z': camera.Zoom(1.1f); break;
	case 'l': currentMode = Lagrange; break;
	case 'b': currentMode = Bezier; break;
	case 'c': currentMode = Catmull; break;
	case 'T': tension += 0.1f; break;
	case 't': tension -= 0.1f; break;
	}
	glutPostRedisplay();
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
	
	glutPostRedisplay();

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	
}
