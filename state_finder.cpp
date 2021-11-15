#include "state_score.hpp"
#include <set>

std::pair<int, int> steps_still_for_state(objects_t *currentstartingarray,
                                          int seed = -1) {
  objects_t states;
  int max_still = 0;
  int seed_for_max_still = 0;
  int back_rng_steps = 5;
  int rng_steps = 10;
  unsigned short local_rng_value = seed;
  if (seed == -1) {
    back_rng_steps = 0;
    rng_steps = num_seeds;
    local_rng_value = 0;
  }

  // printf("told best seed was %d\n", seed);
  for (int i = 0; i < back_rng_steps; i++) {
    local_rng_value = backwords_rng_function_table[local_rng_value];
  }
  for (int i = 0; i < rng_steps; i++) {
    local_rng_value = rng_function_table[local_rng_value];
    memcpy(&states, currentstartingarray, sizeof(objects_t));
    states.rngValue = local_rng_value;
    states.rcpscog.small_enough_movement_so_far = 1;
    int a = 0;
    for (a = 0; a < 1200; a++) {
      advanceobjects(&states);
      if (states.rcpscog.small_enough_movement_so_far == 0) {
        break;
      }
    }

    if (states.rcpscog.small_enough_movement_so_far == 1) {
      printf("cog was still the whole time !!!\n");
      printobjectstates(currentstartingarray);
      exit(0);
      break;
    }
    if (a > max_still) {
      max_still = a;
      seed_for_max_still = local_rng_value;
    }
    // printf("checking rng_value %d, center was %d, score was %d\n",
    //        local_rng_value, seed, a);
  }
  return {max_still, seed_for_max_still};
}

long states_checked = 0;
int most_frames_lasted = 0;
double initial_temperature = 0.125;
int ticker = 10;

std::map<long, long> found_per_length;

void check_small_changes(int best_so_far, objects_t *inputstate,
                         int steps_since_last_increase, int depth = 0,
                         int seed = -1);

void check_state_and_recurse(int best_so_far, objects_t *inputstate,
                             int steps_since_last_increase, int depth,
                             int seed) {
  auto p = steps_still_for_state(inputstate, seed);
  int length = p.first;
  int best_seed = p.second;
  inputstate->rngValue = best_seed;
  states_checked += 1;
  static std::set<objects_t> seen_good_states;
  static uint64_t skiped_states = 0;
  // if (states_checked >= 1000000) {
  //   exit(0);
  // }
  if (length > best_so_far) {
    if (seen_good_states.contains(*inputstate)) {
      // we have already recursed to this state so skip it
      skiped_states += 1;
      return;
    }
    seen_good_states.insert(*inputstate);
    most_frames_lasted = std::max(most_frames_lasted, length);
    if (length >= most_frames_lasted - 10) {
      if (length == most_frames_lasted) {
        printobjectstates(inputstate);
      }
      printf("new best on path = %d, states checked = %ld, seed = %d, "
             "depth = %d, best overall is %d, skiped states = %lu, states "
             "stored = %zu\n",
             length, states_checked, best_seed, depth, most_frames_lasted,
             skiped_states, seen_good_states.size());
    }
    check_small_changes(length, inputstate, 0, depth + 1, best_seed);
    // } else if (false && steps_since_last_increase < 10) {
    //   // Simulated Annealing: A worse point is accepted probabilistically.
    //   // double temperature = initial_temperature / pow(2,length);
    //   double temperature = initial_temperature / (length + 1);
    //   double criterion =
    //       exp((best_so_far - length) /
    //           temperature); // got a seg fault when I used length instead of
    //           depth
    //   double rnd = randbetween(0.0, 1.0); // between 0 and 1
    //   if (ticker > 0 && rnd < criterion) {
    //     ticker--;
    //     check_small_changes(best_so_far, inputstate,
    //                         steps_since_last_increase + 1, depth + 1,
    //                         best_seed);
    //   } else {
    //     ticker++;
    // }
  } else if (length > 120 && // length == best_so_far &&
             steps_since_last_increase < 1) {
    // only allow neutral moves after the first 120 frames
    if (seen_good_states.contains(*inputstate)) {
      // we have already recursed to this state so skip it
      skiped_states += 1;
      return;
    }
    seen_good_states.insert(*inputstate);
    check_small_changes(best_so_far, inputstate, steps_since_last_increase + 1,
                        depth + 1, best_seed);
  }
}

template <class T>
void all_small_changes_to_field(T *field, int start, int end, int mul_factor,
                                int add_factor, int best_so_far,
                                objects_t *inputstate,
                                int steps_since_last_increase, int depth,
                                int seed) {
  for (int i = start; i <= end; i++) {
    int val = i * mul_factor + add_factor;
    if (val == *field) {
      continue;
    }
    auto saved_val = *field;
    *field = val;
    check_state_and_recurse(best_so_far, inputstate, steps_since_last_increase,
                            depth, seed);
    *field = saved_val;
  }
}

void check_small_changes(int best_so_far, objects_t *inputstate,
                         int steps_since_last_increase, int depth, int seed) {
  inputstate->rcpscog.currentAngularVelocity = 0;
  inputstate->rcpscog.targetAngularVelocity = 0;
  int a;
  for (a = 0; a < 6; a++) {
    // inputstate->rotating_blocks[a].max = randbetween<0, 6>() * 20 + 5;
    // all_small_changes_to_field(&inputstate->rotating_blocks[a].timer, 0,
    //                            (inputstate->rotating_blocks[a].max + 40) / 5,
    //                            5, 0, best_so_far, inputstate,
    //                            steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->rotating_blocks[a].remaining_time,
                               0, 165, 1, 0, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
  }
  for (a = 0; a < 2; a++) {
    // inputstate->rotatingtriangularprisms[a].max = randbetween<0, 6>() * 20
    // + 5; inputstate->rotatingtriangularprisms[a].timer =
    //     randbetween(0, inputstate->rotatingtriangularprisms[a].max + 45);
    all_small_changes_to_field(&inputstate->rotatingtriangularprisms[a].timer,
                               0, 170, 1, 0, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
  }
  for (a = 0; a < 4; a++) {
    all_small_changes_to_field(&inputstate->pendulums[a].waitingTimer, 0, 34, 1,
                               0, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->pendulums[a].accelerationDirection,
                               0, 1, 2, -1, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->pendulums[a].angle, 0, 1000, 13,
                               -6500, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->pendulums[a].angularVelocity, 1, 5,
                               21, -63, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->pendulums[a].accelerationMagnitude,
                               0, 1, 29, 13, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
  }
  all_small_changes_to_field(&inputstate->treadmill.currentSpeed, -5, 5, 10, 0,
                             best_so_far, inputstate, steps_since_last_increase,
                             depth, seed);
  all_small_changes_to_field(&inputstate->treadmill.targetSpeed, 0, 1, 100, -50,
                             best_so_far, inputstate, steps_since_last_increase,
                             depth, seed);
  // inputstate->treadmill.max = randbetween<0, 6>() * 20 + 10;
  all_small_changes_to_field(
      &inputstate->treadmill.counter, 0, inputstate->treadmill.max / 5, 5, 0,
      best_so_far, inputstate, steps_since_last_increase, depth, seed);

  for (a = 0; a < 12; a++) {
    inputstate->pushers[a].max_index = randbetween<0, 3>();
    all_small_changes_to_field(&inputstate->pushers[a].countdown, 0, 119, 1, 0,
                               best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->pushers[a].state, 0, 3, 1, 0,
                               best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(
        &inputstate->pushers[a].counter, 0,
        max_index_to_max[inputstate->pushers[a].max_index], 1, 0, best_so_far,
        inputstate, steps_since_last_increase, depth, seed);
  }

  for (a = 0; a < 4; a++) {
    all_small_changes_to_field(&inputstate->cogs[a].currentAngularVelocity, -24,
                               24, 50, 0, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->cogs[a].targetAngularVelocity, -6,
                               6, 20, 0, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
  }
  for (a = 0; a < 2; a++) {
    all_small_changes_to_field(
        &inputstate->spinningtriangles[a].currentAngularVelocity, -24, 24, 50,
        0, best_so_far, inputstate, steps_since_last_increase, depth, seed);
    all_small_changes_to_field(
        &inputstate->spinningtriangles[a].targetAngularVelocity, -6, 6, 20, 0,
        best_so_far, inputstate, steps_since_last_increase, depth, seed);
  }
  all_small_changes_to_field(&inputstate->pitblock.height, 0, 30, 11, -71,
                             best_so_far, inputstate, steps_since_last_increase,
                             depth, seed);
  all_small_changes_to_field(&inputstate->pitblock.verticalSpeed, 0, 1, 20, -9,
                             best_so_far, inputstate, steps_since_last_increase,
                             depth, seed);
  all_small_changes_to_field(&inputstate->pitblock.state, 0, 1, 1, 0,
                             best_so_far, inputstate, steps_since_last_increase,
                             depth, seed);
  // inputstate->pitblock.max = (randbetween<0, 6>() * 20 + 9) % 110 + 1;
  all_small_changes_to_field(
      &inputstate->pitblock.counter, 0, inputstate->pitblock.max, 1, 0,
      best_so_far, inputstate, steps_since_last_increase, depth, seed);
  for (a = 0; a < 2; a++) {
    all_small_changes_to_field(&inputstate->hands[a].angle, -6, 6, 182, 0,
                               best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->hands[a].targetAngle, -1, 1, 1092,
                               0, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->hands[a].displacement, 0, 1, 2184,
                               -1092, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    // inputstate->hands[a].directionTimer =
    //     (randbetween<0, 5>() * 60 + 29) % 270 + 1;
    // inputstate->hands[a].max = randbetween<0, 2>() * 20 + 10;
    all_small_changes_to_field(
        &inputstate->hands[a].timer, 0, inputstate->hands[a].max, 1, 0,
        best_so_far, inputstate, steps_since_last_increase, depth, seed);
  }
  for (a = 0; a < 14; a++) {
    // all_small_changes_to_field(&inputstate->spinners[a].direction, 0, 1, 2,
    // -1,
    //                            best_so_far, inputstate,
    //                            steps_since_last_increase, depth, seed);
    // inputstate->spinners[a].max = randbetween<0, 3>() * 30 + 30;
    all_small_changes_to_field(&inputstate->spinners[a].counter, 0, 120, 1, 0,
                               best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
  }
  for (a = 0; a < 6; a++) {
    // inputstate->wheels[a].max = randbetween<0, 2>() * 20 + 10;
    all_small_changes_to_field(&inputstate->wheels[a].angle, -14, 14, 234, 0,
                               best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->wheels[a].targetAngle, -1, 1, 3276,
                               0, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    all_small_changes_to_field(&inputstate->wheels[a].displacement, 0, 1,
                               2 * 3276, -3276, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
    // inputstate->wheels[a].directionTimer =
    //     (randbetween<0, 5>() * 60 + 29) % 270 + 1;
    all_small_changes_to_field(
        &inputstate->wheels[a].timer, 0, inputstate->wheels[a].max, 1, 0,
        best_so_far, inputstate, steps_since_last_increase, depth, seed);
  }
  for (a = 0; a < 2; a++) {
    // inputstate->elevators[a].max = randbetween<1, 6>() * 30;
    all_small_changes_to_field(&inputstate->elevators[a].counter, 0, 180, 1, 0,
                               best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
  }
  all_small_changes_to_field(&inputstate->sixthcog.currentAngularVelocity, -24,
                             24, 50, 0, best_so_far, inputstate,
                             steps_since_last_increase, depth, seed);
  all_small_changes_to_field(&inputstate->sixthcog.targetAngularVelocity, -6, 6,
                             20, 0, best_so_far, inputstate,
                             steps_since_last_increase, depth, seed);

  all_small_changes_to_field(&inputstate->thwomp.height, 6192, 6607, 1, 0,
                             best_so_far, inputstate, steps_since_last_increase,
                             depth, seed);
  // inputstate->thwomp.max = randbetween<10, 39>();
  all_small_changes_to_field(
      &inputstate->thwomp.counter, 0, inputstate->thwomp.max, 1, 0, best_so_far,
      inputstate, steps_since_last_increase, depth, seed);
  all_small_changes_to_field(&inputstate->thwomp.state, 0, 4, 1, 0, best_so_far,
                             inputstate, steps_since_last_increase, depth,
                             seed);
  all_small_changes_to_field(&inputstate->thwomp.verticalSpeed, 0, 104, -4, 0,
                             best_so_far, inputstate, steps_since_last_increase,
                             depth, seed);
  for (a = 0; a < 2; a++) {
    all_small_changes_to_field(&inputstate->bobombs[a].blinkingTimer, 0, 15, 1,
                               0, best_so_far, inputstate,
                               steps_since_last_increase, depth, seed);
  }
}

// pick a random state for each search and find a new state with a small random
// change to that state
void runsimulation_randomstates() {
  printf("Running\n");
  // initialize_rand();
  objects_t *currentstartingarray =
      (objects_t *)malloc(sizeof(*currentstartingarray));
  memset(currentstartingarray, 0, sizeof(objects_t));
  while (true) {
    randomizearray(currentstartingarray);
    auto p = steps_still_for_state(currentstartingarray);
    int length = p.first;
    int seed = p.second;
    most_frames_lasted = std::max(length, most_frames_lasted);
    states_checked += 1;
    printf("checking top level, best so far is %d, states checked is %ld, "
           "seed = %d\n",
           most_frames_lasted, states_checked, seed);
    check_small_changes(length, currentstartingarray, 0, 1, seed);
  }
  free(currentstartingarray);
}

int main() {
  // gen.seed(0);
  printf("size of objects_t = %zu\n", sizeof(objects_t));
  runsimulation_randomstates();
  // int seed = 0;
  // for (int i = 0; i < num_seeds; i++) {
  //   printf(" i = %d, seed = %d, next = %d, previous = %d\n", i, seed,
  //          rng_function_table[seed], backwords_rng_function_table[seed]);
  //   seed = rng_function(seed);
  // }
  return 0;
}