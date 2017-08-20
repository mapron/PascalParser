/*
 * Copyright (C) 2017 Smirnov Vladimir mapron1@gmail.com
 * Source code licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 or in file COPYING-APACHE-2.0.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.h
 */

#include <CompilerFrontend.h>
#include <ast.h>

#include <QCoreApplication>
#include <QFile>

#include <iostream>
int main1();
int main2();
int main(int argc, char *argv[])
{
	/*main1();
	main2();
	return 0;*/
	QCoreApplication application( argc, argv );
	auto args = application.arguments();
	if (args.size() < 3)
		return 1;

	QFile input(args[1]);
	if (!input.open(QIODevice::ReadOnly))
		return 1;

	QString pascalSource = QString::fromUtf8(input.readAll()), cSource;

	CompilerFrontend compiler;
	if (!compiler.pascal2c(pascalSource, cSource))
	{
		for (auto & message : compiler.messages()._messages)
		{
			std::cerr << message.toString().toUtf8().constData() << std::endl;
		}
		return 1;
	}

	QFile output(args[2]);
	if (!output.open(QIODevice::WriteOnly))
		return 1;

	output.write(cSource.toUtf8());

	return 0;
}

#define Low(x) 0
#define High(x) (sizeof(x)/sizeof(x[0])-1)
#define WRITELN(x) std::cout << x << std::endl
#define sqr(x) ((x) * (x))
#include <cmath>
#include <iostream>

//USES Math;
	typedef  struct  {
		double x, y, z, vx, vy, vz, mass;
	} Body;
	typedef Body* PBody;
	const auto pi = 3.141592653589793;
	const auto solarMass = 4 * sqr(pi);
	const auto daysPerYear = 365.24;
	typedef Body  tbody;
	tbody b[5 - 1 + 1] = { { 0, 0, 0, 0, 0, 0, solarMass },
					   { 4.84143144246472, -1.16032004402743, -0.103622044471123, 0.00166007664274404 * daysPerYear, 0.0076990111841974 * daysPerYear, -6.90460016972063e-05 * daysPerYear, 0.000954791938424327 * solarMass },
					   { 8.34336671824458, 4.1247985641243, -0.403523417114321, -0.00276742510726862 * daysPerYear, 0.00499852801234917 * daysPerYear, 2.30417297573764e-05 * daysPerYear, 0.000285885980666131 * solarMass },
					   { 12.8943695621391, -15.1111514016986, -0.223307578892656, 0.00296460137564762 * daysPerYear, 0.00237847173959481 * daysPerYear, -2.96589568540238e-05 * daysPerYear, 4.36624404335156e-05 * solarMass },
					   { 15.3796971148509, -25.919314609988, 0.179258772950371, 0.00268067772490389 * daysPerYear, 0.00162824170038242 * daysPerYear, -9.51592254519716e-05 * daysPerYear, 5.15138902046611e-05 * solarMass } };
void offsetMomentum()
{
	double px, py, pz;
	int i;
	px = 0;
	py = 0;
	pz = 0;
	for (int i = Low(b) + 1; i<=High(b); i++)
		{ auto &_l2 = b[i];
		  auto &vx =_l2.vx;auto &vy =_l2.vy; auto &vz =_l2.vz; auto &mass =_l2.mass;
			{
				px = px - vx * mass;
				py = py - vy * mass;
				pz = pz - vz * mass;
			};
		};;
	b[Low(b)].vx = px / solarMass;
	b[Low(b)].vy = py / solarMass;
	b[Low(b)].vz = pz / solarMass;
};
double distance(int i, int j)
{
	double Result;
	double dx = b[i].x - b[j].x;
	double dy = b[i].y - b[j].y;
	double dz = b[i].z - b[j].z;
	Result = sqrt(sqr(dx) + sqr(dy) + sqr(dz));
	//std::cout << "dx=" << dx << ", dy=" << dy << ", dz=" << dz << ", distance=" <<  Result << std::endl;
	return Result;
};
double energy()
{
	double Result;
	int i, j;
	Result = 0;
	for (int i = Low(b); i<=High(b); i++)
		{ auto &_l2 = b[i];
		  auto &vx =_l2.vx;auto &vy =_l2.vy; auto &vz =_l2.vz; auto &mass =_l2.mass;
			{
				Result = Result + mass * (sqr(vx) + sqr(vy) + sqr(vz)) * 0.5;
				for (int j = i + 1; j<=High(b); j++)
				{
					Result = Result - mass * b[j].mass / distance(i, j);;
				}
			};
		};;
	return Result;
};
void advance(double dt)
{
	int i, j;
	double dx, dy, dz, mag, dsquared;
	PBody bi, bj;
	for (int i = Low(b); i<=High(b) - 1; i++)
		{
			bi = &(b[i]);
			bj = bi;
			for (int j = i + 1; j<=High(b); j++)
				{
					bj = &(b[j]);
					dx = (*bi).x - (*bj).x;
					dy = (*bi).y - (*bj).y;
					dz = (*bi).z - (*bj).z;
					dsquared = sqr(dx) + sqr(dy) + sqr(dz);
					mag = dt / (sqrt(dsquared) * dsquared);
					(*bi).vx = (*bi).vx - dx * (*bj).mass * mag;
					(*bi).vy = (*bi).vy - dy * (*bj).mass * mag;
					(*bi).vz = (*bi).vz - dz * (*bj).mass * mag;
					(*bj).vx = (*bj).vx + dx * (*bi).mass * mag;
					(*bj).vy = (*bj).vy + dy * (*bi).mass * mag;
					(*bj).vz = (*bj).vz + dz * (*bi).mass * mag;
				};;
		};;
	for (int i = Low(b); i<=High(b); i++)
		{
			bi = &(b[i]);
			{ auto &_l3 = (*bi);
				auto &vx =_l3.vx;auto &vy =_l3.vy; auto &vz =_l3.vz; auto &x =_l3.x;auto &y =_l3.y;auto &z =_l3.z;
				{
					x = x + dt * vx;
					y = y + dt * vy;
					z = z + dt * vz;
				};
			};
		};;
};
	int i;
	int n;
 int main2()
{
	offsetMomentum();
	WRITELN (energy());
	n = 1000;
	for (int i = 1; i<=n; i++)
		advance(0.01);;
	WRITELN (energy());
}


#include <algorithm>
#include <stdio.h>
#include <cmath>
#include <stdlib.h>
#include <immintrin.h>
#include <array>

constexpr double PI(3.141592653589793);
constexpr double SOLAR_MASS ( 4 * PI * PI );
constexpr double DAYS_PER_YEAR(365.24);

struct body {
  double x[3], fill, v[3], mass;
  constexpr body(double x0, double x1, double x2, double v0, double v1, double v2,  double Mass):
	x{x0,x1,x2}, fill(0), v{v0,v1,v2}, mass(Mass) {}
};

class N_Body_System
{
  static std::array<body,5> bodies;

  void offset_momentum()
  {
	unsigned int k;
	for(auto &body: bodies)
	  for(k = 0; k < 3; ++k)
		bodies[0].v[k] -= body.v[k] * body.mass / SOLAR_MASS;
  }

public:
  N_Body_System()
  {
	offset_momentum();
  }
  void advance(double dt)
  {
	constexpr unsigned int N = ((bodies.size() - 1) * bodies.size()) / 2;

	static double r[N][4];
	static double mag[N];

	unsigned int i, m;
	__m128d dx[3], dsquared, distance, dmag;

	i=0;
	for(auto bi(bodies.begin()); bi!=bodies.end(); ++bi)
	  {
		auto bj(bi);
		for(++bj; bj!=bodies.end(); ++bj, ++i)
		  for (m=0; m<3; ++m)
			r[i][m] = bi->x[m] - bj->x[m];
	  }

	for (i=0; i<N; i+=2)
	  {
		for (m=0; m<3; ++m)
		  {
			dx[m] = _mm_loadl_pd(dx[m], &r[i][m]);
			dx[m] = _mm_loadh_pd(dx[m], &r[i+1][m]);
		  }

		dsquared = dx[0] * dx[0] + dx[1] * dx[1] + dx[2] * dx[2];
		distance = _mm_cvtps_pd(_mm_rsqrt_ps(_mm_cvtpd_ps(dsquared)));

		for (m=0; m<2; ++m)
		  distance = distance * _mm_set1_pd(1.5)
			- ((_mm_set1_pd(0.5) * dsquared) * distance)
			* (distance * distance);

		dmag = _mm_set1_pd(dt) / (dsquared) * distance;
		_mm_store_pd(&mag[i], dmag);
	  }

	i=0;
	for(auto bi(bodies.begin()); bi!=bodies.end(); ++bi)
	  {
		auto bj(bi);
		for(++bj; bj!=bodies.end(); ++bj, ++i)
		  for(m=0; m<3; ++m)
			{
			  const double x = r[i][m] * mag[i];
			  bi->v[m] -= x * bj->mass;
			  bj->v[m] += x * bi->mass;
			}
	  }

	for(auto &body: bodies)
	  for(m=0; m<3; ++m)
		body.x[m] += dt * body.v[m];
  }

  double energy()
  {
	double e(0.0);
	for(auto bi(bodies.cbegin()); bi!=bodies.cend(); ++bi)
	  {
		e += bi->mass * ( bi->v[0] * bi->v[0]
						  + bi->v[1] * bi->v[1]
						  + bi->v[2] * bi->v[2] ) / 2.;
		//std::cout << "!e=" << e <<  std::endl;
		auto bj(bi);
		for(++bj; bj!=bodies.end(); ++bj)
		  {
			double distance = 0;
			for(auto k=0; k<3; ++k)
			{
			  const double dx = bi->x[k] - bj->x[k];
			  distance += dx * dx;
			}
			//std::cout << "dx=" << (bi->x[0] - bj->x[0]) << ", dy=" << (bi->x[1] - bj->x[1]) << ", dz=" << (bi->x[2] - bj->x[2]) << ", distance=" <<  sqrt(distance) << std::endl;

			e -= (bi->mass * bj->mass) / std::sqrt(distance);
			//std::cout << "e=" << e << ", distance=" <<std::sqrt(distance) <<  std::endl;
		  }
	  }
	return e;
  }
};


std::array<body,5> N_Body_System::bodies{{
	/* sun */
	body(0., 0., 0. ,
		 0., 0., 0. ,
		 SOLAR_MASS),
	/* jupiter */
	body(4.84143144246472090e+00,
		 -1.16032004402742839e+00,
		 -1.03622044471123109e-01 ,
		 1.66007664274403694e-03 * DAYS_PER_YEAR,
		 7.69901118419740425e-03 * DAYS_PER_YEAR,
		 -6.90460016972063023e-05 * DAYS_PER_YEAR ,
		 9.54791938424326609e-04 * SOLAR_MASS
		 ),
	/* saturn */
	body(8.34336671824457987e+00,
		 4.12479856412430479e+00,
		 -4.03523417114321381e-01 ,
		 -2.76742510726862411e-03 * DAYS_PER_YEAR,
		 4.99852801234917238e-03 * DAYS_PER_YEAR,
		 2.30417297573763929e-05 * DAYS_PER_YEAR ,
		 2.85885980666130812e-04 * SOLAR_MASS
		 ),
	/* uranus */
	body(1.28943695621391310e+01,
		 -1.51111514016986312e+01,
		 -2.23307578892655734e-01 ,
		 2.96460137564761618e-03 * DAYS_PER_YEAR,
		 2.37847173959480950e-03 * DAYS_PER_YEAR,
		 -2.96589568540237556e-05 * DAYS_PER_YEAR ,
		 4.36624404335156298e-05 * SOLAR_MASS
		 ),
	/* neptune */
	body(1.53796971148509165e+01,
		 -2.59193146099879641e+01,
		 1.79258772950371181e-01 ,
		 2.68067772490389322e-03 * DAYS_PER_YEAR,
		 1.62824170038242295e-03 * DAYS_PER_YEAR,
		 -9.51592254519715870e-05 * DAYS_PER_YEAR ,
		 5.15138902046611451e-05 * SOLAR_MASS
		 )
  }};

int main1()
{
  int i, n = 1000;
  N_Body_System system;

  printf("%.9f\n", system.energy());
  for (i = 0; i < n; ++i)
	system.advance(0.01);
  printf("%.9f\n", system.energy());

  return 0;
}
