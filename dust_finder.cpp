#include "state_score.hpp"
#include "unordered_set"
#include <chrono>
#include <iostream>

long states_checked = 0;
int most_frames_lasted = 0;
std::map<long, long> found_per_length;
long print_freq = 200000;
std::unordered_set<std::vector<bool>> stored_dust_vectors;
unsigned long skipped_states = 0;
std::chrono::time_point<std::chrono::system_clock> start_time;

void print_waiting_frames(const std::vector<bool> &dust_frames) {
  printf("\rdust vector is:");
  for (auto dust : dust_frames) {
    if (dust) {
      printf("+");
    } else {
      printf("-");
    }
  }
}

void print_search_info(const std::vector<std::pair<std::vector<bool>, size_t>>
                           &dust_frames_stack) {
  printf("states checked = %lu, most_frames_lasted = %d, skipped states = %lu, "
         "number of stored states = %zu\n",
         states_checked, most_frames_lasted, skipped_states,
         stored_dust_vectors.size());
  std::cout << "running for "
            << std::chrono::duration_cast<std::chrono::minutes>(
                   std::chrono::system_clock::now() - start_time)
                   .count()
            << " minutes so far" << std::endl;
  for (const auto &pair : found_per_length) {
    printf("{%lu, %lu}, ", pair.first, pair.second);
  }
  printf("\n");
  printf("path we took to get here\n");
  for (const auto &pair : dust_frames_stack) {
    print_waiting_frames(pair.first);
    printf("   lasted %zu\n", pair.second);
  }
  printf("\n");
}

int steps_still_for_state_add_remove_dust(std::vector<bool> &dust_frames,
                                          objects_t &states,
                                          size_t dust_frame_to_start_with) {
  // wait some amount of frames making dust for some portion of them
  for (size_t i = dust_frame_to_start_with; i < dust_frames.size(); i++) {
    advanceobjects(&states);
    if (dust_frames[i]) {
      pollRNG(&states.rngValue);
      pollRNG(&states.rngValue);
      pollRNG(&states.rngValue);
      pollRNG(&states.rngValue);
    }
  }
  states.rcpscog.small_enough_movement_so_far = 1;
  // if the target is bad skip this state
  if (states.rcpscog.targetAngularVelocity > 200 ||
      states.rcpscog.targetAngularVelocity < -200) {
    states.rcpscog.small_enough_movement_so_far = 0;
    return 0;
  }
  // if the target is good, but the current is bad, wait until it is good, and
  // then try that
  while (states.rcpscog.currentAngularVelocity > 200 ||
         states.rcpscog.currentAngularVelocity < -200) {
    advanceobjects(&states);
    dust_frames.push_back(false);
  }

  int a = 0;
  for (a = 0; a < 1200; a++) {
    advanceobjects(&states);
    // printobjectstates(&states);
    if (states.rcpscog.small_enough_movement_so_far == 0) {
      return a;
    }
  }

  if (states.rcpscog.small_enough_movement_so_far == 1) {
    printf("cog was still the whole time !!!\n");
    print_waiting_frames(dust_frames);
    printf("\n");
    exit(0);
  }

  return a;
}
void check_small_changes_add_remove_dust(
    int best_so_far, int steps_since_last_increase, int depth,
    std::vector<std::pair<std::vector<bool>, size_t>> dust_frames_stack,
    int bad_steps_allowed);

void check_state_and_recurse_add_remove_dust(
    int best_so_far, int steps_since_last_increase, int depth,
    std::vector<bool> dust_frames,
    std::vector<std::pair<std::vector<bool>, size_t>> dust_frames_stack,
    int bad_steps_allowed, objects_t states, size_t dust_frame_to_start_with) {
  // print_waiting_frames(dust_frames);
  for (const auto &pair : dust_frames_stack) {
    // this is already in the stack somewhere, just skip it
    if (dust_frames == pair.first) {
      return;
    }
  }
  int length = steps_still_for_state_add_remove_dust(dust_frames, states,
                                                     dust_frame_to_start_with);

  states_checked += 1;
  if (states_checked % print_freq == 0) {
    print_search_info(dust_frames_stack);
  }
  if (length >= best_so_far || length >= 20) {
    if (stored_dust_vectors.count(dust_frames)) {
      skipped_states++;
      return;
    }
    stored_dust_vectors.insert(dust_frames);
  }
  found_per_length[length]++;
  if (length > best_so_far) {
    most_frames_lasted = std::max(most_frames_lasted, length);
    dust_frames_stack.emplace_back(dust_frames, length);
    if (length > most_frames_lasted - 5) {
      // extra confirm
      if (false) {
        objects_t state_check;
        int check_length =
            steps_still_for_state_add_remove_dust(dust_frames, state_check, 0);
        if (check_length != length) {
          printf("something is wrong\n");
        }
      }

      printf("new best on path = %d depth = %d\n", length, depth);
      print_search_info(dust_frames_stack);
    }
    check_small_changes_add_remove_dust(length, 0, depth + 1, dust_frames_stack,
                                        bad_steps_allowed);
    // } else if (length == best_so_far && length > 50) {
    //   most_frames_lasted = std::max(most_frames_lasted, length);
    //   if (length > most_frames_lasted - 5) {
    //     print_waiting_frames(dust_frames);
    //     printf("\nmatching best on path = %d, states_checked = %ld, "
    //            "depth = %d, most_frames_lasted overall = %d\n",
    //            length, states_checked, depth, most_frames_lasted);
    //     for (const auto &pair : found_per_length) {
    //       printf("{%lu, %lu}, ", pair.first, pair.second);
    //     }
    //     printf("\n");
    //   }
    //   dust_frames_stack.emplace_back(dust_frames, length);
    //   check_small_changes_add_remove_dust(length, steps_since_last_increase +
    //   1,
    //                                       depth + 1, dust_frames_stack,
    //                                       bad_steps_allowed);
  } else if (steps_since_last_increase < bad_steps_allowed) {
    dust_frames_stack.emplace_back(dust_frames, length);
    check_small_changes_add_remove_dust(
        best_so_far, steps_since_last_increase + 1, depth + 1,
        dust_frames_stack, bad_steps_allowed);
  }
}

static long max_states_to_check = std::numeric_limits<int64_t>::max();

void check_small_changes_add_remove_dust(
    int best_so_far, int steps_since_last_increase, int depth,
    std::vector<std::pair<std::vector<bool>, size_t>> dust_frames_stack,
    int bad_steps_allowed) {
  // // just for helping compare optimizations
  if (states_checked >= max_states_to_check) {
    exit(0);
  }
  objects_t state;
  std::vector<bool> dust_frames = dust_frames_stack.back().first;
  // gotten from pannen as the starting rng seed, update if nessasary
  for (size_t i = 0; i < dust_frames.size(); i++) {
    // first try just swapping this frame
    dust_frames[i] = !dust_frames[i];
    check_state_and_recurse_add_remove_dust(
        best_so_far, steps_since_last_increase, depth, dust_frames,
        dust_frames_stack, bad_steps_allowed, state, i);

    size_t biggest_move_size = 5;

    // then try moving a later frame to this frame
    for (size_t j = i + 1;
         j < std::min(dust_frames.size(), i + biggest_move_size); j++) {
      // if its equal to the flipped value, then assume we moved the value
      // and flip the other one
      if (dust_frames[j] == dust_frames[i] && dust_frames[i]) {
        dust_frames[j] = !dust_frames[j];
        check_state_and_recurse_add_remove_dust(
            best_so_far, steps_since_last_increase, depth, dust_frames,
            dust_frames_stack, bad_steps_allowed, state, i);
        dust_frames[j] = !dust_frames[j];
      }
    }
    dust_frames[i] = !dust_frames[i];
    advanceobjects(&state);
    if (dust_frames[i]) {
      pollRNG(&state.rngValue);
      pollRNG(&state.rngValue);
      pollRNG(&state.rngValue);
      pollRNG(&state.rngValue);
    }
  }
  // try adding a frame either way
  dust_frames.push_back(true);
  check_state_and_recurse_add_remove_dust(
      best_so_far, steps_since_last_increase, depth, dust_frames,
      dust_frames_stack, bad_steps_allowed, state, dust_frames.size());
  dust_frames.pop_back();
  dust_frames.push_back(false);
  check_state_and_recurse_add_remove_dust(
      best_so_far, steps_since_last_increase, depth, dust_frames,
      dust_frames_stack, bad_steps_allowed, state, dust_frames.size());
  dust_frames.pop_back();
  // try removing a frame
  if (!dust_frames.empty()) {
    bool back = dust_frames.back();
    dust_frames.pop_back();
    check_state_and_recurse_add_remove_dust(
        best_so_far, steps_since_last_increase, depth, dust_frames,
        dust_frames_stack, bad_steps_allowed, {}, 0);
    dust_frames.push_back(back);
  }
  // leave it as you found it
}
std::vector<bool> read_vector_from_string(std::string frames) {
  std::vector<bool> vec;
  for (const auto &f : frames) {
    if (f == '+') {
      vec.push_back(true);
    } else if (f == '-') {
      vec.push_back(false);
    } else {
      printf("shouldn't happen\n");
    }
  }
  return vec;
}

// start with state, and a number of frames to stay wait
// make a new state by changing a state from nothing to dust, dust to nothing,
// or adding or removing a frame to wait
void runsimulation_add_remove_dust(int frames_to_wait, int bad_steps_allowed) {
  printf("Running\n");
  // initialize_rand();

  std::vector<bool> dust_frames(frames_to_wait);
  for (size_t i = 0; i < dust_frames.size(); i++) {
    dust_frames[i] = randbetween<0, 10>() == 0;
  }
  // dust_frames = read_vector_from_string(
  //     "-----+------------+----------------+---------------------------------++-"
  //     "-------+-+------------+--------+----+-+-------------+----+--------+-----"
  //     "------------+-+-----------------------------------------");
  while (true) {
    objects_t state;
    int length = steps_still_for_state_add_remove_dust(dust_frames, state, 0);
    states_checked += 1;
    printf("start is %d\n", length);
    check_small_changes_add_remove_dust(length, 0, 1, {{dust_frames, length}},
                                        bad_steps_allowed);
    printf("finished seraching from the starting point, starting over\n");
    dust_frames.clear();
    for (size_t i = 0; i < dust_frames.size(); i++) {
      dust_frames[i] = randbetween<0, 10>() == 0;
    }
  }
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    printf("usage\n./dust_finder <waiting frames> <bad steps allowed> <print "
           "frequency>\n");
    exit(1);
  }
  start_time = std::chrono::system_clock::now();
  int frames_to_wait = atoi(argv[1]);
  int bad_steps = atoi(argv[2]);
  print_freq = atoll(argv[3]);
  if (argc == 5) {
    max_states_to_check = atol(argv[4]);
  }
  runsimulation_add_remove_dust(frames_to_wait, bad_steps);
  return 0;
}