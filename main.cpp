#include <GLFW\glfw3.h>
#include "linmath.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <vector>
#include <windows.h>
#include <time.h>

using namespace std;

const float DEG2RAD = 3.14159 / 180;

void processInput(GLFWwindow* window);

enum BRICKTYPE { REFLECTIVE, DESTRUCTABLE };
enum ONOFF { ON, OFF };

class Brick
{
public:
	float red, green, blue;
	float x, y, width, height;
	BRICKTYPE brick_type;
	ONOFF onoff;
	int hitCount; // Number of hits required to destroy the brick

	Brick(BRICKTYPE bt, float xx, float yy, float ww, float hh, float rr, float gg, float bb)
		: brick_type(bt), x(xx), y(yy), width(ww), height(hh), red(rr), green(gg), blue(bb), onoff(ON), hitCount(3)
	{
	}

	void drawBrick()
	{
		if (onoff == ON)
		{
			glColor3d(red, green, blue);
			glBegin(GL_POLYGON);

			glVertex2d(x + width / 2, y + height / 2);
			glVertex2d(x + width / 2, y - height / 2);
			glVertex2d(x - width / 2, y - height / 2);
			glVertex2d(x - width / 2, y + height / 2);

			glEnd();
		}
	}

	void handleCollision()
	{
		hitCount--;
		if (hitCount <= 0)
		{
			onoff = OFF; // Brick is destroyed when hit count reaches 0
		}
		else
		{
			// Change the color of the brick upon each hit
			red -= 0.1f;
			green -= 0.1f;
			blue -= 0.1f;
		}
	}
};

class Paddle
{
public:
	float x, y, width, height;
	float red, green, blue;

	Paddle(float xx, float yy, float ww, float hh, float rr, float gg, float bb)
	{
		x = xx; y = yy; width = ww; height = hh; red = rr; green = gg; blue = bb;
	}

	void moveLeft()
	{
		if (x - width / 2 > -1.0)
			x -= 0.1f;
	}

	void moveRight()
	{
		if (x + width / 2 < 1.0)
			x += 0.1f;
	}

	void drawPaddle()
	{
		glColor3f(red, green, blue);
		glBegin(GL_POLYGON);
		glVertex2f(x - width / 2, y - height / 2);
		glVertex2f(x + width / 2, y - height / 2);
		glVertex2f(x + width / 2, y + height / 2);
		glVertex2f(x - width / 2, y + height / 2);
		glEnd();
	}
};


class Circle
{
public:
	float red, green, blue;
	float radius;
	float x;
	float y;
	float speed = 0.03;
	int direction; // 1=up 2=right 3=down 4=left 5 = up right   6 = up left  7 = down right  8= down left
	float angle; // New addition: angle of movement in degrees

	Circle(double xx, double yy, double rr, int dir, float rad, float r, float g, float b)
	{
		x = xx;
		y = yy;
		radius = rr;
		red = r;
		green = g;
		blue = b;
		radius = rad;
		direction = dir;
		angle = 45.0f; // Initial angle of movement
	}

	void CheckCollision(Brick* brk, Paddle* paddle, vector<Circle>& circles)
	{
		if (brk->onoff == ON && brk->hitCount > 0)
		{
			if ((x > brk->x - brk->width && x <= brk->x + brk->width) && (y > brk->y - brk->height && y <= brk->y + brk->height))
			{
				direction = GetRandomDirection();
				x += 0.03;
				y += 0.04;
				brk->handleCollision();
			}
		}
		// Check collision with paddle
		if ((x > paddle->x - paddle->width / 2 && x < paddle->x + paddle->width / 2) && (y - radius < paddle->y + paddle->height / 2))
		{
			// Reverse the direction
			direction = GetRandomDirection();
		}
		// Check collision with other circles
		for (int i = 0; i < circles.size(); i++)
		{
			if (&circles[i] != this && CheckCircleCollision(this, &circles[i]))
			{
				// Change color of both circles
				ChangeCircleColor();
				circles[i].ChangeCircleColor();
				break;
			}
		}
	}

	bool CheckCircleCollision(Circle* circle1, Circle* circle2)
	{
		float dx = circle1->x - circle2->x;
		float dy = circle1->y - circle2->y;
		float distance = sqrt(dx * dx + dy * dy);
		return distance <= circle1->radius + circle2->radius;
	}

	void ChangeCircleColor()
	{
		// Change the color of the circle upon collision
		red = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		green = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		blue = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	}

	int GetRandomDirection()
	{
		return (rand() % 8) + 1;
	}

	void MoveOneStep()
	{
		if (direction == 1 || direction == 5 || direction == 6)  // up
		{
			if (y > -1 + radius)
			{
				y -= speed;
			}
			else
			{
				// Reverse the direction
				direction = (direction + 2) % 8;
				y = -1 + radius;
			}
		}

		if (direction == 2 || direction == 5 || direction == 7)  // right
		{
			if (x < 1 - radius)
			{
				x += speed;
			}
			else
			{
				// Reverse the direction
				direction = (direction - 1) % 8;
				x = 1 - radius;
			}
		}

		if (direction == 3 || direction == 7 || direction == 8)  // down
		{
			if (y < 1 - radius) {
				y += speed;
			}
			else
			{
				// Reverse the direction
				direction = (direction + 6) % 8;
				y = 1 - radius;
			}
		}

		if (direction == 4 || direction == 6 || direction == 8)  // left
		{
			if (x > -1 + radius) {
				x -= speed;
			}
			else
			{
				// Reverse the direction
				direction = (direction + 1) % 8;
				x = -1 + radius;
			}
		}
	}

	void DrawCircle()
	{
		glColor3f(red, green, blue);
		glBegin(GL_POLYGON);
		for (int i = 0; i < 360; i++) {
			float degInRad = i * DEG2RAD;
			glVertex2f((cos(degInRad) * radius) + x, (sin(degInRad) * radius) + y);
		}
		glEnd();
	}
};

void AddBricks(vector<Brick>& bricks)
{
	float startX = -0.9f;          // Starting X position of the first brick
	float startY = 0.8f;           // Starting Y position of the first brick
	float brickWidth = 0.1f;       // Width of each brick
	float brickHeight = 0.05f;     // Height of each brick
	float brickSpacingX = 0.05f;   // Horizontal spacing between bricks
	float brickSpacingY = 0.07f;   // Vertical spacing between bricks
	float red = 1.0f;
	float green = 0.0f;
	float blue = 0.0f;

	int rows = 6;                  // Number of rows of bricks
	int columns = 10;              // Number of columns of bricks

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			float x = startX + j * (brickWidth + brickSpacingX);
			float y = startY - i * (brickHeight + brickSpacingY);
			BRICKTYPE brickType = (i % 2 == 0) ? REFLECTIVE : DESTRUCTABLE;
			bricks.push_back(Brick(brickType, x, y, brickWidth, brickHeight, red, green, blue));
			red -= 0.1f;
			green += 0.1f;
			blue += 0.1f;
		}
	}
}


vector<Brick> bricks;
vector<Circle> circles;
Paddle paddle(0.0f, -0.9f, 0.2f, 0.03f, 1.0f, 1.0f, 1.0f);

int main(void) {
	srand(time(NULL));

	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	GLFWwindow* window = glfwCreateWindow(480, 480, "Brick Breaker Game", NULL, NULL);

	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	AddBricks(bricks);

	while (!glfwWindowShouldClose(window)) {
		glViewport(0, 0, 480, 480);
		glClear(GL_COLOR_BUFFER_BIT);

		processInput(window);

		for (int i = 0; i < circles.size(); i++)
		{
			for (int j = 0; j < bricks.size(); j++)
			{
				circles[i].CheckCollision(&bricks[j], &paddle, circles);
			}

			circles[i].MoveOneStep();
			circles[i].DrawCircle();
		}

		for (int i = 0; i < bricks.size(); i++)
		{
			bricks[i].drawBrick();
		}

		paddle.drawPaddle();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		paddle.moveLeft();

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		paddle.moveRight();

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		double r, g, b;
		r = rand() / 10000;
		g = rand() / 10000;
		b = rand() / 10000;
		Circle B(0, 0, 02, 2, 0.05, r, g, b);
		circles.push_back(B);
	}
}