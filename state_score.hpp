//  clang++ -std=c++17 -lm -O3 -march=native -o run  rcps.cpp
//  (on Apple M1) clang++ -std=c++17 -lm -O3 -o run  rcps.cpp
#include <array>
#include <cmath>
#include <map>
#include <math.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

/* Returns an angle between 0 and 65535 inclusive by using mods. */
int normalize(int angle) { return (((angle % 65536) + 65536) % 65536); }

// Will be used to obtain a seed for the random number engine
std::random_device rd;
// Standard mersenne_twister_engine seeded with rd()
std::mt19937 gen(rd());

/* Intializes pseudorandom number generator */
// void initialize_rand() { srand((unsigned)time(NULL)); }

/* Generates integer between two values, inclusive*/
int randbetween(int a, int b) {
  std::uniform_int_distribution<> distrib(a, b);
  return distrib(gen);
  // return ((b - a + 1) * (((double)rand()) / RAND_MAX) + a);
}

double randbetween(double a, double b) {
  std::uniform_real_distribution<> distrib(a, b);
  return distrib(gen);
  // return ((b - a + 1) * (((double)rand()) / RAND_MAX) + a);
}

template <int a, int b> int randbetween() {
  if constexpr ((a == 0) & (b == 1)) {
    return gen() & 1;
  }
  if constexpr ((a == 0) & (b == 3)) {
    return gen() & 3;
  }
  if constexpr ((a == 0) & (b == 7)) {
    return gen() & 7;
  }
  if constexpr ((a == 0) & (b == 15)) {
    return gen() & 15;
  }
  // return a + (gen() % ((b - a) + 1));
  static std::uniform_int_distribution<> distrib(a, b);
  return distrib(gen);
}

/* Returns a new number that is the current number moved towards the target
number by at most max displacement. */
int moveNumberTowards(int currentNumber, int targetNumber,
                      int maxDisplacement) {
  if (currentNumber == targetNumber) { // exactly equal to target
    return currentNumber;
  } else if (currentNumber < targetNumber) { // lower than target
    int diff = targetNumber - currentNumber;
    int newNumber = currentNumber + std::min(diff, maxDisplacement);
    return newNumber;
  } else { // higher than target
    int diff = currentNumber - targetNumber;
    int newNumber = currentNumber - std::min(diff, maxDisplacement);
    return newNumber;
  }
}

/* Returns a new angle that is the current angle moved towards the target angle
in the closer direction by at most max displacement. Normalization is included.
*/
int moveAngleTowards(int currentAngle, int targetAngle, int maxDisplacement) {
  if (currentAngle == targetAngle) {
    return currentAngle;
  } else {
    int diff = targetAngle - currentAngle;
    diff = (diff + 65536) % 65536;
    int newAngle;
    if (diff < 32768) { // target is slightly above current
      newAngle = currentAngle + std::min(diff, maxDisplacement);
    } else { // target is slightly below current
      newAngle = currentAngle - std::min(65536 - diff, maxDisplacement);
    }
    return normalize(newAngle);
  }
}

constexpr int num_seeds = 65114;

/* iterates rng instead of cycling it through a list */
constexpr unsigned short rng_function(unsigned short input) {
  if (input == 0x560A)
    input = 0;
  unsigned short S0 = (unsigned char)input << 8;
  S0 = S0 ^ input;
  input = ((S0 & 0xFF) << 8) | ((S0 & 0xFF00) >> 8);
  S0 = ((unsigned char)S0 << 1) ^ input;
  short S1 = (S0 >> 1) ^ 0xFF80;
  if ((S0 & 1) == 0) {
    if (S1 == (short)0xAA55)
      input = 0;
    else
      input = S1 ^ 0x1FF4;
  } else
    input = S1 ^ 0x8180;
  return (unsigned short)input;
}
constexpr std::array<unsigned short, 1U << 16> fill_rng_function_table() {
  std::array<unsigned short, 1U << 16> table = {};
  for (unsigned int i = 0; i < (1U << 16); i++) {
    table[i] = rng_function(i);
  }
  return table;
}

constexpr std::array<unsigned short, 1U << 16> rng_function_table =
    fill_rng_function_table();

constexpr std::array<unsigned short, 1U << 16>
fill_backwords_rng_function_table() {
  std::array<unsigned short, 1U << 16> table = {};
  int seed = 0;
  for (unsigned int i = 0; i < num_seeds; i++) {
    table[rng_function(seed)] = seed;
    seed = rng_function(seed);
  }
  return table;
}

constexpr std::array<unsigned short, 1U << 16> backwords_rng_function_table =
    fill_backwords_rng_function_table();

/* calls and updates the rng value */
int pollRNG(unsigned short *rngValue) {
  // *rngValue = rng_function(*rngValue);
  // if (rng_function(*rngValue) != rng_function_table[*rngValue]) {
  //   printf("something is wrong\n");
  // }
  *rngValue = rng_function_table[*rngValue];
  return (int)*rngValue;
}

/* A bob-omb is the black bomb enemy. There are two of them in TTC,
near the start of the course. A bob-omb calls RNG every frame to determine
whether it should blink its eyes. If it does blink, then it blinks
for 16 frames, during which it won't call RNG.
A bob-omb will only update when Mario is within 4000 units of the bob-omb.
Note that this is not synonymous with whether the bob-omb is visible or not,
which is dictated by a radius smaller than 4000. In other words,
for certain distances (like 3500), the bob-omb will not be visible, but will
still update and call RNG just as normal. */

typedef struct bobomb_t {
  // how deep into the blink the bob-omb is
  // this variable is 0 when the bob-omb is not blinking
  uint8_t blinkingTimer;
  auto operator<=>(const bobomb_t &) const = default;
} bobomb_t;

void bobomb(bobomb_t *b, unsigned short *rngValue) {
  // how deep into the blink the bob-omb is
  // this variable is 0 when the bob-omb is not blinking
  if (b->blinkingTimer > 0) { // currently blinking
    b->blinkingTimer = (b->blinkingTimer + 1) % 16;
  } else if (pollRNG(rngValue) <=
             655) { // the else already means it's not currently blinking
    b->blinkingTimer++;
  }
}

/* A cog is the hexagon platform that spins about a vertical axis
(i.e. changes its yaw). A cog has a target angular velocity and a
current angular velocity. On every frame, the current angular velocity
moves 50 towards the target angular velocity. Then the cog's angle changes
by the current angular velocity. When the current angular velocity reaches
the target angular velocity, a new target angular velocity is calculated. */

typedef struct cog_t {
  int16_t currentAngularVelocity; // = -1200, -1150, ... , 1150, 1200
  int16_t targetAngularVelocity;  // = -1200, -1000, ... , 1000, 1200
  int16_t last_target = 0;
  bool small_enough_movement_so_far = true;
  auto operator<=>(const cog_t &) const = default;
} cog_t;

void cog(cog_t *c, unsigned short *rngValue) {
  if (c->currentAngularVelocity > c->targetAngularVelocity) {
    c->currentAngularVelocity -= 50;
  } else if (c->currentAngularVelocity < c->targetAngularVelocity) {
    c->currentAngularVelocity += 50;
  }
  if (c->currentAngularVelocity == c->targetAngularVelocity) {
    int magnitude =
        (pollRNG(rngValue) % 7) * 200; // = 0, 200, 400, 600, 800, 1000, 1200
    int sign = (pollRNG(rngValue) <= 32766) ? -1 : 1; // = -1, 1
    c->targetAngularVelocity =
        magnitude * sign; // = -1200, -1000, ... , 1000, 1200
  }
}

void rcpscog(cog_t *c, unsigned short *rngValue) {
  if (c->currentAngularVelocity > c->targetAngularVelocity) {
    c->currentAngularVelocity -= 50;
  } else if (c->currentAngularVelocity < c->targetAngularVelocity) {
    c->currentAngularVelocity += 50;
  }
  if (c->currentAngularVelocity == c->targetAngularVelocity) {
    int magnitude =
        (pollRNG(rngValue) % 7) * 200; // = 0, 200, 400, 600, 800, 1000, 1200
    if (magnitude > 200) {
      c->small_enough_movement_so_far = 0;
      c->last_target = 0;
      return;
    }
    int sign = (pollRNG(rngValue) <= 32766) ? -1 : 1; // = -1, 1
    c->last_target = c->targetAngularVelocity;
    c->targetAngularVelocity =
        magnitude * sign; // = -1200, -1000, ... , 1000, 1200
    if (c->last_target != 0 && magnitude != 0 &&
        c->last_target != -1 * c->targetAngularVelocity) {
      c->last_target = 0;
      c->small_enough_movement_so_far = 0;
    }
  }
}

/* An elevator is the yellow rectangle platform that moves up and down and will
periodically stops and switches directions. An elevator moves up or down and
will switch directions when it reaches its min height or max height.
In addition, when the counter variable exceeds the max variable, the elevator
will call RNG to determine whether its new direction (up or down) and how long
until the next possible direction switch. */

typedef struct elevator_t {
  // int max; // = 30, 60, 90, 120, 150, 180
  // starts at 30, 60, 90, 120, 150, 180 and goes down to 0
  uint8_t counter;
  auto operator<=>(const elevator_t &) const = default;

} elevator_t;

// void elevator(elevator_t *e, unsigned short *rngValue) {
//   if (e->counter > e->max) {
//     pollRNG(rngValue);                            // direction call
//     e->max = ((pollRNG(rngValue) % 6) * 30) + 30; // = 30, 60, 90, 120, 150,
//     180
//    e->counter = 0;
//   }
//   e->counter++;
// }
void elevator(elevator_t *e, unsigned short *rngValue) {
  if (e->counter == 0) {
    pollRNG(rngValue); // direction call
    e->counter =
        ((pollRNG(rngValue) % 6) * 30) + 30; // = 30, 60, 90, 120, 150, 180
  }
  e->counter--;
}

/* A hand is the long horizontal clock hand that rotates in a circle and that
Mario is supposed to ride on to get to the other side of the course. A hand has
a displacement (i.e. how much it should tick, which will be positive for CCW
ticks and negative for CW ticks). When timer > max, the hand ticks and
calculates a new max using RNG. Furthermore, if at this time the direction timer
has decremented to zero, then the hand will calculate a new displacement
(i.e. whether it should tick CW or CCW) as well as how long it should be until
it has the chance to change direction again (i.e. what value its direction
timer will be). */

typedef struct hand_t {
  int angle;
  int targetAngle;
  int16_t max;
  int16_t displacement;
  int16_t directionTimer;
  int16_t timer;
  auto operator<=>(const hand_t &) const = default;
} hand_t;

void hand(hand_t *h, unsigned short *rngValue) {
  if (h->max == 0) { // course just started
    h->max = 10;
    h->displacement = -1092;
  }
  h->angle = moveAngleTowards(h->angle, h->targetAngle, 200);
  h->directionTimer = std::max(0, h->directionTimer - 1);
  if (h->timer > h->max &&
      h->angle == h->targetAngle) { // done waiting and reached target
    h->targetAngle = h->targetAngle + h->displacement;
    h->targetAngle = normalize(h->targetAngle);
    if (h->directionTimer == 0) { // time to maybe switch directions
      if (pollRNG(rngValue) % 4 == 0) {
        h->displacement = 1092;
        h->directionTimer = ((pollRNG(rngValue) % 3) * 30) + 30; // = 30, 60, 90
      } else {
        h->displacement = -1092;
        h->directionTimer =
            ((pollRNG(rngValue) % 4) * 60) + 90; // = 90, 150, 210, 270
      }
    }
    h->max = ((pollRNG(rngValue) % 3) * 20) + 10; // = 10, 30, 50
    h->timer = 0;
  }
  h->timer++;
}

/* A pendulum is the pendulum that swings back and forth. A pendulum at rest
will call RNG to determine how long it should wait for and how fast it should
accelerate during the next swing. After it's waited the allotted time, it swings
with that acceleration. Once it crosses strictly past the vertical (i.e. angle
0),
the pendulum decelerates by that same acceleration until it comes to a stop. */

typedef struct pendulum_t {
  int angle;
  int angularVelocity;
  int8_t accelerationDirection;
  int8_t accelerationMagnitude;
  int8_t waitingTimer;
  auto operator<=>(const pendulum_t &) const = default;
} pendulum_t;

void pendulum(pendulum_t *p, unsigned short *rngValue) {
  if (p->waitingTimer > 0) { // waiting
    p->waitingTimer--;
  } else {                               // swinging
    if (p->accelerationMagnitude == 0) { // give initital acceleration on start
      p->accelerationMagnitude = 13;
    }
    if (p->angle > 0) {
      p->accelerationDirection = -1;
    } else if (p->angle < 0) {
      p->accelerationDirection = 1;
    }
    p->angularVelocity = p->angularVelocity +
                         p->accelerationDirection * p->accelerationMagnitude;
    p->angle = p->angle + p->angularVelocity;
    if (p->angularVelocity == 0) { // reached peak of swing
      p->accelerationMagnitude =
          (pollRNG(rngValue) % 3 == 0) ? 42 : 13; // = 13, 42
      if (pollRNG(rngValue) % 2 == 0) {           // stop for some time
        p->waitingTimer =
            (int)((pollRNG(rngValue) / 65536.0 * 30) + 5); // = [5,35)
      }
    }
  }
}

/* The pit block is the cube platform that moves up and down near the pit and
the pendulum's star. There is only 1 pit block. The piblock moves up, waits,
moves down, waits, then repeats. It calls RNG when it reaches the top to
determine how long it should wait for. It does not do this at the bottom,
since the time it waits there is always 20 frames. */

typedef struct pitblock_t {
  int16_t height;
  int8_t verticalSpeed;
  int8_t state; // 0 = going up, 1 = going down
  int8_t max;
  int8_t counter;
  auto operator<=>(const pitblock_t &) const = default;
} pitblock_t;

void pitblock(pitblock_t *p, unsigned short *rngValue) {
  if (p->counter > p->max) { // move
    if (p->state == 0) {     // move up
      p->height = std::min(-71, p->height + p->verticalSpeed);
      if (p->height == -71 || p->height == 259) { // reached top
        p->verticalSpeed = -9;
        p->state = 1;
        p->counter = 0;
        p->max =
            ((pollRNG(rngValue) % 6) * 20) + 10; // = 10, 30, 50, 70, 90, 110
      }
    } else { // move down
      p->height = std::max(-71, p->height + p->verticalSpeed);
      if (p->height == -71 || p->height == 259) { // reached bottom
        p->verticalSpeed = 11;
        p->state = 0;
        p->counter = 0;
        p->max = 20;
      }
    }
  }
  p->counter++;
}

/* A pusher is rectangular prism that extends from and retracts into the wall.
They are referred to as "moving bars" in the star Timed Jumps on Moving Bars.
A pusher begins flush with the wall (i.e. its outer surface is flush with the
wall). It waits in this state for some amount of time, dictated by RNG.
Then it retracts deeper into the wall, poised to eject, and waits in this state
for some amount of time, dictated by RNG. Then it extends out, and calls RNG to
determine whether it should extend out all the way out or stop flush with the
wall. If it extends all the way out, then it eventually comes to a stop and then
retracts until it's flush with the wall. */

constexpr std::array<uint8_t, 4> max_index_to_max = {1, 12, 55, 100};

typedef struct pusher_t {
  uint8_t max_index; // (0,1,2,3) -> (1, 12, 55, 100)
  uint8_t countdown; //[0,120)
  uint8_t state;     // 0 = flush with wall, 1 = retracted, 2 = extending, 3 =
                     // retracting
  uint8_t counter;
  auto operator<=>(const pusher_t &) const = default;
} pusher_t;

// returns what it should be at the end if there was no rng calls, else sets max
// to 0
constexpr pusher_t pusher_precalc(pusher_t p) {
  if (p.state == 0) { // flush with wall
    if (p.counter <= max_index_to_max[p.max_index]) {
      p.counter++;
    } else if (p.countdown > 0) {
      p.countdown--;
      p.counter++;
    } else {
      p.max_index = 255;
    }
  } else if (p.state == 1) { // retracted
    if (p.counter < 10) {    // waiting
      p.counter++;
    } else {
      if (p.countdown > 0) { // moving back in
        p.countdown--;
        p.counter++;
      } else { // moving back in
        p.state = 2;
        p.counter = 0;
      }
    }
  } else if (p.state == 2) { // extending
    if (p.counter == 0) {    // wait one frame
      p.counter++;
    } else if (p.counter == 1) { // either extend out or fake it
      p.max_index = 255;
    } else if (p.counter < 36) { // continue extending out
      p.counter++;
    } else { // finished extending out
      p.state = 3;
      p.counter = 0;
    }
  } else {                // retracting
    if (p.counter < 82) { // still retracting
      p.counter++;
    } else { // finished retracting
      p.state = 0;
      p.counter = 0;
    }
  }
  return p;
}
#if CLANG == 1
constexpr std::array<std::array<std::array<std::array<pusher_t, 220>, 4>, 120>,
                     4>
precalc_pusher_table() {
  std::array<std::array<std::array<std::array<pusher_t, 220>, 4>, 120>, 4>
      table = {};
  for (uint8_t max_index = 0; max_index < 4; max_index++) {
    for (uint8_t countdown = 0; countdown < 120; countdown++) {
      for (uint8_t state = 0; state < 4; state++) {
        for (uint8_t counter = 0; counter < 220; counter++) {
          pusher_t pusher = {max_index, countdown, state, counter};
          table[max_index][countdown][state][counter] = pusher_precalc(pusher);
        }
      }
    }
  }

  return table;
}

constexpr auto pusher_precalc_table = precalc_pusher_table();
#endif

void pusher_full(pusher_t *p, unsigned short *rngValue) {
  if (p->state == 0) { // flush with wall
    if (p->counter <= max_index_to_max[p->max_index]) {
      p->counter++;
    } else if (p->countdown > 0) {
      p->countdown--;
      p->counter++;
    } else {
      p->max_index = pollRNG(rngValue) % 4;
      // countdown = 0 or [20,120)
      if (pollRNG(rngValue) % 2 == 0) {
        p->countdown =
            (int)((pollRNG(rngValue) / 65536.0 * 100) + 20); // = [20,120)
      }
      p->state = 1;
      p->counter = 0;
    }
  } else if (p->state == 1) { // retracted
    if (p->counter < 10) {    // waiting
      p->counter++;
    } else {
      if (p->countdown > 0) { // moving back in
        p->countdown--;
        p->counter++;
      } else { // moving back in
        p->state = 2;
        p->counter = 0;
      }
    }
  } else if (p->state == 2) { // extending
    if (p->counter == 0) {    // wait one frame
      p->counter++;
    } else if (p->counter == 1) {       // either extend out or fake it
      if (pollRNG(rngValue) % 4 == 0) { // fake extend
        p->state = 0;
        p->counter = 0;
      } else { // actually extend
        p->counter++;
      }
    } else if (p->counter < 36) { // continue extending out
      p->counter++;
    } else { // finished extending out
      p->state = 3;
      p->counter = 0;
    }
  } else {                 // retracting
    if (p->counter < 82) { // still retracting
      p->counter++;
    } else { // finished retracting
      p->state = 0;
      p->counter = 0;
    }
  }
}

void pusher(pusher_t *p, unsigned short *rngValue) {
#if CLANG == 1
  pusher_t quick_check =
      pusher_precalc_table[p->max_index][p->countdown][p->state][p->counter];
  if (quick_check.max_index != 255) {
    *p = quick_check;
    return;
  }
#endif
  // pusher_t starting_state = *p;
  pusher_full(p, rngValue);

  // if (quick_check.max != 0) {
  //   if (quick_check.max != p->max || quick_check.countdown != p->countdown ||
  //       quick_check.state != p->state || quick_check.counter != p->counter) {
  //     printf("got {%d, %d, %d, %d}, expected {%d, %d, %d, %d}\n",
  //            quick_check.max, quick_check.countdown, quick_check.state,
  //            quick_check.counter, p->max, p->countdown, p->state,
  //            p->counter);
  //     printf("starting state = {%d, %d, %d, %d}\n", starting_state.max,
  //            starting_state.countdown, starting_state.state,
  //            starting_state.counter);
  //   }
  // }
}

/* Rotating block is the cube that rotates around a horizontal axis
(i.e. changes its pitch). When it completes a rotation, it calls RNG to
determine how long it should wait until the next rotation. Once it has waited
this long, it begins rotating and the process repeats. */

// typedef struct rotatingblock_t {
//   int max;   // = 5, 25, 45, 65, 85, 105, 125
//   int timer; //[0,165]
// } rotatingblock_t;

// void rotatingblock(rotatingblock_t *rb, unsigned short *rngValue) {
//   if (rb->timer >= rb->max + 40) { // done waiting
//     rb->max =
//         ((pollRNG(rngValue) % 7) * 20) + 5; // = 5, 25, 45, 65, 85, 105, 125
//     rb->timer = 0;
//   }
//   rb->timer++;
// }

typedef struct rotatingblock_t {
  uint8_t
      remaining_time; // 45, 65, 85, 105, 125, 145, 165 then counts down to 0
  auto operator<=>(const rotatingblock_t &) const = default;

} rotatingblock_t;

void rotatingblock(rotatingblock_t *rb, unsigned short *rngValue) {
  if (rb->remaining_time == 0) { // done waiting
    rb->remaining_time =
        ((pollRNG(rngValue) % 7) * 20) + 45; // = 45, 65, 85, 105, 125, 145, 165
  }
  rb->remaining_time--;
}

/* Rotating triangular prism is the triangular prism that rotates around a
horizontal axis (i.e. changes its pitch). They look and function like the
rotating blocks. When it completes a rotation, it calls RNG to determine how
long it should wait until the next rotation. Once it has waited this long, it
begins rotating and the process repeats. */

typedef struct rotatingtriangularprism_t {
  // int max;   // = 5, 25, 45, 65, 85, 105, 125
  uint8_t timer; // [0,170]
  auto operator<=>(const rotatingtriangularprism_t &) const = default;
} rotatingtriangularprism_t;

void rotatingtriangularprism(rotatingtriangularprism_t *rtp,
                             unsigned short *rngValue) {
  if (rtp->timer >= 170) { // done waiting
    int max =
        ((pollRNG(rngValue) % 7) * 20) + 5; // = 5, 25, 45, 65, 85, 105, 125
    rtp->timer = 125 - max;
  }
  rtp->timer++;
}

/* A spinner is the rectangle platform that spins about a horizontal axis
(i.e. changes its pitch). They are mostly found near the red coins. When
a spinner completes a spin, it calls RNG to determine which direction it should
rotate next as well as how long the rotation should be. Then for 5 frames,
the spinner remains still. Then for max-5 frames, the spinner spins in its
intended direction. Then for 1 frame, the spinner spins counterclockwise. */

typedef struct spinner_t {
  // int direction; // 1 = CCW, -1 = CW
  // int max;
  uint8_t counter;
  auto operator<=>(const spinner_t &) const = default;
} spinner_t;

void spinner(spinner_t *sp, unsigned short *rngValue) {
  if (sp->counter > 120) {
    // calculate new spin
    // sp->direction = (pollRNG(rngValue) <= 32766) ? -1 : 1; // = -1, 1
    pollRNG(rngValue);                             // for direction
    int max = ((pollRNG(rngValue) % 4) * 30) + 30; // = 30, 60, 90, 120
    sp->counter = 120 - max;
  }
  sp->counter++;
}

/* A spinning triangle is the triangle platform that spins about a vertical axis
(i.e. changes its yaw). It functions exactly as the cog does. */

typedef struct spinningtriangle_t {
  int16_t currentAngularVelocity; // = -1200, -1150, ... , 1150, 1200
  int16_t targetAngularVelocity;  // = -1200, -1000, ... , 1000, 1200
  auto operator<=>(const spinningtriangle_t &) const = default;
} spinningtriangle_t;

void spinningtriangle(spinningtriangle_t *st, unsigned short *rngValue) {
  if (st->currentAngularVelocity > st->targetAngularVelocity) {
    st->currentAngularVelocity -= 50;
  } else if (st->currentAngularVelocity < st->targetAngularVelocity) {
    st->currentAngularVelocity += 50;
  }
  if (st->currentAngularVelocity == st->targetAngularVelocity) {
    int magnitude =
        (pollRNG(rngValue) % 7) * 200; // = 0, 200, 400, 600, 800, 1000, 1200
    int sign = (pollRNG(rngValue) <= 32766) ? -1 : 1; // = -1, 1
    st->targetAngularVelocity =
        magnitude * sign; // = -1200, -1000, ... , 1000, 1200
  }
}

/* A thwomp is the blue cube-like enemy that moves up and down in an attempt to
squish Mario. There is only one Thwomp in TTC, near the very top of the clock.
A thwomp moves up, waits, moves down, waits, then repeats.
It calls RNG when it reaches the top to determine how long it should wait for,
and it also does this at the bottom. */

typedef struct thwomp_t {
  int16_t height;
  uint8_t verticalSpeed;
  int8_t max;   // [10,40) or [20,30)
  int8_t state; // 0 = going up, 1 = at top, 2 = going down, 3/4 = at bottom
  int8_t counter;
  auto operator<=>(const thwomp_t &) const = default;
} thwomp_t;

void thwomp(thwomp_t *th, unsigned short *rngValue) {
  if (th->state == 0) { // going up
    th->height = std::min(6607, th->height + 10);
    th->counter++;
    if (th->height == 6607) { // reached top
      th->state = 1;
      th->counter = 0;
    }
  } else if (th->state == 1) { // at top
    if (th->counter == 0) {    // just reached top
      th->max = (int)((pollRNG(rngValue) / 65536.0 * 30) + 10); // = [10,40)
    }
    if (th->counter <= th->max) { // waiting
      th->counter++;
    } else { // done waiting
      th->state = 2;
      th->counter = 0;
    }
  } else if (th->state == 2) { // going down
    th->verticalSpeed -= 4;
    th->height = std::max(6192, th->height + th->verticalSpeed);
    th->counter++;
    if (th->height == 6192) { // reached bottom
      th->verticalSpeed = 0;
      th->state = 3;
      th->counter = 0;
    }
  } else if (th->state == 3) { // at bottom (1/2)
    if (th->counter < 10) {    // waiting
      th->counter++;
    } else { // done waiting
      th->state = 4;
      th->counter = 0;
    }
  } else {                  // at bottom (2/2)
    if (th->counter == 0) { // just reached bottom
      th->max = (int)(pollRNG(rngValue) / 65536.0 * 10 + 20); // = [20,30)
    }
    if (th->counter <= th->max) { // waiting
      th->counter++;
    } else { // done waiting
      th->state = 0;
      th->counter = 0;
    }
  }
}

/* A treadmill is the yellow treadmill that periodically reverses directions.
Although there are 7 treadmills in TTC, only the first one actually updates
meaningfully using RNG, and the others simply copy what this first one is doing.
When a treadmill comes to a stop, it calls RNG to determine whether it should
move forwards or backwards and also how long it should move in that direction.
Then the treadmill accelerates to 50 speed in that direction. Once it's moved
in that direction for the allotted time, it decelerates to a stop, and the
process repeats. */

typedef struct treadmill_t {
  int8_t currentSpeed;
  int8_t targetSpeed;
  uint8_t max;
  uint8_t counter;
  auto operator<=>(const treadmill_t &) const = default;
} treadmill_t;

void treadmill(treadmill_t *tr, unsigned short *rngValue) {
  if (tr->counter <= tr->max) { // still/accelerate/move
    if (tr->counter > 5) {      // accelerate/move
      tr->currentSpeed =
          moveNumberTowards(tr->currentSpeed, tr->targetSpeed, 10);
    }
  } else { // slow down
    tr->currentSpeed = moveNumberTowards(tr->currentSpeed, 0, 10);
    if (tr->currentSpeed == 0) { // came to a stop
      tr->max =
          ((pollRNG(rngValue) % 7) * 20) + 10; // = 10, 30, 50, 70, 90, 110, 130
      tr->targetSpeed = (pollRNG(rngValue) <= 32766) ? -50 : 50; // = -50, 50
      tr->counter = 0;
    }
  }
  tr->counter++;
}

/* A wheel is the little cog-like structure that is found slightly under the
floor in various parts of TTC. They appear in pairs. They function exactly as
the hands do, except that their ticks are greater in magnitude. */

typedef struct wheel_t {
  int angle;
  int8_t max; // = 10, 30, 50
  int targetAngle;
  int displacement;
  int16_t directionTimer; // = 30, 60, 90 or = 90, 150, 210, 270
  int8_t timer;
  auto operator<=>(const wheel_t &) const = default;
} wheel_t;

void wheel(wheel_t *w, unsigned short *rngValue) {
  if (w->max == 0) { // course just started
    w->max = 5;
    w->displacement = -3276;
  }
  w->angle = moveAngleTowards(w->angle, w->targetAngle, 200);
  w->directionTimer = std::max(0, w->directionTimer - 1);
  if (w->timer <= w->max) { // waiting
    w->timer++;
  } else if (w->angle == w->targetAngle) { // done waiting and reached target
    w->targetAngle = w->targetAngle + w->displacement;
    w->targetAngle = normalize(w->targetAngle);
    if (w->directionTimer == 0) {       // time to maybe switch directions
      if (pollRNG(rngValue) % 4 == 0) { // time to move CCW
        w->displacement = 3276;
        w->directionTimer = ((pollRNG(rngValue) % 3) * 30) + 30; // = 30, 60, 90
      } else { // time to move CW
        w->displacement = -3276;
        w->directionTimer =
            (pollRNG(rngValue) % 4) * 60 + 90; // = 90, 150, 210, 270
      }
    }
    w->max = ((pollRNG(rngValue) % 3) * 20) + 10; // = 10, 30, 50
    w->timer = 0;
    w->timer++;
  } else { // timer high enough, but not at target angle (will only happen at
           // level start)
    w->timer++;
  }
}

// got initial state from pannen, update if nessesary
using objects_t = struct objects_t {
  rotatingblock_t rotating_blocks[6] = {{40 + 125 - 31}, {40 + 5 - 16},
                                        {40 + 25 - 6},   {40 + 45 - 51},
                                        {40 + 65 - 71},  {40 + 25 - 61}};
  rotatingtriangularprism_t rotatingtriangularprisms[2] = {{125 - (125 - 126)},
                                                           {125 - (125 - 26)}};
  pendulum_t pendulums[4] = {{-7155, 26, 1, 13, 0},
                             {-1993, 390, 1, 13, 0},
                             {5822, 130, -1, 13, 0},
                             {-9159, 84, 1, 42, 0}};
  treadmill_t treadmill = {0, -50, 30, 5};
  pusher_t pushers[12] = {{3, 40, 1, 39}, {2, 0, 3, 82}, {1, 49, 1, 42},
                          {2, 0, 3, 21},  {3, 0, 3, 5},  {0, 0, 2, 7},
                          {3, 0, 0, 87},  {2, 0, 3, 5},  {2, 0, 0, 51},
                          {0, 0, 3, 80},  {0, 0, 3, 63}, {1, 0, 3, 6}};
  cog_t rcpscog = {150, -200};
  cog_t cogs[4] = {{600, 800}, {-350, -600}, {-300, 1200}, {900, 1000}};
  spinningtriangle_t spinningtriangles[2] = {{150, 0}, {-950, -1000}};
  pitblock_t pitblock = {259, -9, 1, 110, 110};
  hand_t hands[2] = {{33704, 33704, 50, -1092, 173, 36},
                     {5344, 5344, 50, -1092, 168, 32}};
  spinner_t spinners[14] = {
      {120 - (30 - 14)},   {120 - (30 - 24)},  {120 - (30 - 26)},
      {120 - (120 - 116)}, {120 - (90 - 72)},  {120 - (120 - 6)},
      {120 - (90 - 81)},   {120 - (60 - 41)},  {120 - (120 - 115)},
      {120 - (90 - 61)},   {120 - (120 - 13)}, {120 - (90 - 65)},
      {120 - (60 - 31)},   {120 - (120 - 37)}};
  wheel_t wheels[6] = {
      {42016, 50, 42016, 3276, 0, 42},   {12580, 50, 12580, -3276, 81, 45},
      {35416, 50, 35416, -3276, 97, 32}, {48616, 50, 48616, 3276, 49, 42},
      {58744, 30, 58268, -3276, 23, 15}, {40704, 30, 38628, -3276, 84, 7}};
  elevator_t elevators[2] = {{180 - 120}, {150 - 106}};
  cog_t sixthcog = {50, -800};
  thwomp_t thwomp = {6482, 0, 23, 0, 29};
  bobomb_t bobombs[2] = {{0}, {0}};
  unsigned short rngValue = 43517;
  auto operator<=>(const objects_t &) const = default;
};

/* moves objects forward one frame */
void advanceobjects(objects_t *objects) {
  int i;
  for (i = 0; i < 6; i++) {
    rotatingblock(&objects->rotating_blocks[i], &objects->rngValue);
  }
  for (i = 0; i < 2; i++) {
    rotatingtriangularprism(&objects->rotatingtriangularprisms[i],
                            &objects->rngValue);
  }
  for (i = 0; i < 4; i++) {
    pendulum(&objects->pendulums[i], &objects->rngValue);
  }
  treadmill(&objects->treadmill, &objects->rngValue);
  for (i = 0; i < 12; i++) {
    pusher(&objects->pushers[i], &objects->rngValue);
  }
  rcpscog(&objects->rcpscog, &objects->rngValue);
  if (objects->rcpscog.small_enough_movement_so_far == 0) {
    return;
  }
  for (i = 0; i < 4; i++) {
    cog(&objects->cogs[i], &objects->rngValue);
  }
  for (i = 0; i < 2; i++) {
    spinningtriangle(&objects->spinningtriangles[i], &objects->rngValue);
  }
  pitblock(&objects->pitblock, &objects->rngValue);
  for (i = 0; i < 2; i++) {
    hand(&objects->hands[i], &objects->rngValue);
  }
  for (i = 0; i < 14; i++) {
    spinner(&objects->spinners[i], &objects->rngValue);
  }
  for (i = 0; i < 6; i++) {
    wheel(&objects->wheels[i], &objects->rngValue);
  }
  for (i = 0; i < 2; i++) {
    elevator(&objects->elevators[i], &objects->rngValue);
  }
  cog(&objects->sixthcog, &objects->rngValue);
  thwomp(&objects->thwomp, &objects->rngValue);
  for (i = 0; i < 2; i++) {
    bobomb(&objects->bobombs[i], &objects->rngValue);
  }
}

void randomizearray(objects_t *inputstate) {
  int a;
  for (a = 0; a < 6; a++) {
    // inputstate->rotating_blocks[a].max = randbetween<0, 6>() * 20 + 5;
    // inputstate->rotating_blocks[a].timer =
    //     randbetween(0, ((inputstate->rotating_blocks[a].max + 40) / 5) * 5);
    inputstate->rotating_blocks[a].remaining_time = randbetween<0, 165>();
  }
  for (a = 0; a < 2; a++) {
    // inputstate->rotatingtriangularprisms[a].max = randbetween<0, 6>() * 20 +
    // 5;
    inputstate->rotatingtriangularprisms[a].timer = randbetween(0, 170);
  }
  for (a = 0; a < 4; a++) {
    inputstate->pendulums[a].waitingTimer = randbetween<0, 34>();
    inputstate->pendulums[a].accelerationDirection =
        randbetween<0, 1>() * 2 - 1;
    inputstate->pendulums[a].angle = randbetween<0, 1000>() * 13 - 6500;
    inputstate->pendulums[a].angularVelocity = randbetween<1, 5>() * 21 - 63;
    inputstate->pendulums[a].accelerationMagnitude =
        randbetween<0, 1>() * 29 + 13;
  }
  inputstate->treadmill.currentSpeed = randbetween<-5, 5>() * 10;
  inputstate->treadmill.targetSpeed = randbetween<0, 1>() * 100 - 50;
  inputstate->treadmill.max = randbetween<0, 6>() * 20 + 10;
  inputstate->treadmill.counter =
      randbetween(0, ((inputstate->treadmill.max) / 5) * 5);
  for (a = 0; a < 12; a++) {
    inputstate->pushers[a].max_index = randbetween<0, 3>();
    inputstate->pushers[a].countdown = randbetween<0, 119>();
    inputstate->pushers[a].state = randbetween<0, 3>();
    inputstate->pushers[a].counter = randbetween(
        0, ((max_index_to_max[inputstate->pushers[a].max_index]) / 5) * 5);
  }
  inputstate->rcpscog.currentAngularVelocity = 0;
  inputstate->rcpscog.targetAngularVelocity = 0;
  for (a = 0; a < 4; a++) {
    inputstate->cogs[a].currentAngularVelocity = randbetween<-24, 24>() * 50;
    inputstate->cogs[a].targetAngularVelocity = randbetween<-6, 6>() * 20;
  }
  for (a = 0; a < 2; a++) {
    inputstate->spinningtriangles[a].currentAngularVelocity =
        randbetween<-24, 24>() * 50;
    inputstate->spinningtriangles[a].targetAngularVelocity =
        randbetween<-6, 6>() * 20;
  }
  inputstate->pitblock.height = randbetween<0, 30>() * 11 - 71;
  inputstate->pitblock.verticalSpeed = randbetween<0, 1>() * 20 - 9;
  inputstate->pitblock.state = randbetween<0, 1>();
  inputstate->pitblock.max = (randbetween<0, 6>() * 20 + 9) % 110 + 1;
  inputstate->pitblock.counter = randbetween(0, inputstate->pitblock.max);
  for (a = 0; a < 2; a++) {
    inputstate->hands[a].angle = randbetween<-6, 6>() * 182;
    inputstate->hands[a].max = randbetween<0, 2>() * 20 + 10;
    inputstate->hands[a].targetAngle = randbetween<-1, 1>() * 1092;
    inputstate->hands[a].displacement = (randbetween<0, 1>() * 2 - 1) * 1092;
    inputstate->hands[a].directionTimer =
        (randbetween<0, 5>() * 60 + 29) % 270 + 1;
    inputstate->hands[a].timer = randbetween(0, inputstate->hands[a].max);
  }
  for (a = 0; a < 14; a++) {
    // inputstate->spinners[a].direction = randbetween<0, 1>() * 2 - 1;
    // inputstate->spinners[a].max = randbetween<0, 3>() * 30 + 30;
    inputstate->spinners[a].counter = randbetween(0, 120);
  }
  for (a = 0; a < 6; a++) {
    inputstate->wheels[a].max = randbetween<0, 2>() * 20 + 10;
    inputstate->wheels[a].angle = randbetween<-14, 14>() * 234;
    inputstate->wheels[a].targetAngle = randbetween<-1, 1>() * 3276;
    inputstate->wheels[a].displacement = (randbetween<0, 1>() * 2 - 1) * 3276;
    inputstate->wheels[a].directionTimer =
        (randbetween<0, 5>() * 60 + 29) % 270 + 1;
    inputstate->wheels[a].timer = randbetween(0, inputstate->wheels[a].max);
  }
  for (a = 0; a < 2; a++) {
    // inputstate->elevators[a].max = randbetween<1, 6>() * 30;
    inputstate->elevators[a].counter = randbetween(0, 180);
  }
  inputstate->sixthcog.currentAngularVelocity = randbetween<-24, 24>() * 50;
  inputstate->sixthcog.targetAngularVelocity = randbetween<-6, 6>() * 20;
  inputstate->thwomp.height = randbetween<6192, 6607>();
  inputstate->thwomp.max = randbetween<10, 39>();
  inputstate->thwomp.counter = randbetween(0, inputstate->thwomp.max);
  inputstate->thwomp.state = randbetween<0, 4>();
  inputstate->thwomp.verticalSpeed = randbetween<0, 104>() * -4;
  for (a = 0; a < 2; a++) {
    inputstate->bobombs[a].blinkingTimer = randbetween<0, 15>();
  }
}

void printobjectstates(const objects_t *const inputstate) {
  int a;
  for (a = 0; a < 6; a++) {
    // printf("Rotating Block %i max: %i\n", a + 1,
    //        inputstate->rotating_blocks[a].max);
    // printf("Rotating Block %i timer: %i\n", a + 1,
    //        inputstate->rotating_blocks[a].timer);
    printf("Rotating Block %i remaining time: %i\n", a + 1,
           inputstate->rotating_blocks[a].remaining_time);
  }
  for (a = 0; a < 2; a++) {
    // printf("Rotating Triangular Prism %i max: %i\n", a + 1,
    //        inputstate->rotatingtriangularprisms[a].max);
    printf("Rotating Triangular Prism %i timer: %i\n", a + 1,
           inputstate->rotatingtriangularprisms[a].timer);
  }
  for (a = 0; a < 4; a++) {
    printf("Pendulum %i waiting timer: %i, ", a + 1,
           inputstate->pendulums[a].waitingTimer);
    printf("Pendulum %i acceleration direction: %i, ", a + 1,
           inputstate->pendulums[a].accelerationDirection);
    printf("Pendulum %i angle: %i, ", a + 1, inputstate->pendulums[a].angle);
    printf("Pendulum %i angular velocity: %i, ", a + 1,
           inputstate->pendulums[a].angularVelocity);
    printf("Pendulum %i acceleration magnitude: %i\n", a + 1,
           inputstate->pendulums[a].accelerationMagnitude);
  }
  printf("Treadmill current speed: %i, ", inputstate->treadmill.currentSpeed);
  printf("Treadmill target speed: %i, ", inputstate->treadmill.targetSpeed);
  printf("Treadmill max: %i, ", inputstate->treadmill.max);
  printf("Treadmill counter: %i\n", inputstate->treadmill.counter);
  for (a = 0; a < 12; a++) {
    printf("Pusher %i max: %i, ", a + 1,
           max_index_to_max[inputstate->pushers[a].max_index]);
    printf("Pusher %i countdown: %i, ", a + 1,
           inputstate->pushers[a].countdown);
    printf("Pusher %i state: %i, ", a + 1, inputstate->pushers[a].state);
    printf("Pusher %i counter: %i\n", a + 1, inputstate->pushers[a].counter);
  }
  printf("Cog current angular velocity: %i, ",
         inputstate->rcpscog.currentAngularVelocity);
  printf("Cog target anvular velocity: %i \n",
         inputstate->rcpscog.targetAngularVelocity);
  for (a = 0; a < 4; a++) {
    printf("Cogs %i currentAngularVelocity: %i, ", a + 1,
           inputstate->cogs[a].currentAngularVelocity);
    printf("Cogs %i targetAngularVelocity: %i \n", a + 1,
           inputstate->cogs[a].targetAngularVelocity);
  }
  for (a = 0; a < 2; a++) {
    printf("Spinningtriangles %i currentAngularVelocity: %i, ", a + 1,
           inputstate->spinningtriangles[a].currentAngularVelocity);
    printf("Spinningtriangles %i targetAngularVelocity: %i \n", a + 1,
           inputstate->spinningtriangles[a].targetAngularVelocity);
  }
  printf("Pit Block height: %i, ", inputstate->pitblock.height);
  printf("Pit Block vertical speed: %i, ", inputstate->pitblock.verticalSpeed);
  printf("Pit Block state: %i, ", inputstate->pitblock.state);
  printf("Pit Block max: %i, ", inputstate->pitblock.max);
  printf("Pit Block counter: %i \n", inputstate->pitblock.counter);
  for (a = 0; a < 2; a++) {
    printf("Hand %i angle: %i, ", a + 1, inputstate->hands[a].angle);
    printf("Hand %i max: %i, ", a + 1, inputstate->hands[a].max);
    printf("Hand %i target angle: %i, ", a + 1,
           inputstate->hands[a].targetAngle);
    printf("Hand %i displacement: %i, ", a + 1,
           inputstate->hands[a].displacement);
    printf("Hand %i direction timer: %i, ", a + 1,
           inputstate->hands[a].directionTimer);
    printf("Hand %i timer: %i \n", a + 1, inputstate->hands[a].timer);
  }
  for (a = 0; a < 14; a++) {
    // printf("Spinner %i direction: %i \n", a + 1,
    //        inputstate->spinners[a].direction);
    // printf("Spinner %i max: %i \n", a + 1, inputstate->spinners[a].max);
    printf("Spinner %i counter: %i \n", a + 1, inputstate->spinners[a].counter);
  }
  for (a = 0; a < 6; a++) {
    printf("Wheel %i max: %i, ", a + 1, inputstate->wheels[a].max);
    printf("Wheel %i angle: %i, ", a + 1, inputstate->wheels[a].angle);
    printf("Wheel %i target angle: %i, ", a + 1,
           inputstate->wheels[a].targetAngle);
    printf("Wheel %i displacement: %i, ", a + 1,
           inputstate->wheels[a].displacement);
    printf("Wheel %i direction timer: %i, ", a + 1,
           inputstate->wheels[a].directionTimer);
    printf("Wheel %i timer: %i \n", a + 1, inputstate->wheels[a].timer);
  }
  for (a = 0; a < 2; a++) {
    // printf("Elevator %i max: %i \n", a + 1, inputstate->elevators[a].max);
    printf("Elevator %i counter: %i \n", a + 1,
           inputstate->elevators[a].counter);
  }
  printf("Cog current angular velocity: %i, ",
         inputstate->sixthcog.currentAngularVelocity);
  printf("Cog target angular velocity: %i \n",
         inputstate->sixthcog.targetAngularVelocity);
  printf("Thwomp height: %i, ", inputstate->thwomp.height);
  printf("Thwomp max: %i, ", inputstate->thwomp.max);
  printf("Thwomp counter: %i, ", inputstate->thwomp.counter);
  printf("Thwomp state: %i, ", inputstate->thwomp.state);
  printf("Thwomp vertical speed: %i \n", inputstate->thwomp.verticalSpeed);
  for (a = 0; a < 2; a++) {
    printf("Bobomb %i blinking timer: %i \n", a + 1,
           inputstate->bobombs[a].blinkingTimer);
  }
  printf("RNGvalue: %i \n", inputstate->rngValue);
}