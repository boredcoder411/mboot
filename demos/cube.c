#ifdef CUBE_DEMO
float cosf(float x) {
  float result = 1.0f;
  float term = 1.0f;
  for (int n = 1; n < 10; n++) {
    term *= -x * x / (2 * n * (2 * n - 1));
    result += term;
  }
  return result;
}

float sinf(float x) {
  float result = x;
  float term = x;
  for (int n = 1; n < 10; n++) {
    term *= -x * x / (2 * n * (2 * n + 1));
    result += term;
  }
  return result;
}

typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  float x, y;
} Vec2;

#define CUBE_SIZE 40
Vec3 cube_vertices[8] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                         {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};

int cube_edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                         {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

float angle_x = 0.0f;
float angle_y = 0.0f;
float angle_z = 0.0f;

#define CX (SCREEN_WIDTH / 2)
#define CY (SCREEN_HEIGHT / 2)

Vec2 project(Vec3 v) {
  float scale = CUBE_SIZE;
  float dist = 3.0f;
  float z = v.z + dist;
  float px = (v.x / z) * scale + CX;
  float py = (v.y / z) * scale + CY;
  return (Vec2){(int)px, (int)py};
}

void draw_cube() {
  if (angle_x > 6.283185f)
    angle_x -= 6.283185f;
  if (angle_y > 6.283185f)
    angle_y -= 6.283185f;
  if (angle_z > 6.283185f)
    angle_z -= 6.283185f;

  Vec2 projected[8];
  for (int i = 0; i < 8; i++) {
    Vec3 v = cube_vertices[i];

    float y = v.y * cosf(angle_x) - v.z * sinf(angle_x);
    float z = v.y * sinf(angle_x) + v.z * cosf(angle_x);
    v.y = y;
    v.z = z;

    float x = v.x * cosf(angle_y) + v.z * sinf(angle_y);
    z = -v.x * sinf(angle_y) + v.z * cosf(angle_y);
    v.x = x;
    v.z = z;

    x = v.x * cosf(angle_z) - v.y * sinf(angle_z);
    y = v.x * sinf(angle_z) + v.y * cosf(angle_z);
    v.x = x;
    v.y = y;

    projected[i] = project(v);
  }

  for (int i = 0; i < 12; i++) {
    Vec2 p1 = projected[cube_edges[i][0]];
    Vec2 p2 = projected[cube_edges[i][1]];
    draw_line(p1.x, p1.y, p2.x, p2.y, VGA_GREEN);
  }

  angle_x += 0.02f;
  angle_y += 0.03f;
  angle_z += 0.015f;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void pit_handler(registers_t *r) {
  clear_screen();
  draw_cube();
}
#endif
