#include <iostream>
#include <vector>

// Resolution of screen
const unsigned int windowWidth = 600, windowHeight = 600;

//--------------------------
struct vec2 {
	//--------------------------
	float x, y;

	vec2(float x0 = 0, float y0 = 0) { x = x0; y = y0; }
	vec2 operator*(float a) const { return vec2(x * a, y * a); }
	vec2 operator/(float a) const { return vec2(x / a, y / a); }
	vec2 operator+(const vec2& v) const { return vec2(x + v.x, y + v.y); }
	vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }
	vec2 operator*(const vec2& v) const { return vec2(x * v.x, y * v.y); }
	vec2 operator-() const { return vec2(-x, -y); }
};

inline float dot(const vec2& v1, const vec2& v2) {
	return (v1.x * v2.x + v1.y * v2.y);
}

inline float length(const vec2& v) { return sqrtf(dot(v, v)); }

inline vec2 normalize(const vec2& v) { return v * (1 / length(v)); }

inline vec2 operator*(float a, const vec2& v) { return vec2(v.x * a, v.y * a); }

//--------------------------
struct vec3 {
	//--------------------------
	float x, y, z;

	vec3(float x0 = 0, float y0 = 0, float z0 = 0) { x = x0; y = y0; z = z0; }
	vec3(vec2 v) { x = v.x; y = v.y; z = 0; }

	vec3 operator*(float a) const { return vec3(x * a, y * a, z * a); }
	vec3 operator/(float a) const { return vec3(x / a, y / a, z / a); }
	vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
	vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
	vec3 operator*(const vec3& v) const { return vec3(x * v.x, y * v.y, z * v.z); }
	vec3 operator-()  const { return vec3(-x, -y, -z); }
};

inline float dot(const vec3& v1, const vec3& v2) { return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z); }

inline float length(const vec3& v) { return sqrtf(dot(v, v)); }

inline vec3 normalize(const vec3& v) { return v * (1 / length(v)); }

inline vec3 cross(const vec3& v1, const vec3& v2) {
	return vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

inline vec3 operator*(float a, const vec3& v) { return vec3(v.x * a, v.y * a, v.z * a); }

class BezierCurve {
	std::vector<vec3> cps; // control pts
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

class CatmullRom {
	std::vector<vec3> cps; // control points
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

class LagrangeCurve {
	std::vector<vec3> cps; // control pts 
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

int main()
{
	BezierCurve bc;
	bc.AddControlPoint(vec3(6, 1, 0));
	bc.AddControlPoint(vec3(6, 6, 2));
	bc.AddControlPoint(vec3(4, 8, 3));
	auto Bezier = bc.r(1);

	CatmullRom cr;
	cr.AddControlPoint(vec3(5,1), 0);
	cr.AddControlPoint(vec3(2, 10), 1);
	cr.AddControlPoint(vec3(5, 7), 2);
	cr.AddControlPoint(vec3(2, 5), 3);
	auto Catmull= cr.r(1.5);

	LagrangeCurve lc;
	lc.AddControlPoint(vec3(4, 8, 0));
	lc.AddControlPoint(vec3(7, 9, 1));
	lc.AddControlPoint(vec3(4, 4, 2));
	auto Lagrange = lc.r(1);

	printf("Bezier: %f\n", Bezier.x);
	printf("Catmull: %f\n", Catmull.x);
	printf("Lagrange: %f", Lagrange.x);

}




